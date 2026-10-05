#pragma once
#include "RegistryConfig.hpp"
#include <map>

namespace OpenGlass::ConfigurationMigration
{
	struct Change
	{
		Settings::Scope scope;
		Settings::Id id;
		RegistryConfig::RawValue before;
		RegistryConfig::RawValue after;
		std::wstring description;
		std::wstring Name() const { return std::wstring(Settings::Get(id).name); }
		bool operator==(const Change&) const = default;
	};
	using Values = std::map<Settings::Id, RegistryConfig::RawValue>;
	[[nodiscard]] std::vector<Change> Prepare(Values users, Values machines, Settings::Scope target);
	[[nodiscard]] std::vector<Change> Prepare(const RegistryConfig& user, const RegistryConfig& machine, Settings::Scope target);
}
