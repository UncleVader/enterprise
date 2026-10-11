#include "valueTextDocument.h"

#include "backend/backend_exception.h"

// Order MUST match ibValueTextDocument::Func.
void ibValueTextDocument_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("TextDocument()"));

	helper.AppendFunc(wxT("GetText"), wxT("GetText()"));
	helper.AppendFunc(wxT("SetText"), 1, wxT("SetText(text : string)"));
	helper.AppendFunc(wxT("AddLine"), 1, wxT("AddLine(line : string)"));
	helper.AppendFunc(wxT("InsertLine"), 2, wxT("InsertLine(number : number, line : string)"));
	helper.AppendFunc(wxT("DeleteLine"), 1, wxT("DeleteLine(number : number)"));
	helper.AppendFunc(wxT("ReplaceLine"), 2, wxT("ReplaceLine(number : number, line : string)"));
	helper.AppendFunc(wxT("GetLine"), 1, wxT("GetLine(number : number)"));
	helper.AppendFunc(wxT("LineCount"), wxT("LineCount()"));
	helper.AppendFunc(wxT("Clear"), wxT("Clear()"));
}

bool ibValueTextDocument::Init(ibValue** /*paParams*/, const long lSizeArray)
{
	return lSizeArray == 0;
}

void ibValueTextDocument::SetText(const wxString& text)
{
	m_lines.clear();
	wxString norm = text;
	norm.Replace(wxT("\r\n"), wxT("\n"), true);
	norm.Replace(wxT("\r"), wxT("\n"), true);
	// A break at the end ENDS the last line. It does not open an empty one after it,
	// the same cut TextReader uses for a file.
	for (size_t start = 0; start < norm.length(); ) {
		const size_t brk = norm.find(wxT('\n'), start);
		if (brk == wxString::npos) {
			m_lines.push_back(norm.Mid(start));
			break;
		}
		m_lines.push_back(norm.Mid(start, brk - start));
		start = brk + 1;
		if (start == norm.length())
			break;
	}
}

wxString ibValueTextDocument::GetText() const
{
	wxString text;
	for (size_t i = 0; i < m_lines.size(); ++i) {
		if (i > 0)
			text += wxT('\n');
		text += m_lines[i];
	}
	return text;
}

void ibValueTextDocument::RequireLine(long long number) const
{
	if (number < 1 || number > LineCount())
		ibBackendCoreException::Error(
			_("TextDocument: line %lld is outside 1..%lld"),
			number, LineCount());
}

void ibValueTextDocument::AddLine(const wxString& line)
{
	m_lines.push_back(line);
}

void ibValueTextDocument::InsertLine(long long number, const wxString& line)
{
	// One past the end appends. Anywhere further, or before the first line, is a refusal.
	if (number < 1 || number > LineCount() + 1)
		ibBackendCoreException::Error(
			_("TextDocument: line %lld is outside 1..%lld"),
			number, LineCount() + 1);
	m_lines.insert(m_lines.begin() + static_cast<size_t>(number - 1), line);
}

void ibValueTextDocument::DeleteLine(long long number)
{
	RequireLine(number);
	m_lines.erase(m_lines.begin() + static_cast<size_t>(number - 1));
}

void ibValueTextDocument::ReplaceLine(long long number, const wxString& line)
{
	RequireLine(number);
	m_lines[static_cast<size_t>(number - 1)] = line;
}

wxString ibValueTextDocument::GetLine(long long number) const
{
	RequireLine(number);
	return m_lines[static_cast<size_t>(number - 1)];
}

bool ibValueTextDocument::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
{
	switch (lMethodNum) {
	case enGetText:
		pvarRetValue = GetText();
		return true;
	case enSetText:
		if (lSizeArray < 1)
			return false;
		SetText(paParams[0]->GetString());
		return true;
	case enAddLine: {
		wxString line;
		if (lSizeArray > 0 && paParams[0] != nullptr)
			line = paParams[0]->GetString();
		AddLine(line);
		return true;
	}
	case enInsertLine:
		if (lSizeArray < 2)
			return false;
		InsertLine(paParams[0]->GetInteger(), paParams[1]->GetString());
		return true;
	case enDeleteLine:
		if (lSizeArray < 1)
			return false;
		DeleteLine(paParams[0]->GetInteger());
		return true;
	case enReplaceLine:
		if (lSizeArray < 2)
			return false;
		ReplaceLine(paParams[0]->GetInteger(), paParams[1]->GetString());
		return true;
	case enGetLine:
		if (lSizeArray < 1)
			return false;
		pvarRetValue = GetLine(paParams[0]->GetInteger());
		return true;
	case enLineCount:
		pvarRetValue = ibNumber(LineCount());
		return true;
	case enClear:
		Clear();
		return true;
	}
	return false;
}

VALUE_TYPE_REGISTER(ibValueTextDocument, "TextDocument", value_to_clsid("VL_TDOC"));
