#pragma once
#include <windows.h>
#include <wil/result.h>

namespace OpenGlass::ColorPolicy
{
	// Inject only the registry read so tests exercise the production policy rules.
	template<typename Reader>
	bool IsAccentSyncBlocked(Reader&& read)
	{
		constexpr auto path = LR"(Software\Policies\Microsoft\Windows\Personalization)";
		wchar_t background[8]{}; // Match uxtheme's 16-byte policy buffer.
		DWORD size = sizeof(background);
		const auto backgroundStatus = read(HKEY_LOCAL_MACHINE, path, L"PersonalColors_Background",
			RRF_RT_REG_SZ, nullptr, background, &size);
		// NoChangingStartMenuBackground gates a getter's refresh, not every setter.
		// Keep the user preference current for later explicit setters, including logon.
		THROW_HR_IF(E_ACCESSDENIED, backgroundStatus == ERROR_ACCESS_DENIED);
		return backgroundStatus == ERROR_SUCCESS;
	}
}
