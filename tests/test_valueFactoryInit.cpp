// =============================================================================
// The value factory and an Init that REFUSES BY RAISING - which is how a type says why it refuses. The object
// used to be held in a raw pointer until Init returned: deleted when Init answered false, leaked when Init
// raised. A new value is now born owned (the factory answers with its holder), so a refusal either way lets it go.
// =============================================================================

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/systemManager.h"
#include "backend/system/value/valueNotifyDescription.h"

namespace {

int g_alive = 0;

// Registered for this suite only: it counts itself, and refuses any argument by raising.
class ibValueTestRefusingInit : public ibValue {
public:
	ibValueTestRefusingInit() : ibValue(ibValueTypes::TYPE_VALUE, true) { ++g_alive; }
	virtual ~ibValueTestRefusingInit() { --g_alive; }

	virtual bool Init() { return true; }
	virtual bool Init(ibValue** paParams, const long) {
		if (paParams[0]->GetBoolean())
			ibBackendCoreException::Error(wxT("refused, and this is why"));
		return false;   // the quiet refusal, which the factory has always cleaned up after
	}
};

} // namespace

VALUE_TYPE_REGISTER(ibValueTestRefusingInit, "TestRefusingInit", value_to_clsid("VL_TRIN"));

TEST(ValueFactory, AnInitThatRaisesDoesNotLeakTheObject) {
	ASSERT_EQ(g_alive, 0);
	ibValue raise(true);
	ibValue* params[] = { &raise };
	try {
		ibValue::CreateObject(wxT("TestRefusingInit"), params, 1);
		FAIL() << "the refusal must reach the caller";
	}
	catch (const ibBackendException& e) {
		EXPECT_NE(std::string(e.what()).find("this is why"), std::string::npos) << "the type's own words, not a generic sentence";
	}
	EXPECT_EQ(g_alive, 0) << "an object whose Init raised is nobody's - the factory lets go of it";
}

namespace {

int g_notifyCalls = 0;
wxString g_notifyResult;
wxString g_notifyExtra;

void BindNotifyProbe(ibValue::ibMemberTable& helper, const ibValue*)
{
	helper.AppendProc(wxT("Handler"), 2, wxT("Handler(result, extra)"));
}

class ibValueNotifyProbe : public ibValueStaticMembers<&BindNotifyProbe> {
public:
	ibValueNotifyProbe() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE) {}
	bool CallAsProc(const long, ibValue** paParams, const long lSizeArray) override {
		++g_notifyCalls;
		if (lSizeArray > 0 && paParams != nullptr && paParams[0] != nullptr)
			g_notifyResult = paParams[0]->GetString();
		if (lSizeArray > 1 && paParams[1] != nullptr)
			g_notifyExtra = paParams[1]->GetString();
		return true;
	}
};

} // namespace

VALUE_TYPE_REGISTER(ibValueNotifyProbe, "NotifyProbe", value_to_clsid("VL_NTPR"));

TEST(NotifyDescription, ACallReachesTheNamedProcedure) {
	g_notifyCalls = 0;
	ibValue module = ibValue::CreateObject(wxT("NotifyProbe"));
	ibValue procedure(wxT("Handler"));
	ibValue extra(wxT("more"));
	ibValue* params[] = { &procedure, &module, &extra };
	ibValue notify = ibValue::CreateObject(wxT("NotifyDescription"), params, 3);

	ibValueNotifyDescription* asNotify = nullptr;
	ASSERT_TRUE(notify.ConvertToValue(asNotify));
	asNotify->Call(ibValue(wxT("Yes")));
	EXPECT_EQ(g_notifyCalls, 1);
	EXPECT_EQ(g_notifyResult, wxT("Yes"));
	EXPECT_EQ(g_notifyExtra, wxT("more"));
}

TEST(NotifyDescription, AMissingModuleAndAnUnknownProcedureAreNamed) {
	ibValue procedure(wxT("Handler"));
	ibValue* params[] = { &procedure };
	ibValue notify = ibValue::CreateObject(wxT("NotifyDescription"), params, 1);
	ibValueNotifyDescription* asNotify = nullptr;
	ASSERT_TRUE(notify.ConvertToValue(asNotify));
	try {
		asNotify->Call(ibValue(wxT("Yes")));
		FAIL() << "a description with no module must be refused";
	} catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("module")));
	}

	ibValue module = ibValue::CreateObject(wxT("NotifyProbe"));
	ibValue missing(wxT("Missing"));
	ibValue* bad[] = { &missing, &module };
	ibValue unknown = ibValue::CreateObject(wxT("NotifyDescription"), bad, 2);
	ASSERT_TRUE(unknown.ConvertToValue(asNotify));
	try {
		asNotify->Call(ibValue());
		FAIL() << "an unknown procedure must be refused";
	} catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("Missing")));
	}
}

TEST(NotifyDescription, TheNameAndQuestionCompile) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	ibValueSystemFunction valueSystem;
	cc.AddContextVariable(wxT("System"), &valueSystem, true);
	const wxString src =
		wxT("Procedure Check()\n")
		wxT("    var notify;\n")
		wxT("    notify = New NotifyDescription(\"Handler\", Undefined, 1);\n")
		wxT("    Question(\"Save?\", QuestionMode.Ok, notify);\n")
		wxT("EndProcedure\n");
	try {
		ASSERT_TRUE(cc.Compile(src));
	} catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}

TEST(ValueFactory, AnInitThatAnswersFalseIsStillCleanedUpAfter) {
	ASSERT_EQ(g_alive, 0);
	ibValue quiet(false);
	ibValue* params[] = { &quiet };
	EXPECT_THROW(ibValue::CreateObject(wxT("TestRefusingInit"), params, 1), ibBackendException);
	EXPECT_EQ(g_alive, 0);
}
