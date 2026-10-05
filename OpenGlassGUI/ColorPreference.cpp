#include "pch.h"
#include "ColorPreference.hpp"
#include "PreviewJournal.hpp"
#include "ShellColorRefresh.hpp"

namespace OpenGlass
{
	namespace
	{
		constexpr auto AccentKey = LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\Accent)";
		constexpr auto DesktopKey = LR"(Control Panel\Desktop)";
		std::optional<DWORD> ReadDword(HKEY user, const wchar_t* path, const wchar_t* name)
		{
			DWORD value{}, size = sizeof(value);
			const auto status = RegGetValueW(user, path, name, RRF_RT_REG_DWORD, nullptr, &value, &size);
			if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) return {};
			THROW_IF_WIN32_ERROR(status);
			THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_DATA), size != sizeof(value));
			return value;
		}
		DWORD ReadAccent(HKEY user)
		{
			const auto color = ReadDword(user, AccentKey, L"AccentColorMenu");
			THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_DATA), !color);
			return (GetRValue(*color) << 16) | (GetGValue(*color) << 8) | GetBValue(*color);
		}
		struct Preference { DWORD start, accent; };
		using SetPreference = HRESULT(WINAPI*)(const Preference*, bool);
		// Check the actual token identity, including alternate-credential elevation.
		bool TokenMatches(HANDLE token, const std::wstring& sid)
		{
			DWORD size{};
			GetTokenInformation(token, TokenUser, nullptr, 0, &size);
			std::vector<BYTE> buffer(size);
			THROW_IF_WIN32_BOOL_FALSE(GetTokenInformation(token, TokenUser, buffer.data(), size, &size));
			wil::unique_hlocal_string text;
			THROW_IF_WIN32_BOOL_FALSE(ConvertSidToStringSidW(static_cast<PSID>(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid), text.put()));
			return !_wcsicmp(text.get(), sid.c_str());
		}

		wil::unique_handle GetUserToken(const std::wstring& sid)
		{
			wil::unique_handle token;
			THROW_IF_WIN32_BOOL_FALSE(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, token.put()));
			if (!TokenMatches(token.get(), sid))
			{
				DWORD pid{}, session{}, currentSession{};
				GetWindowThreadProcessId(GetShellWindow(), &pid);
				THROW_HR_IF(E_ACCESSDENIED, !pid);
				THROW_IF_WIN32_BOOL_FALSE(ProcessIdToSessionId(pid, &session));
				THROW_IF_WIN32_BOOL_FALSE(ProcessIdToSessionId(GetCurrentProcessId(), &currentSession));
				THROW_HR_IF(E_ACCESSDENIED, session != currentSession);
				wil::unique_handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
				THROW_LAST_ERROR_IF(!process);
				THROW_IF_WIN32_BOOL_FALSE(OpenProcessToken(process.get(), TOKEN_QUERY | TOKEN_DUPLICATE, token.put()));
				THROW_HR_IF(E_ACCESSDENIED, !TokenMatches(token.get(), sid));
			}
			wil::unique_handle duplicate;
			THROW_IF_WIN32_BOOL_FALSE(DuplicateTokenEx(token.get(), TOKEN_QUERY | TOKEN_IMPERSONATE,
				nullptr, SecurityImpersonation, TokenImpersonation, duplicate.put()));
			return duplicate;
		}

		struct UserScope
		{
			wil::unique_handle previous;
			explicit UserScope(HANDLE token)
			{
				if (!OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_QUERY, TRUE, previous.put()))
				{
					THROW_LAST_ERROR_IF(GetLastError() != ERROR_NO_TOKEN);
				}
				THROW_IF_WIN32_BOOL_FALSE(SetThreadToken(nullptr, token));
			}
			~UserScope()
			{
				FAIL_FAST_IF_WIN32_BOOL_FALSE(SetThreadToken(nullptr, previous.get()));
			}
		};

		class WindowsColorBackend final : public ColorPreference::Backend
		{
			wil::unique_hmodule theme, shell;
			wil::unique_hkey userKey, desktopKey;
			wil::unique_handle token;
			std::wstring userSid;
			SetPreference set{};
			HWND shellWindow{};
		public:
			HRESULT Capture(const std::wstring& sid, ColorPreference::Snapshot& snapshot) noexcept override
			try
			{
				RETURN_HR_IF(E_INVALIDARG, sid.empty());
				if (!token)
				{
					token = GetUserToken(sid);
					userSid = sid;
				}
				RETURN_HR_IF(E_ACCESSDENIED, userSid != sid);
				if (!userKey) RETURN_IF_WIN32_ERROR(RegOpenKeyExW(HKEY_USERS, sid.c_str(), 0, KEY_READ, userKey.put()));
				ColorPreference::Snapshot captured;
				captured.automatic = ReadDword(userKey.get(), DesktopKey, L"AutoColorization");
				if (!captured.IsAutomatic()) captured.rgb = ReadAccent(userKey.get());
				snapshot = captured;
				return S_OK;
			}
			CATCH_RETURN()

			HRESULT Prepare(const ColorPreference::Snapshot& choice) noexcept override
			try
			{
				RETURN_HR_IF(E_UNEXPECTED, !token || !userKey);
				// Existing Desktop key and access are required before any state mutation.
				RETURN_IF_WIN32_ERROR(RegOpenKeyExW(userKey.get(), DesktopKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, desktopKey.put()));
				// Private Windows code uses predefined HKCU while impersonating the original user.
				RETURN_IF_WIN32_ERROR(RegDisablePredefinedCache());
				if (!choice.IsAutomatic())
				{
					RETURN_HR_IF(E_INVALIDARG, !choice.rgb || *choice.rgb > 0xFFFFFF);
					if (!theme) theme.reset(LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
					RETURN_LAST_ERROR_IF(!theme);
					set = reinterpret_cast<SetPreference>(GetProcAddress(theme.get(), MAKEINTRESOURCEA(122)));
					RETURN_HR_IF(E_NOINTERFACE, !set);
					return S_OK;
				}
				if (!shell) shell.reset(LoadLibraryExW(L"shell32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));
				RETURN_LAST_ERROR_IF(!shell);
				// Retain the shell capability check even though delivery is synchronous.
				RETURN_HR_IF(E_NOINTERFACE, !GetProcAddress(shell.get(), MAKEINTRESOURCEA(901)));
				DWORD pid{}, session{}, currentSession{};
				shellWindow = GetShellWindow();
				GetWindowThreadProcessId(shellWindow, &pid);
				RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_READY), !pid);
				RETURN_IF_WIN32_BOOL_FALSE(ProcessIdToSessionId(pid, &session));
				RETURN_IF_WIN32_BOOL_FALSE(ProcessIdToSessionId(GetCurrentProcessId(), &currentSession));
				RETURN_HR_IF(E_ACCESSDENIED, session != currentSession);
				wil::unique_handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
				RETURN_LAST_ERROR_IF(!process);
				wil::unique_handle shellToken;
				RETURN_IF_WIN32_BOOL_FALSE(OpenProcessToken(process.get(), TOKEN_QUERY, shellToken.put()));
				RETURN_HR_IF(E_ACCESSDENIED, !TokenMatches(shellToken.get(), userSid));
				return S_OK;
			}
			CATCH_RETURN()

			HRESULT Apply(const ColorPreference::Snapshot& choice) noexcept override
			try
			{
				RETURN_IF_FAILED(Prepare(choice));
				UserScope scope(token.get());
				if (choice.automatic)
				{
					const DWORD mode = *choice.automatic;
					RETURN_IF_WIN32_ERROR(RegSetValueExW(desktopKey.get(), L"AutoColorization", 0, REG_DWORD,
						reinterpret_cast<const BYTE*>(&mode), sizeof(mode)));
				}
				else
				{
					const auto status = RegDeleteValueW(desktopKey.get(), L"AutoColorization");
					if (status != ERROR_FILE_NOT_FOUND) RETURN_IF_WIN32_ERROR(status);
				}
				if (choice.IsAutomatic())
				{
					return ShellColorRefresh::Request(shellWindow);
				}
				const DWORD rgb = *choice.rgb;
				const DWORD colorref = RGB((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255);
				const Preference preference{ colorref, colorref };
				RETURN_IF_FAILED(set(&preference, true));
				// Policy can reject a private API request without a useful HRESULT.
				RETURN_HR_IF(E_ACCESSDENIED, ReadAccent(userKey.get()) != rgb);
				return S_OK;
			}
			CATCH_RETURN()
		};
	}

	struct ColorPreference::State
	{
		std::unique_ptr<Backend> backend;
		PreviewJournal<int, Snapshot> journal;
		explicit State(std::unique_ptr<Backend> value) : backend(std::move(value)) {}
	};
	ColorPreference::ColorPreference() : ColorPreference(std::make_unique<WindowsColorBackend>()) {}
	ColorPreference::ColorPreference(std::unique_ptr<Backend> backend) : m_state(std::make_unique<State>(std::move(backend))) {}
	ColorPreference::~ColorPreference() = default;

	HRESULT ColorPreference::Capture(const std::wstring& userSid, Snapshot& snapshot) noexcept
	{
		return m_state->backend->Capture(userSid, snapshot);
	}
	HRESULT ColorPreference::ReadAutoColorization(const std::wstring& userSid, bool& enabled) noexcept
	try
	{
		RETURN_HR_IF(E_INVALIDARG, userSid.empty());
		wil::unique_hkey user;
		RETURN_IF_WIN32_ERROR(RegOpenKeyExW(HKEY_USERS, userSid.c_str(), 0, KEY_READ, user.put()));
		enabled = ReadDword(user.get(), DesktopKey, L"AutoColorization").value_or(0) != 0;
		return S_OK;
	}
	CATCH_RETURN()
	HRESULT ColorPreference::ReadRgb(const std::wstring& userSid, DWORD& rgb) noexcept
	try
	{
		RETURN_HR_IF(E_INVALIDARG, userSid.empty());
		wil::unique_hkey user;
		RETURN_IF_WIN32_ERROR(RegOpenKeyExW(HKEY_USERS, userSid.c_str(), 0, KEY_READ, user.put()));
		rgb = ReadAccent(user.get());
		return S_OK;
	}
	CATCH_RETURN()

	HRESULT ColorPreference::Apply(const std::wstring& userSid, std::optional<DWORD> argb) noexcept
	try
	{
		Snapshot before;
		RETURN_IF_FAILED(Capture(userSid, before));
		const Snapshot desired{ argb ? 0u : 1u, argb ? std::optional<DWORD>(*argb & 0xFFFFFF) : std::nullopt };
		// Check both forward and recovery capabilities before any mutation.
		RETURN_IF_FAILED(m_state->backend->Prepare(desired));
		RETURN_IF_FAILED(m_state->backend->Prepare(before));
		auto& journal = m_state->journal;
		journal.Begin();
		journal.Touch(0, [&](int) { return before; });
		RETURN_IF_FAILED(m_state->backend->Apply(desired));
		// Track the choice, never Explorer's asynchronous palette/DWM output.
		journal.Reconcile([&](int) { return desired; });
		return S_OK;
	}
	CATCH_RETURN()

	HRESULT ColorPreference::RollbackAttempt() noexcept
	{
		if (!m_state->journal.IsAttemptActive()) return S_OK;
		const auto result = RestoreChanges(true);
		m_state->journal.RollbackAttempt([](int, const auto&) { return true; }, SUCCEEDED(result));
		return result;
	}
	HRESULT ColorPreference::RecoverSnapshot(const Snapshot& snapshot) noexcept
	{
		RETURN_IF_FAILED(m_state->backend->Prepare(snapshot));
		return m_state->backend->Apply(snapshot);
	}
	std::optional<ColorPreference::Snapshot> ColorPreference::Baseline() const
	{
		std::optional<Snapshot> result;
		m_state->journal.VisitRestore(false, [&](int, const Snapshot& value) { result = value; });
		return result;
	}
	HRESULT ColorPreference::RestoreChanges(bool attempt) noexcept
	{
		HRESULT result = S_OK;
		m_state->journal.VisitRestore(attempt, [&](int, const Snapshot& value) { result = m_state->backend->Apply(value); });
		return result;
	}
	void ColorPreference::CommitAttempt() noexcept { m_state->journal.CommitAttempt(); }
	HRESULT ColorPreference::Revert() noexcept { return RestoreChanges(false); }
	void ColorPreference::Accept() noexcept { m_state->journal.Accept(); }
	bool ColorPreference::IsDirty() const noexcept { return m_state->journal.IsDirty(); }
}
