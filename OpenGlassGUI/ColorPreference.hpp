#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include <optional>
#include <functional>
#include "RegistryConfig.hpp"

namespace OpenGlass
{
	// Semantic user choice, independent of asynchronous Windows-generated colors.
	class ColorPreference final
	{
		struct State;
		std::unique_ptr<State> m_state;
		HRESULT RestoreChanges(bool attempt) noexcept;
	public:
		struct Snapshot
		{
			DWORD automatic{};
			std::optional<DWORD> rgb; // Manual RGB only; Automatic never captures derived RGB.
			bool applyChoice{ true }; // Accent-only recovery must not change the coloring mode.
			std::optional<RegistryConfig::RawValue> accent; // Engaged only when AccentColorMenu participates.
			bool IsAutomatic() const noexcept { return automatic != 0; }
			bool RestoresAutomatic() const noexcept { return applyChoice && IsAutomatic(); }
			bool operator==(const Snapshot&) const = default;
		};
		// Backend boundary also lets tests exercise the real journal without Windows writes.
		struct Backend
		{
			virtual ~Backend() = default;
			virtual HRESULT Capture(const std::wstring& userSid, Snapshot& snapshot) noexcept = 0;
			virtual HRESULT Prepare(const Snapshot& choice) noexcept = 0;
			virtual HRESULT Apply(const Snapshot& choice) noexcept = 0;
		};
		ColorPreference();
		explicit ColorPreference(std::unique_ptr<Backend> backend);
		~ColorPreference();
		HRESULT Capture(const std::wstring& userSid, Snapshot& snapshot) noexcept;
		HRESULT RollbackAttempt() noexcept;
		HRESULT RecoverSnapshot(const Snapshot& snapshot) noexcept;
		std::optional<Snapshot> Baseline(bool attempt = false) const;
		void CommitAttempt() noexcept;
		HRESULT ReadRgb(const std::wstring& userSid, DWORD& rgb) noexcept;
		HRESULT ReadAutoColorization(const std::wstring& userSid, bool& enabled) noexcept;
		HRESULT Apply(const std::wstring& userSid, std::optional<DWORD> argb,
			const std::function<void(const Snapshot&)>& beforeWrite = {}) noexcept;
		HRESULT Revert() noexcept;
		void Accept() noexcept;
		bool IsDirty() const noexcept;
	};
}
