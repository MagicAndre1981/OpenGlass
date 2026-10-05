#pragma once
#include <Windows.h>
#include <sddl.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <span>
#include <algorithm>

namespace OpenGlass::ManagedFiles
{
	inline constexpr auto Acl = L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GRGX;;;BU)(A;OICI;GRGX;;;S-1-5-90-0)"
#ifdef OPENGLASS_PRESET_TEST_STORAGE
		L"(A;OICI;FA;;;OW)"
#endif
		;
	inline void CheckPath(const std::filesystem::path& path)
	{
		for (auto current = std::filesystem::absolute(path).lexically_normal(); !current.empty();)
		{
			const auto attributes = GetFileAttributesW(current.c_str());
			THROW_HR_IF(E_INVALIDARG, attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT));
			const auto parent = current.parent_path();
			if (parent == current) break;
			current = parent;
		}
	}
	inline bool Within(const std::filesystem::path& path, const std::filesystem::path& root)
	{
		const auto value = std::filesystem::absolute(path).lexically_normal().wstring();
		auto prefix = std::filesystem::absolute(root).lexically_normal().wstring();
		if (prefix.back() != L'\\') prefix += L'\\';
		return value.size() > prefix.size() && CompareStringOrdinal(value.c_str(), static_cast<int>(prefix.size()), prefix.c_str(), static_cast<int>(prefix.size()), TRUE) == CSTR_EQUAL;
	}
	inline void Protect(const std::filesystem::path& path)
	{
		CheckPath(path);
		PSECURITY_DESCRIPTOR descriptor{};
		THROW_IF_WIN32_BOOL_FALSE(ConvertStringSecurityDescriptorToSecurityDescriptorW(Acl, SDDL_REVISION_1, &descriptor, nullptr));
		wil::unique_hlocal owner(descriptor);
		THROW_IF_WIN32_BOOL_FALSE(SetFileSecurityW(path.c_str(), DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, descriptor));
	}
	inline void Directory(const std::filesystem::path& path)
	{
		CheckPath(path);
		if (!std::filesystem::exists(path))
		{
			Directory(path.parent_path());
			std::filesystem::create_directory(path);
			Protect(path);
		}
	}
	inline std::vector<std::byte> Read(const std::filesystem::path& path)
	{
		CheckPath(path);
		const auto size = std::filesystem::file_size(path);
		THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE), size > 128ull * 1024 * 1024);
		std::vector<std::byte> bytes(static_cast<size_t>(size));
		std::ifstream file(path, std::ios::binary);
		file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
		THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_READ_FAULT), !file || file.peek() != EOF);
		return bytes;
	}
	inline bool Matches(const std::filesystem::path& path, const std::vector<std::byte>& bytes)
	{
		try { return Read(path) == bytes; } catch (...) { return false; }
	}
	inline void Write(const std::filesystem::path& path, std::span<const std::byte> bytes)
	{
		Directory(path.parent_path()); CheckPath(path);
		wil::unique_hfile file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
		THROW_LAST_ERROR_IF(!file);
		DWORD written{};
		THROW_IF_WIN32_BOOL_FALSE(WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr));
		THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_WRITE_FAULT), written != bytes.size());
		THROW_IF_WIN32_BOOL_FALSE(FlushFileBuffers(file.get()));
		file.reset(); Protect(path);
	}
	inline bool ProtectedFile(const std::filesystem::path& path)
	{
		const auto attr = GetFileAttributesW(path.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES || (attr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) return false;
		DWORD size{};
		GetFileSecurityW(path.c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &size);
		if (!size) return false;
		std::vector<std::byte> bytes(size);
		if (!GetFileSecurityW(path.c_str(), DACL_SECURITY_INFORMATION, bytes.data(), size, &size)) return false;
		SECURITY_DESCRIPTOR_CONTROL control{}; DWORD revision{};
		if (!GetSecurityDescriptorControl(bytes.data(), &control, &revision) || !(control & SE_DACL_PROTECTED)) return false;
		PSECURITY_DESCRIPTOR expected{};
		if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(Acl, SDDL_REVISION_1, &expected, nullptr)) return false;
		wil::unique_hlocal owner(expected);
		PACL actualAcl{}, expectedAcl{}; BOOL present{}, defaulted{};
		if (!GetSecurityDescriptorDacl(bytes.data(), &present, &actualAcl, &defaulted) || !present || !actualAcl) return false;
		if (!GetSecurityDescriptorDacl(expected, &present, &expectedAcl, &defaulted) || !expectedAcl) return false;
		if (actualAcl->AceCount != expectedAcl->AceCount) return false;
		GENERIC_MAPPING mapping{ FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS };
		for (DWORD index = 0; index < actualAcl->AceCount; ++index)
		{
			ACCESS_ALLOWED_ACE *actual{}, *wanted{};
			if (!GetAce(actualAcl, index, reinterpret_cast<void**>(&actual)) || !GetAce(expectedAcl, index, reinterpret_cast<void**>(&wanted))) return false;
			if (actual->Header.AceType != ACCESS_ALLOWED_ACE_TYPE || wanted->Header.AceType != ACCESS_ALLOWED_ACE_TYPE
				|| ((actual->Header.AceFlags ^ wanted->Header.AceFlags) & INHERIT_ONLY_ACE) || !EqualSid(reinterpret_cast<PSID>(&actual->SidStart), reinterpret_cast<PSID>(&wanted->SidStart))) return false;
			auto actualMask = actual->Mask, wantedMask = wanted->Mask;
			MapGenericMask(&actualMask, &mapping); MapGenericMask(&wantedMask, &mapping);
			if (actualMask != wantedMask) return false;
		}
		return true;
	}
	inline bool LinkOrCopy(const std::filesystem::path& source, const std::filesystem::path& target, const std::filesystem::path& managedRoot)
	{
		CheckPath(source); CheckPath(target); Directory(target.parent_path());
		const auto name = source.filename().wstring();
		const bool imageOrLayout = name.ends_with(L".png") || name.ends_with(L".png.layout");
		if (imageOrLayout && Within(source, managedRoot) && ProtectedFile(source) && CreateHardLinkW(target.c_str(), source.c_str(), nullptr)) return true;
		Write(target, Read(source));
		return false;
	}
	inline void RemoveTree(const std::filesystem::path& path, const std::filesystem::path& root)
	{
		THROW_HR_IF(E_INVALIDARG, !Within(path, root)); CheckPath(path);
		if (!std::filesystem::exists(path)) return;
		if (std::filesystem::is_directory(path))
			for (const auto& item : std::filesystem::recursive_directory_iterator(path)) CheckPath(item.path());
		std::filesystem::remove_all(path); // Never mutate attributes/ACLs of shared hard links.
	}
}
