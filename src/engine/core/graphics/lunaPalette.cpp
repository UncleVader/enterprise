#include "core/graphics/lunaPalette.h"

#include "core/serialize/jsonProvider.h"
#include "core/fileSystem/fs.h"
#include "core/programFolder.h"
#include "core/exception.h"

#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/log.h>

namespace {

// Same leaf the file keeps in the source tree, so the release layout and a
// dev build (walked up to, below) agree on a name.
const wxChar* const kFileName = wxT("lunaPalette.json");

// "#RRGGBB" only — the one shape the palette file (and the web client's own
// defaults) ever writes. Anything else is left alone by the caller.
bool ParseHexColour(const wxString& hex, ibColour& out) {
	wxString s = hex;
	if (!s.IsEmpty() && s[0] == wxT('#'))
		s = s.Mid(1);
	if (s.length() != 6)
		return false;

	long value = 0;
	if (!s.ToLong(&value, 16))
		return false;

	out = ibColour(static_cast<unsigned char>((value >> 16) & 0xFF),
	               static_cast<unsigned char>((value >> 8) & 0xFF),
	               static_cast<unsigned char>(value & 0xFF));
	return true;
}

} // namespace

// Where the palette is, in the order a running binary should look — the same
// two steps webClient.cpp's LoadClient() uses to find the web client itself:
//   1. <program folder>/theme/lunaPalette.json — the release layout, next to
//      the executable like lang/ and plugins/ (programFolder.h).
//   2. The development tree: walk up from the executable for
//      src/engine/frontend/uikit/theme/lunaPalette.json, bounded at six levels
//      for the same reason LoadClient() bounds its own walk — so a build run
//      straight out of the source checkout reads the file being edited, no
//      install step needed.
wxString ibLunaPalette::FilePath() {
	const wxString programFolder = ibProgramFolder();
	if (!programFolder.IsEmpty()) {
		const wxString besidePath = programFolder + wxFILE_SEP_PATH + wxT("theme")
			+ wxFILE_SEP_PATH + kFileName;
		if (wxFileName::FileExists(besidePath))
			return besidePath;
	}

	const wxString exeDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
	wxFileName walk(exeDir, wxEmptyString);
	for (int i = 0; i < 6; ++i) {
		const wxString candidate = walk.GetPath() + wxFILE_SEP_PATH
			+ wxT("src") + wxFILE_SEP_PATH + wxT("engine") + wxFILE_SEP_PATH
			+ wxT("frontend") + wxFILE_SEP_PATH + wxT("uikit") + wxFILE_SEP_PATH
			+ wxT("theme") + wxFILE_SEP_PATH + kFileName;
		if (wxFileName::FileExists(candidate))
			return candidate;
		if (walk.GetDirCount() == 0)
			break;
		walk.RemoveLastDir();
	}

	return wxString();
}

void ibLunaPalette::Load(const wxString& path, ibLunaPalette& out) {
	if (path.IsEmpty() || !wxFileExists(path))
		return;

	wxFile file(path);
	if (!file.IsOpened())
		return;

	const wxFileOffset len = file.Length();
	if (len <= 0)
		return;

	wxMemoryBuffer buf(static_cast<size_t>(len));
	void* writeBuf = buf.GetWriteBuf(static_cast<size_t>(len));
	const ssize_t readBytes = file.Read(writeBuf, static_cast<size_t>(len));
	buf.UngetWriteBuf(readBytes > 0 ? static_cast<size_t>(readBytes) : 0);
	if (readBytes != len)
		return;

	try {
		ibReaderMemory reader(buf);
		ibDataNode root;
		ibJsonProvider provider;
		if (!provider.Read(reader, root))
			return;

		auto applyIfPresent = [&root](const wxString& key, ibColour& target) {
			const wxString hex = root.GetValue<wxString>(key);
			ibColour parsed;
			if (!hex.IsEmpty() && ParseHexColour(hex, parsed))
				target = parsed;
		};
		applyIfPresent(wxT("chrome"),        out.chrome);
		applyIfPresent(wxT("chromeLight"),   out.chromeLight);
		applyIfPresent(wxT("content"),       out.content);
		applyIfPresent(wxT("border"),        out.border);
		applyIfPresent(wxT("captionTop"),    out.captionTop);
		applyIfPresent(wxT("captionBottom"), out.captionBottom);
		applyIfPresent(wxT("accent"),        out.accent);
		applyIfPresent(wxT("accentDeep"),    out.accentDeep);
	}
	catch (const ibCoreException& err) {
		wxLogMessage(wxT("Luna palette: ignoring %s (%s)"), path, err.GetErrorDescription());
	}
}
