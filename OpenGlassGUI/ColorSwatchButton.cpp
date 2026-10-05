#include "pch.h"
#include "ColorSwatchButton.hpp"
#include <wx/msw/dc.h>

namespace OpenGlass
{
	namespace
	{
		constexpr int SwatchSizeDip = 40;

		wxColour GetSurfaceBorder()
		{
			return wxSystemSettings::GetAppearance().IsDark()
				? wxColour(102, 102, 102)
				: wxColour(154, 154, 154);
		}

		wxColour GetSelectionBorder()
		{
			return wxSystemSettings::GetAppearance().IsDark()
				? wxColour(76, 194, 255)
				: wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
		}

	}

	void DrawAutomaticColorSwatch(wxGraphicsContext& context, const wxRect2DDouble& bounds, bool compact)
	{
		const bool dark = wxSystemSettings::GetAppearance().IsDark();
		context.PushState();
		if (!compact)
		{
			context.SetPen(*wxTRANSPARENT_PEN);
			context.SetBrush(wxBrush(dark ? wxColour(53, 53, 53) : wxColour(241, 241, 241)));
			context.DrawRectangle(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height);
		}

		// A fixed color-card fan denotes the mode, not the current accent color.
		context.Translate(bounds.m_x, bounds.m_y);
		context.Scale(bounds.m_width / 40, bounds.m_height / 40);
		const wxColour colors[] = {
			wxColour(83, 145, 213), wxColour(154, 112, 190), wxColour(223, 105, 111),
			wxColour(236, 187, 80), wxColour(100, 177, 135)
		};
		const size_t count = compact ? 3 : std::size(colors);
		for (size_t index = 0; index < count; ++index)
		{
			context.PushState();
			context.Translate(20, compact ? 29 : 28);
			context.Rotate(compact
				? (static_cast<double>(index) - 1) * 0.6981317007977318 // 40 degrees
				: (static_cast<double>(index) - 2) * 0.4363323129985824); // 25 degrees
			context.SetPen(wxPen(dark ? wxColour(53, 53, 53) : wxColour(255, 255, 255), 1));
			const size_t colorIndex = compact ? (index == 0 ? 0 : index + 1) : index;
			context.SetBrush(wxBrush(colors[colorIndex]));
			if (compact) context.DrawRoundedRectangle(-4.5, -25, 9, 28, 1.2);
			else context.DrawRoundedRectangle(-3.5, -21, 7, 24, 1.2);
			context.PopState();
		}
		if (!compact)
		{
			context.SetPen(*wxTRANSPARENT_PEN);
			context.SetBrush(wxBrush(dark ? wxColour(221, 221, 221) : wxColour(255, 255, 255)));
			context.DrawEllipse(18, 26, 4, 4);
			context.SetBrush(wxBrush(wxColour(120, 120, 120)));
			context.DrawEllipse(19.2, 27.2, 1.6, 1.6);
		}
		context.PopState();
	}

