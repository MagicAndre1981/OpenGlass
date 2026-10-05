#pragma once

#include <optional>

namespace OpenGlass
{
	template <typename T>
	[[nodiscard]] constexpr T ResolveOverridableRegistryValue(
		const std::optional<T>& userOverride,
		const std::optional<T>& userBase,
		const std::optional<T>& machineOverride,
		const std::optional<T>& machineBase,
		T defaultValue
	) noexcept
	{
		if (userOverride)
		{
			return *userOverride;
		}
		if (machineOverride)
		{
			return *machineOverride;
		}
		if (userBase)
		{
			return *userBase;
		}
		if (machineBase)
		{
			return *machineBase;
		}
		return defaultValue;
	}

	// Presence means a valid typed value, not equality with the displayed default.
	enum class EditorRegistryNotice
	{
		None,
		Inherited,
		Overridden
	};
	[[nodiscard]] constexpr EditorRegistryNotice GetEditorRegistryNotice(bool userScope, bool hasUserValue, bool hasMachineValue) noexcept
	{
		if (userScope) return !hasUserValue && hasMachineValue ? EditorRegistryNotice::Inherited : EditorRegistryNotice::None;
		return hasUserValue ? EditorRegistryNotice::Overridden : EditorRegistryNotice::None;
	}
}
