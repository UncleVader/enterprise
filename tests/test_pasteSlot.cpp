#include <wx/string.h>

#include <gtest/gtest.h>

#include "frontend/visualView/pasteSlot.h"

TEST(PasteSlot, FormContainerGetsASizerItem)
{
	EXPECT_TRUE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_FRAME, wxT("Form"), wxT("Form"), wxT("Tablebox")));
}

TEST(PasteSlot, SizerAndSizerItemWrapToo)
{
	EXPECT_TRUE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_SIZER, wxT("Sizer"), wxT("BoxSizer"), wxT("Textctrl")));
	EXPECT_TRUE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_SIZERITEM, wxT("SizerItem"), wxT("SizerItem"), wxT("Statictext")));
}

TEST(PasteSlot, NotebookPageWrapsAndTheNotebookItselfDoesNot)
{
	EXPECT_TRUE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_WINDOW, wxT("NotebookPage"), wxT("NotebookPage"), wxT("Tablebox")));
	EXPECT_FALSE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_WINDOW, wxT("Notebook"), wxT("Notebook"), wxT("NotebookPage")));
}

TEST(PasteSlot, PastingASizerItemDoesNotNestAnother)
{
	EXPECT_FALSE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_FRAME, wxT("Form"), wxT("Form"), wxT("SizerItem")));
	EXPECT_FALSE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_FRAME, wxT("Form"), wxT("Form"), wxT("sizeritem")));
}

TEST(PasteSlot, TableAndToolbarTakeTheirOwnChildrenBare)
{
	EXPECT_FALSE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_WINDOW, wxT("Container"), wxT("Tablebox"), wxT("TableboxColumn")));
	EXPECT_FALSE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_WINDOW, wxT("Container"), wxT("TableboxColumnGroup"), wxT("TableboxColumn")));
	EXPECT_FALSE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_WINDOW, wxT("Toolbar"), wxT("Toolbar"), wxT("Tool")));
	EXPECT_FALSE(ibPastedControlNeedsSizerItem(
		COMPONENT_TYPE_WINDOW, wxT("Tool"), wxT("Tool"), wxT("Menu")));
}
