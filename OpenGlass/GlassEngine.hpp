#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "RegistryValueResolver.hpp"
#include "uDWMProjection.hpp"

namespace OpenGlass::GlassEngine
{
	enum UpdateType : UCHAR
	{
		None = 0,
		Framework = 1 << 0,
		Backdrop = 1 << 1,
		Theme = 1 << 2,
		Hook = 1 << 3,
		All = Backdrop | Framework
	};

	HKEY GetDwmKey();
	HKEY GetPersonalizeKey();

	// Both architectures use the same scope-independent inheritance contract.
	FORCEINLINE std::optional<DWORD> TryGetDwordFromRegistry(PCWSTR keyName)
	{
		DWORD value{};
		if (SUCCEEDED(wil::reg::get_value_dword_nothrow(GetDwmKey(), keyName, &value))) return value;
		if (SUCCEEDED(wil::reg::get_value_dword_nothrow(HKEY_LOCAL_MACHINE,
			L"Software\\Microsoft\\Windows\\DWM", keyName, &value))) return value;
		return std::nullopt;
	}

	FORCEINLINE DWORD GetDwordFromRegistry(PCWSTR keyName, DWORD defaultValue = 0)
	{
		return TryGetDwordFromRegistry(keyName).value_or(defaultValue);
	}

	DWORD GetOverridableDwordFromRegistry(
		PCWSTR baseKeyName,
		PCWSTR overrideKeyName,
		DWORD defaultValue = 0
	);

	template <size_t Length>
	FORCEINLINE void GetStringFromRegistry(PCWSTR keyName, WCHAR(&returnValue)[Length])
	{
		HRESULT hr{ S_OK };
		hr = wil::reg::get_value_string_nothrow(
			GetDwmKey(),
			keyName,
			returnValue
		);
		if (FAILED(hr))
		{
			hr = wil::reg::get_value_string_nothrow(
				HKEY_LOCAL_MACHINE,
				L"Software\\Microsoft\\Windows\\DWM",
				keyName,
				returnValue
			);
		}
		if (FAILED(hr)) returnValue[0] = L'\0';
	}

	void LoadRegistry(bool redrawNow = true);
	void UnloadRegistry();

	void SetDwmNotificationWindow(HWND hWnd);
	HWND GetDwmNotificationWindow();

	void Update(UpdateType type, bool redrawNow = true);
	void Startup();
	void Activate();
	void Shutdown();
}
