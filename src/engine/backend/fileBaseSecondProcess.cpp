#include "backend/fileBaseSecondProcess.h"

#include "backend/databaseLayer/firebird/firebirdBootstrap.h"

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

wxString ibFirebirdServerMode(const wxString& confText)
{
	wxString found;
	wxStringTokenizer lines(confText, wxT("\r\n"), wxTOKEN_STRTOK);
	while (lines.HasMoreTokens()) {
		wxString line = lines.GetNextToken();
		line.Trim().Trim(false);
		if (line.IsEmpty() || line.StartsWith(wxT("#")))
			continue;

		const int hash = line.Find(wxT('#'));
		if (hash != wxNOT_FOUND)
			line = line.Left(hash);
		line.Trim().Trim(false);

		wxString lower = line.Lower();
		if (!lower.StartsWith(wxT("servermode")))
			continue;
		const wxChar after = lower.length() > 10 ? lower[10] : wxT('\0');
		if (after != wxT(' ') && after != wxT('\t') && after != wxT('='))
			continue;

		const int eq = line.Find(wxT('='));
		if (eq == wxNOT_FOUND)
			continue;
		wxString value = line.Mid(eq + 1);
		value.Trim().Trim(false);
		if (!value.IsEmpty())
			found = value;
	}
	return found;
}

namespace {

bool ExclusiveServerMode(const wxString& serverMode)
{
	// SuperClassic starts with the same letters and is the sharing mode.
	// The comparison is the whole word.
	return serverMode.IsSameAs(wxT("Super"), false)
		|| serverMode.IsSameAs(wxT("ThreadedDedicated"), false);
}

} // namespace

wxString ibSecondProcessOnLocalFile(const wxString& filePath, bool heldStandalone,
	const wxString& serverMode)
{
	if (filePath.IsEmpty() || !heldStandalone || !ExclusiveServerMode(serverMode))
		return wxEmptyString;

	// The same test ibFirebirdLeaderMode uses to leave a path standalone.
	// A share is coordinated by leader mode, not by this check.
	if (filePath.StartsWith(wxT("\\\\")) || filePath.StartsWith(wxT("//")))
		return wxEmptyString;

	return wxT("This process holds this local file base and Firebird is in Super mode, which keeps the file to itself. "
		"A second process cannot open it. SuperClassic, which is what the shipped engine uses, shares a local file "
		"with the application.");
}

wxString ibConfiguredFirebirdServerMode()
{
	const wxString dir = ibFirebirdBootstrap::GetFbRuntimeDir();
	if (dir.IsEmpty())
		return wxEmptyString;

	wxFFile file(dir + wxFileName::GetPathSeparator() + wxT("firebird.conf"), wxT("rb"));
	if (!file.IsOpened())
		return wxEmptyString;

	wxString text;
	if (!file.ReadAll(&text))
		return wxEmptyString;
	return ibFirebirdServerMode(text);
}
