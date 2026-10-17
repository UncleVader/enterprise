////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : visual page window 
////////////////////////////////////////////////////////////////////////////

#include "pageWindow.h"
#include "frontend/visualView/ctrl/frame.h"   // wxDefaultStypeBGColour — the form surface

wxIMPLEMENT_DYNAMIC_CLASS(ibPanelPage, wxPanel);

ibPanelPage::ibPanelPage() :
	wxPanel(), m_boxSizer(nullptr)
{
}

ibPanelPage::ibPanelPage(wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxString& name) :
	wxPanel(parent, winid, pos, size, style, name), m_boxSizer(nullptr)
{
	m_boxSizer = new wxBoxSizer(wxHORIZONTAL);	
	SetSizer(m_boxSizer);

	// ⭐ A PAGE IS THE FORM'S SURFACE, so it wears the form's colour. Every control's background defaults to
	// the cream (wxDefaultStypeBGColour) and the form window is cream too, but a page is a plain wxPanel and
	// came up in the system grey: each label then sat on its own cream strip over a grey page, which is
	// what made a form look like a stack of stickers. A page that is not cream is a page a designer
	// deliberately gave another colour, and none can — a page has no colour property — so this is the default.
	SetBackgroundColour(wxDefaultStypeBGColour);
}

ibPanelPage::~ibPanelPage()
{
	SetSizer(nullptr);
}