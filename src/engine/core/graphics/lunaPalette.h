#ifndef _CORE_LUNA_PALETTE_H__
#define _CORE_LUNA_PALETTE_H__

////////////////////////////////////////////////////////////////////////////
// ibLunaPalette — the Luna theme's colours, as DATA (core has no wx GUI).
//
// Both renderers of the Luna theme read the SAME file: the desktop theme
// (frontend/uikit/theme/lunaTheme.cpp, wxColour) and the web client's CSS
// (frontend/web/webClient.cpp, "#RRGGBB" text injected into client.html).
// Living here, under core, is what lets both reach it without either
// depending on the other — core sits under frontend AND under wfrontend.
//
// The JSON file (theme/lunaPalette.json, next to the executable — same
// lookup convention as lang/) is the single source of truth; the constants
// below are only the fallback when that file is absent or malformed, so a
// missing/broken resource never blocks startup or half-applies a theme.
////////////////////////////////////////////////////////////////////////////

#include "core/core.h"
#include "core/graphics/colour.h"

#include <wx/string.h>

struct CORE_API ibLunaPalette {
	ibColour chrome       { 0xB8, 0xC9, 0xD4 };   // #B8C9D4
	ibColour chromeLight  { 0xC8, 0xD6, 0xDF };   // #C8D6DF
	ibColour content      { 0xFA, 0xF7, 0xF0 };   // #FAF7F0
	ibColour border       { 0xA8, 0xBA, 0xC8 };   // #A8BAC8
	ibColour captionTop   { 0x5A, 0x7B, 0x95 };   // #5A7B95
	ibColour captionBottom{ 0x3F, 0x5C, 0x77 };   // #3F5C77
	ibColour accent       { 0xD9, 0x77, 0x57 };   // #D97757
	ibColour accentDeep   { 0xB8, 0x5A, 0x38 };   // #B85A38

	// theme/lunaPalette.json next to the executable, or an empty path if none is found.
	static wxString FilePath();

	// Starts from the hardcoded defaults above and overwrites each colour the file
	// names ("#RRGGBB" strings, same 8 keys as the members); a key that is absent,
	// and the whole file if it is absent or malformed, leaves the matching default
	// untouched. Always succeeds in the sense that `out` is always a usable palette.
	static void Load(const wxString& path, ibLunaPalette& out);
};

#endif
