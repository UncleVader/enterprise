#include "backend/mcp/mcpSheetPath.h"

#include "backend/appData.h"

#include <wx/filename.h>

#if !defined(__WXMSW__)
#include <climits>
#include <cstdlib>
#endif

namespace {

wxString WithSlash(wxString path)
{
	const wxString sep = wxFileName::GetPathSeparator();
	if (!path.IsEmpty() && !path.EndsWith(sep))
		path += sep;
	return path;
}

wxString Canonical(const wxString& path)
{
	wxFileName name(path);
	name.Normalize(wxPATH_NORM_ENV_VARS | wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_ABSOLUTE);
	const wxString full = name.GetFullPath();

#if !defined(__WXMSW__)
	char buf[PATH_MAX];
	if (realpath(full.utf8_str(), buf) != nullptr)
		return wxString::FromUTF8(buf);

	// An export names a file that is not there yet. Resolve the parent, then
	// put the file name back, so a link in the parent cannot point outside.
	const wxString dir = name.GetPath();
	if (!dir.IsEmpty() && realpath(dir.utf8_str(), buf) != nullptr) {
		wxFileName joined(wxString::FromUTF8(buf), name.GetFullName());
		return joined.GetFullPath();
	}
#endif

	return full;
}

bool Inside(const wxString& file, const wxString& root)
{
	const wxString want = Canonical(file);
	const wxString gate = WithSlash(Canonical(root));
	if (want.IsEmpty() || gate.IsEmpty() || want.length() <= gate.length())
		return false;

#ifdef __WXMSW__
	return want.Lower().StartsWith(gate.Lower());
#else
	return want.StartsWith(gate);
#endif
}

} // namespace

wxString ibMcpSheetExchangeDir()
{
	if (appData == nullptr)
		return wxString();

	const wxString base = appData->GetFile();
	if (base.IsEmpty())
		return wxString();

	wxFileName dir = wxFileName::DirName(base);
	dir.AppendDir(wxT("exchange"));
	return dir.GetFullPath();
}

bool ibMcpSheetPathAllowed(const wxString& fileName, const wxString& exchangeDir, wxString& refusal)
{
	if (exchangeDir.IsEmpty()) {
		refusal = _("This base has no exchange folder, so a spreadsheet file path is refused.");
		return false;
	}

	if (fileName.IsEmpty()) {
		refusal = _("Say a file inside the base's exchange folder.");
		return false;
	}

	if (Inside(fileName, exchangeDir))
		return true;

	refusal = wxString::Format(
		_("'%s' is outside the exchange folder '%s'. Nothing was read or written."),
		fileName, exchangeDir);
	return false;
}
