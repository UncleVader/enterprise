#ifndef __PASTE_SLOT_H__
#define __PASTE_SLOT_H__

// form_add wraps a new control in a SizerItem when its parent is the form,
// a sizer, a sizer item, or a notebook page, and then gives that slot the
// layout flags for the child's kind (a container expands). form_paste used
// to drop the control in bare, so a table pasted onto a form had no slot
// and rendered at its natural width. This is the same decision, with no
// form object behind it, so the unit test can drive it.

#include <wx/string.h>

#include "formdefs.h"

inline bool ibPastedControlNeedsSizerItem(
	int parentComponent,
	const wxString& parentTypeName,
	const wxString& parentClassName,
	const wxString& childClassName)
{
	// The copied object already is the slot. Wrapping it again would nest
	// a sizer item inside a sizer item and throw away the flags it carried.
	if (childClassName.CmpNoCase(wxT("SizerItem")) == 0)
		return false;

	// A notebook page is not a sizer, and form_add still gives its child a slot.
	if (parentTypeName.CmpNoCase(wxT("NotebookPage")) == 0)
		return true;

	// These parents accept a fixed kind of child and do not wrap it.
	if (parentTypeName.CmpNoCase(wxT("Tool")) == 0
		|| parentTypeName.CmpNoCase(wxT("Toolbar")) == 0
		|| parentTypeName.CmpNoCase(wxT("Notebook")) == 0
		|| parentClassName.CmpNoCase(wxT("Tablebox")) == 0
		|| parentClassName.CmpNoCase(wxT("TableboxColumnGroup")) == 0)
		return false;

	return parentComponent == COMPONENT_TYPE_FRAME
		|| parentComponent == COMPONENT_TYPE_SIZER
		|| parentComponent == COMPONENT_TYPE_SIZERITEM;
}

#endif
