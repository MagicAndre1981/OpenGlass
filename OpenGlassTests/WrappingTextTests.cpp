#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/wrapsizer.h>
#include <source_location>
#include "../OpenGlassGUI/WrappingLayout.hpp"

using namespace OpenGlass;

wxIMPLEMENT_APP_NO_MAIN(wxApp);

int TestWrappingTextLayout()
{
	int failures{};
	const auto Check = [&failures](bool condition, const std::source_location& location = std::source_location::current())
	{
		if (!condition)
		{
			++failures;
			fprintf(stderr, "Check failed at %s:%u\n", location.file_name(), location.line());
		}
	};
	wxInitializer initializer;
	Check(initializer.IsOk());
	if (!initializer.IsOk()) return 1;
	// Hidden layout fixture: no MainFrame, system settings, download or elevation.
	wxFrame frame(nullptr, wxID_ANY, L"Wrapping fixture");
	auto* page = new wxScrolledWindow(&frame);
	page->SetScrollRate(5, 5);
	auto* root = new wxBoxSizer(wxVERTICAL);
	const wxString message = L"If you experience a DWM crash, open a GitHub issue promptly with the exact Windows build and revision and a full dump. Reddit, Discord, and other third-party posts are not tracked as OpenGlass bug reports.";
	auto* description = CreateWrappingLabel(page, message);
	root->Add(description, 0, wxEXPAND | wxALL, 8);
	auto* notice = new wxPanel(page);
	wxString noticeText = message;
	auto* row = new wxBoxSizer(wxHORIZONTAL);
	row->Add(24, 16);
	auto* text = new wxBoxSizer(wxVERTICAL);
	auto* label = CreateWrappingLabel(notice, message);
	text->Add(label, 0, wxEXPAND | wxBOTTOM, 4);
	auto* link = new wxStaticText(notice, wxID_ANY, L"Open a GitHub issue");
	text->Add(link);
	row->Add(text, 1, wxEXPAND);
	notice->SetSizer(row);
	notice->Bind(wxEVT_SIZE, [label, &noticeText](wxSizeEvent& event)
	{
		WrapStaticTextToParentWidth(label, noticeText);
		event.Skip();
	});
	root->Add(notice, 0, wxEXPAND | wxALL, 8);
	page->SetSizer(root);
	int narrowHeight{};
	for (const int width : { 800, 360, 1100 })
	{
		page->SetSize(width, 600);
		page->Layout();
		WrapStaticTextToParentWidth(description, message);
		WrapStaticTextToParentWidth(label, message);
		page->Layout();
		page->FitInside();
		Check(root->GetMinSize().x <= page->GetClientSize().x);
		for (auto* item : { description, label })
		{
			wxClientDC dc(item);
			dc.SetFont(item->GetFont());
			const auto extent = dc.GetMultiLineTextExtent(item->GetLabelText());
			Check(extent.x <= item->GetClientSize().x);
			Check(extent.y <= item->GetClientSize().y);
			Check(item->GetLabelText().Contains(L"\n"));
			Check(item->GetSize().y >= item->GetBestSize().y);
			Check(item->GetPosition().x + item->GetSize().x <= item->GetParent()->GetClientSize().x);
		}
		Check(link->GetPosition().y >= label->GetPosition().y + label->GetSize().y);
		Check(link->GetPosition().y + link->GetSize().y <= notice->GetClientSize().y);
		if (width == 360) narrowHeight = label->GetSize().y;
		if (width == 1100) Check(label->GetSize().y < narrowHeight);
	}
	const auto longHeight = label->GetSize().y;
	noticeText = L"Ready.";
	WrapStaticTextToParentWidth(label, noticeText);
	page->Layout();
	page->FitInside();
	Check(label->GetSize().y < longHeight);
	noticeText = message;
	WrapStaticTextToParentWidth(label, noticeText);
	page->Layout();
	page->FitInside();
	Check(label->GetSize().y >= longHeight);
	// The colour page is laid out while hidden, before the notebook gets its final size.
	auto* colors = new wxScrolledWindow(&frame);
	colors->SetScrollRate(5, 5);
	auto* colorRoot = new wxBoxSizer(wxVERTICAL);
	auto* group = new wxStaticBoxSizer(wxVERTICAL, colors, L"Color presets");
	auto* presets = new wxWrapSizer(wxHORIZONTAL, wxREMOVE_LEADING_SPACES);
	for (int index = 0; index < 18; ++index)
	{
		auto* cell = new wxBoxSizer(wxVERTICAL);
		auto* swatch = new wxButton(colors, wxID_ANY, L"", wxDefaultPosition, wxSize(40, 40));
		swatch->SetMinSize(wxSize(40, 40));
		cell->Add(swatch, 0, wxALIGN_CENTER_HORIZONTAL);
		cell->Add(new wxStaticText(colors, wxID_ANY, L"Automatic", wxDefaultPosition, wxSize(64, -1), wxALIGN_CENTER_HORIZONTAL), 0, wxEXPAND | wxTOP, 2);
		presets->Add(cell, 0, wxALL, 1);
	}
	group->Add(presets, 0, wxEXPAND | wxALL, 2);
	colorRoot->Add(group, 0, wxEXPAND | wxALL, 2);
	auto* following = new wxStaticText(colors, wxID_ANY, L"Color intensity:");
	colorRoot->Add(following, 0, wxALL, 8);
	colors->SetSizer(colorRoot);
	colors->Layout();
	colors->FitInside();
	BindWrappingPageLayout(colors);
	for (const int width : { 880, 500, 1000 })
	{
		colors->SetSize(width, 600);
		for (auto* item : presets->GetChildren())
		{
			const auto rect = item->GetRect();
			Check(rect.GetRight() < colors->GetClientSize().x);
			Check(rect.GetBottom() < following->GetPosition().y);
		}
		Check(colors->GetVirtualSize().x <= colors->GetClientSize().x);
	}
	return failures;
}
