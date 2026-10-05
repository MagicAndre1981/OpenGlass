#include "pch.h"
#include "MainFrame.hpp"
#include "ColorSwatchButton.hpp"
#include "EffectiveConfiguration.hpp"

namespace OpenGlass
{
	void MainFrame::ApplyColorizationBalances(DWORD intensity)
	{
		const auto parameters = ColorizationPresets::CalculateWindows7Parameters(
			ColorizationPresets::CalculateIntensityAlpha(intensity) << 24,
			!m_chkEnableTransparency->IsChecked());
		const std::pair<Settings::Id, DWORD> values[]
		{
			{ Settings::Id::ColorizationColorBalanceOverride, parameters.colorBalance },
			{ Settings::Id::ColorizationAfterglowBalanceOverride, parameters.afterglowBalance },
			{ Settings::Id::ColorizationBlurBalanceOverride, parameters.blurBalance }
		};
		for (const auto& [id, value] : values)
		{
			TrackSettingChange(id);
			const std::wstring name(Settings::Get(id).name);
			THROW_IF_FAILED(m_config->SetDword(name, value));
		}
	}

	void MainFrame::ApplyColorizationColor(std::optional<DWORD> argb, ColorizationPresets::Family family)
	{
		if (!m_config)
		{
			return;
		}

		if (RunPreview([&]
		{
			auto setDword = [this](Settings::Id id, DWORD value) {
				const std::wstring name(Settings::Get(id).name);
				TrackSettingChange(id);
				THROW_IF_FAILED(m_config->SetDword(name, value));
			};

			const auto cleanup = EffectiveConfiguration::PlanColorCleanup(EffectiveConfiguration::Read(*m_userConfig), EffectiveConfiguration::Read(*m_systemConfig));
			THROW_IF_FAILED(m_userConfig->CheckDeleteAccess());
			THROW_IF_FAILED(m_systemConfig->CheckDeleteAccess());
			THROW_IF_FAILED(m_colorPreference.Apply(m_targetUserSid.ToStdWstring(), argb));
			for (const auto& change : cleanup)
			{
				THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_RETRY), GetConfigForScope(change.scope)->ReadRaw(change.Name()) != change.before);
				TrackSettingChange(change.scope, change.id);
				THROW_IF_FAILED(GetConfigForScope(change.scope)->WriteRaw(change.Name(), change.after));
			}
			if (!argb) return; // Automatic selection retains the configured intensity.
			const auto intensity = ColorizationPresets::CalculateVistaOpacity(*argb);
			setDword(Settings::Id::GlassOpacity, intensity);
			if (family == ColorizationPresets::Family::Windows7) ApplyColorizationBalances(intensity);
		}, true, Settings::UpdateImpact::Colorization)) LoadSettings();
	}

	void MainFrame::ApplyColorizationPreset(const ColorizationPresets::Preset& preset)
	{
		ApplyColorizationColor(preset.argb, preset.family);
	}

	const ColorizationPresets::Preset* MainFrame::FindMatchingWindows7Preset(bool opaque) const
	{
		if (!m_config || !m_rbGlassType || m_rbGlassType->GetSelection() != 1)
		{
			return nullptr;
		}

		const DWORD color = m_userConfig->GetDword(L"ColorizationColor", 0xFF000000);
		const DWORD afterglow = m_userConfig->GetDword(L"ColorizationAfterglow", 0);
		const DWORD colorBalance = m_config->GetDword(L"ColorizationColorBalanceOverride", 10);
		const DWORD afterglowBalance = m_config->GetDword(L"ColorizationAfterglowBalanceOverride", 10);
		const DWORD blurBalance = m_config->GetDword(L"ColorizationBlurBalanceOverride", 50);

		for (const auto& preset : ColorizationPresets::Windows7)
		{
			if (ColorizationPresets::MatchesWindows7Preset(preset,
				{ color, afterglow, colorBalance, afterglowBalance, blurBalance },
				m_config->GetDword(L"GlassOpacity", 63), opaque))
			{
				return &preset;
			}
		}

		return nullptr;
	}

	WXLRESULT MainFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
	{
		const auto result = wxFrame::MSWWindowProc(message, wParam, lParam);
		if (message == WM_DWMCOLORIZATIONCOLORCHANGED || message == WM_SETTINGCHANGE)
		{
			QueueColorizationRefresh();
		}
		return result;
	}

	void MainFrame::QueueColorizationRefresh()
	{
		if (!m_customColorsInitialized || m_colorizationRefreshPending || IsBeingDeleted()) return;
		m_colorizationRefreshPending = true;
		CallAfter([this]
		{
			m_colorizationRefreshPending = false;
			// Our own attempt refreshes controls when complete; don't read intermediate writes.
			if (IsBeingDeleted() || m_preview.IsAttemptActive()) return;
			// Read only: external color changes must not alter the preview journal or saved presets.
			UpdateColorizationPresetSelection();
		});
	}

	void MainFrame::UpdateColorizationPresetSelection()
	{
		if (!m_config || !m_rbGlassType)
		{
			return;
		}

		bool automatic{};
		const auto modeResult = m_colorPreference.ReadAutoColorization(m_targetUserSid.ToStdWstring(), automatic);
		const bool manual = SUCCEEDED(modeResult) && !automatic;
		for (auto* button : m_automaticColorButtons)
		{
			button->SetValue(SUCCEEDED(modeResult) && automatic);
			button->SetToolTip(SUCCEEDED(modeResult) ? wxString(L"Automatic accent color from the wallpaper")
				: wxString::Format(L"Could not read the automatic accent color setting (0x%08lX).", modeResult));
		}
		const ColorizationPresets::Preset* selectedPreset = nullptr;
		if (m_rbGlassType->GetSelection() == 0)
		{
			const DWORD color = m_userConfig->GetDword(L"ColorizationColor", 0xFF000000);
			const DWORD opacity = m_config->GetDword(L"GlassOpacity", 63);
			for (const auto& preset : ColorizationPresets::Vista)
			{
				if (
					(color & 0xFFFFFF) == (preset.argb & 0xFFFFFF)
					&& opacity == ColorizationPresets::CalculateVistaOpacity(preset.argb)
				)
				{
					selectedPreset = &preset;
					break;
				}
			}
		}
		else
		{
			selectedPreset = FindMatchingWindows7Preset(
				m_chkEnableTransparency && !m_chkEnableTransparency->IsChecked()
			);
		}

		for (const auto& [preset, button] : m_presetButtons)
		{
			if (button)
			{
				button->SetValue(manual && preset == selectedPreset);
			}
		}

		DWORD color = m_userConfig->GetDword(L"ColorizationColor", 0xFF000000);
		bool colorAvailable = true;
		if (SUCCEEDED(modeResult) && automatic)
		{
			// Read the original user's current accent, never the elevated account or message payload.
			colorAvailable = SUCCEEDED(m_colorPreference.ReadRgb(m_targetUserSid.ToStdWstring(), color));
		}
		for (auto* button : m_customColorButtons)
		{
			if (button)
			{
				if (colorAvailable && (!m_customColorsInitialized || automatic || selectedPreset == nullptr))
				{
					button->SetColor((color & 0xFFFFFF) | (ColorizationPresets::CalculateIntensityAlpha(
						std::min<DWORD>(m_config->GetDword(L"GlassOpacity", 63), 100)) << 24));
				}
				button->SetValue(manual && selectedPreset == nullptr);
			}
		}
		m_customColorsInitialized = true;
	}
}
