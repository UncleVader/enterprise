#ifndef __VALUE_TEXT_DOCUMENT_H__
#define __VALUE_TEXT_DOCUMENT_H__

#include "backend/compiler/value.h"

#include <vector>

// TextDocument is ТекстовыйДокумент: lines of text held in memory. New TextDocument()
// is the constructor a port already writes. Lines are counted from 1.

void ibValueTextDocument_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueTextDocument : public ibValueStaticMembers<&ibValueTextDocument_BindNames> {
	enum Func {
		enGetText,
		enSetText,
		enAddLine,
		enInsertLine,
		enDeleteLine,
		enReplaceLine,
		enGetLine,
		enLineCount,
		enClear,
	};
public:
	ibValueTextDocument() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE) {}

	virtual bool Init(ibValue** paParams, const long lSizeArray);   // no arguments: New TextDocument()

	virtual bool IsEmpty() const { return m_lines.empty(); }
	virtual ibString GetString() const { return GetText(); }

	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray);

	void SetText(const wxString& text);
	wxString GetText() const;
	void AddLine(const wxString& line);
	void InsertLine(long long number, const wxString& line);
	void DeleteLine(long long number);
	void ReplaceLine(long long number, const wxString& line);
	wxString GetLine(long long number) const;
	long long LineCount() const { return static_cast<long long>(m_lines.size()); }
	void Clear() { m_lines.clear(); }

private:
	void RequireLine(long long number) const;

	std::vector<wxString> m_lines;
};

#endif
