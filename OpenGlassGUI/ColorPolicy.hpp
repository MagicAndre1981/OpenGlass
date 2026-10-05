#pragma once
#include <windows.h>
#include <wil/result.h>

namespace OpenGlass::ColorPolicy
{
	// Inject only the registry read so tests exercise the production policy rules.
	template<typename Reader>
	bool IsSelectionDisabled(Reader&& read)
	{
		constexpr auto path = LR"(Software\Policies\Microsoft\Windows\Personalization)";
		wchar_t background[8]{}; // Match uxtheme's 16-byte policy buffer.
		DWORD size = sizeof(background);
		const auto backgroundStatus = read(HKEY_LOCAL_MACHINE, path, L"PersonalColors_Background",
			RRF_RT_REG_SZ, nullptr, background, &size);
		DWORD disabled{};
		size = sizeof(disabled);
		const auto changeStatus = read(HKEY_LOCAL_MACHINE, path, L"NoChangingStartMenuBackground",
			RRF_RT_REG_DWORD, nullptr, &disabled, &size);
		// An inaccessible policy is not evidence that accent selection is allowed.
		THROW_HR_IF(E_ACCESSDENIED, backgroundStatus == ERROR_ACCESS_DENIED || changeStatus == ERROR_ACCESS_DENIED);
		return backgroundStatus == ERROR_SUCCESS || (changeStatus == ERROR_SUCCESS && disabled != 0);
	}
}