	ColorSwatchButton::ColorSwatchButton(wxWindow* parent, wxWindowID id, const wxString& label, DWORD argb, bool automatic)
		: wxToggleButton(parent, id, wxEmptyString)
		, m_argb(argb)
		, m_automatic(automatic)
	{
		SetName(label);
		SetToolTip(label);
		SetBitmapMargins(0, 0);
		const wxSize buttonSize = FromDIP(wxSize(SwatchSizeDip, SwatchSizeDip));
		SetMinSize(buttonSize);
		SetMaxSize(buttonSize);
		RebuildBitmap();

		Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent& event) {
			Refresh(false);
			event.Skip();
		});
		Bind(wxEVT_DPI_CHANGED, &ColorSwatchButton::OnDpiChanged, this);
		Bind(wxEVT_SYS_COLOUR_CHANGED, &ColorSwatchButton::OnSystemColorChanged, this);
	}

	void ColorSwatchButton::SetColor(DWORD argb)
	{
		if (m_argb == argb)
		{
			return;
		}

		m_argb = argb;
		RebuildBitmap();
		Refresh(false);
	}

	void ColorSwatchButton::SetValue(bool value)
	{
		if (GetValue() == value)
		{
			return;
		}

		wxToggleButton::SetValue(value);
		Refresh(false);
	}

	bool ColorSwatchButton::MSWOnDraw(WXDRAWITEMSTRUCT* item)
	{
		auto* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(item);
		if (!drawItem || !drawItem->hDC)
		{
			return false;
		}

		const bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
		const bool selected = GetValue() || pressed;
		wxColour background = GetParent()->GetBackgroundColour();
		if (!background.IsOk())
		{
			background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		}
		wil::unique_hbrush backgroundBrush{ ::CreateSolidBrush(RGB(
			background.Red(),
			background.Green(),
			background.Blue()
		)) };
		::FillRect(drawItem->hDC, &drawItem->rcItem, backgroundBrush.get());

		const wxBitmap& bitmap = selected ? m_selectedBitmap : m_normalBitmap;
		if (bitmap.IsOk())
		{
			const int width = drawItem->rcItem.right - drawItem->rcItem.left;
			const int height = drawItem->rcItem.bottom - drawItem->rcItem.top;
			const int x = drawItem->rcItem.left + (width - bitmap.GetWidth()) / 2;
			const int y = drawItem->rcItem.top + (height - bitmap.GetHeight()) / 2;
			{
				wxDCTemp dc(
					reinterpret_cast<WXHDC>(drawItem->hDC),
					wxSize(
						drawItem->rcItem.right - drawItem->rcItem.left,
						drawItem->rcItem.bottom - drawItem->rcItem.top
					)
				);
				dc.DrawBitmap(bitmap, x, y, true);
			}
		}

		if (
			(drawItem->itemState & ODS_FOCUS) != 0
			&& (drawItem->itemState & ODS_NOFOCUSRECT) == 0
		)
		{
			RECT focusRect = drawItem->rcItem;
			::InflateRect(&focusRect, -FromDIP(selected ? 4 : 2), -FromDIP(selected ? 4 : 2));
			::DrawFocusRect(drawItem->hDC, &focusRect);
		}

		return true;
	}

	void ColorSwatchButton::RebuildBitmap()
	{
		const wxSize bitmapSize = FromDIP(wxSize(SwatchSizeDip, SwatchSizeDip));
		const wxColour color(
			(m_argb >> 16) & 0xFF,
			(m_argb >> 8) & 0xFF,
			m_argb & 0xFF
		);
		const wxColour light = color.ChangeLightness(175);
		const wxColour dark = color.ChangeLightness(95);
		wxGraphicsGradientStops gradient(light, dark);

		auto createBitmap = [&](const wxColour& border, int borderWidth) {
			wxBitmap bitmap(bitmapSize.GetWidth(), bitmapSize.GetHeight(), 32);
			bitmap.UseAlpha();

			wxMemoryDC memoryDc(bitmap);
			memoryDc.SetBackground(wxBrush(wxColour(0, 0, 0, 0)));
			memoryDc.Clear();

			{
				std::unique_ptr<wxGraphicsContext> context{ wxGraphicsContext::Create(memoryDc) };
				if (context)
				{
					context->SetPen(*wxTRANSPARENT_PEN);
					context->SetBrush(wxBrush(border));
					context->DrawRectangle(0, 0, bitmapSize.GetWidth(), bitmapSize.GetHeight());

					const wxRect2DDouble bounds(borderWidth, borderWidth,
						bitmapSize.GetWidth() - 2 * borderWidth, bitmapSize.GetHeight() - 2 * borderWidth);
					if (m_automatic)
					{
						DrawAutomaticColorSwatch(*context, bounds);
					}
					else
					{
						context->SetBrush(context->CreateLinearGradientBrush(0, 0,
							bitmapSize.GetWidth(), bitmapSize.GetHeight(), gradient));
						context->DrawRectangle(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height);
					}
				}
			}

			memoryDc.SelectObject(wxNullBitmap);
			return bitmap;
		};

		m_normalBitmap = createBitmap(GetSurfaceBorder(), std::max(1, FromDIP(1)));
		m_selectedBitmap = createBitmap(GetSelectionBorder(), std::max(2, FromDIP(2)));
		SetBitmap(m_normalBitmap);
	}

	void ColorSwatchButton::OnDpiChanged(wxDPIChangedEvent& event)
	{
		SetBitmapMargins(0, 0);
		const wxSize buttonSize = FromDIP(wxSize(SwatchSizeDip, SwatchSizeDip));
		SetMinSize(buttonSize);
		SetMaxSize(buttonSize);
		RebuildBitmap();
		InvalidateBestSize();
		GetParent()->Layout();
		event.Skip();
	}

	void ColorSwatchButton::OnSystemColorChanged(wxSysColourChangedEvent& event)
	{
		RebuildBitmap();
		Refresh(false);
		event.Skip();
	}
}
