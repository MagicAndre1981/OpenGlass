#pragma once

#include <optional>

namespace OpenGlass
{
	enum class RegistryValueSource
	{
		UserOverride,
		UserBase,
		MachineOverride,
		MachineBase,
		Default
	};

	template <typename T>
	struct ResolvedRegistryValue
	{
		T value;
		RegistryValueSource source;

		[[nodiscard]] constexpr bool IsOverride() const noexcept
		{
			return source == RegistryValueSource::UserOverride
				|| source == RegistryValueSource::MachineOverride;
		}
	};

	template <typename T>
	[[nodiscard]] constexpr ResolvedRegistryValue<T> ResolveOverridableRegistryValue(
		const std::optional<T>& userOverride,
		const std::optional<T>& userBase,
		const std::optional<T>& machineOverride,
		const std::optional<T>& machineBase,
		T defaultValue
	) noexcept
	{
		if (userOverride)
		{
			return { *userOverride, RegistryValueSource::UserOverride };
		}
		if (machineOverride)
		{
			return { *machineOverride, RegistryValueSource::MachineOverride };
		}
		if (userBase)
		{
			return { *userBase, RegistryValueSource::UserBase };
		}
		if (machineBase)
		{
			return { *machineBase, RegistryValueSource::MachineBase };
		}
		return { defaultValue, RegistryValueSource::Default };
	}

	// Editing shows only the selected layer, or its default when uncustomized.
	// This display-only result is not the input to effective preset capture or raw backups.
	template <typename T>
	[[nodiscard]] constexpr ResolvedRegistryValue<T> ResolveEditorRegistryValue(
		bool userScope,
		const std::optional<T>& userOverride,
		const std::optional<T>& userBase,
		const std::optional<T>& machineOverride,
		const std::optional<T>& machineBase,
		T defaultValue
	) noexcept
	{
		if (userScope)
		{
			if (userOverride) return { *userOverride, RegistryValueSource::UserOverride };
			if (userBase) return { *userBase, RegistryValueSource::UserBase };
		}
		else
		{
			if (machineOverride) return { *machineOverride, RegistryValueSource::MachineOverride };
			if (machineBase) return { *machineBase, RegistryValueSource::MachineBase };
		}
		return { defaultValue, RegistryValueSource::Default };
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

	template <typename T, typename Name, typename UserReader, typename MachineReader>
	[[nodiscard]] ResolvedRegistryValue<T> ResolveOverridableRegistryValueFromReaders(
		const Name& baseName,
		const Name& overrideName,
		T defaultValue,
		UserReader&& readUser,
		MachineReader&& readMachine
	)
	{
		const auto userOverride = readUser(overrideName);
		const auto userBase = readUser(baseName);
		const auto machineOverride = readMachine(overrideName);
		const auto machineBase = readMachine(baseName);
		return ResolveOverridableRegistryValue(
			userOverride,
			userBase,
			machineOverride,
			machineBase,
			defaultValue
		);
	}
}
