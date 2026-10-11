// ExchangePlan is ПланОбмена. Without the kind, a configuration cannot hold one,
// and ExchangePlans is "var is not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaExchangePlanObject.h"
#include "backend/moduleManager/globalContextManager.h"
#include "backend/system/value/valueArray.h"

namespace {

struct OpenPlanFile : ibMetaDataConfigurationFile {
	ibMetaData::LoadGuard m_open{ this };
};

ibValue Plans(ibMetaData& cfg)
{
	ibValueGlobalContextManager context(&cfg);
	ibValue plans;
	EXPECT_TRUE(context.GetPropVal(context.FindProp(wxT("ExchangePlans")), plans));
	return plans;
}

wxString CodeOf(ibValue& node)
{
	ibValue code;
	EXPECT_TRUE(node.GetPropVal(node.FindProp(wxT("Code")), code));
	return wxString(code.GetString());
}

} // namespace

TEST(ExchangePlan, TheConfigurationAcceptsOneAndACatalogDoesNot) {
	ibMetaDataConfigurationFile cfg;
	ibValueMetaObjectConfiguration* const root = cfg.GetCommonMetaObject();
	ASSERT_NE(root, nullptr);
	ibValueMetaObject* const plan = cfg.CreateMetaObject(g_metaExchangePlanCLSID, root, false);
	ASSERT_NE(plan, nullptr);
	EXPECT_EQ(plan->GetClassType(), g_metaExchangePlanCLSID);
	EXPECT_EQ(cfg.GetAnyArrayObject(g_metaExchangePlanCLSID).size(), 1u);

	ibValueMetaObject* const catalog = cfg.CreateMetaObject(g_metaCatalogCLSID, root, false);
	ASSERT_NE(catalog, nullptr);
	EXPECT_EQ(cfg.CreateMetaObject(g_metaExchangePlanCLSID, catalog, false), nullptr);
}

TEST(ExchangePlan, TheManagerIsEmptyAndTheMasterNodeIsRemembered) {
	OpenPlanFile cfg;
	ibValueMetaObjectConfiguration* const root = cfg.GetCommonMetaObject();
	auto* const plan = dynamic_cast<ibValueMetaObjectExchangePlan*>(
		cfg.CreateMetaObject(g_metaExchangePlanCLSID, root, true));
	ASSERT_NE(plan, nullptr);
	plan->SetName(wxT("Rib"));

	ibValue plans = Plans(cfg);
	ibValue manager;
	ASSERT_TRUE(plans.GetAt(ibValue(wxT("Rib")), manager));

	ibValue node;
	ASSERT_TRUE(manager.CallAsFunc(manager.FindMethod(wxT("ThisNode")), node, nullptr, 0));
	EXPECT_TRUE(node.IsEmpty());
	ibValue emptyRef;
	ASSERT_TRUE(manager.CallAsFunc(manager.FindMethod(wxT("EmptyRef")), emptyRef, nullptr, 0));
	EXPECT_TRUE(emptyRef.IsEmpty());

	ibValue code(wxT("SHOP"));
	ibValue* findArgs[] = { &code };
	ibValue found;
	ASSERT_TRUE(manager.CallAsFunc(manager.FindMethod(wxT("FindByCode")), found, findArgs, 1));
	EXPECT_TRUE(found.IsEmpty());

	ibValue selected;
	ASSERT_TRUE(manager.CallAsFunc(manager.FindMethod(wxT("Select")), selected, nullptr, 0));
	const ibValueArray* const rows = dynamic_cast<const ibValueArray*>(selected.GetRef());
	ASSERT_NE(rows, nullptr);
	EXPECT_EQ(rows->Count(), 0u);

	ibValue master;
	ASSERT_TRUE(plans.CallAsFunc(plans.FindMethod(wxT("MasterNode")), master, nullptr, 0));
	EXPECT_EQ(master.GetType(), ibValueTypes::TYPE_EMPTY);

	ibValue* setArgs[] = { &code };
	ASSERT_TRUE(plans.CallAsProc(plans.FindMethod(wxT("SetMasterNode")), setArgs, 1));
	ibValue again = Plans(cfg);
	ibValue remembered;
	ASSERT_TRUE(again.CallAsFunc(again.FindMethod(wxT("MasterNode")), remembered, nullptr, 0));
	EXPECT_FALSE(remembered.IsEmpty());
	EXPECT_EQ(CodeOf(remembered), wxT("SHOP"));

	ibValue cleared;
	ibValue* clearArgs[] = { &cleared };
	ASSERT_TRUE(again.CallAsProc(again.FindMethod(wxT("SetMasterNode")), clearArgs, 1));
	ibValue gone;
	ASSERT_TRUE(again.CallAsFunc(again.FindMethod(wxT("MasterNode")), gone, nullptr, 0));
	EXPECT_EQ(gone.GetType(), ibValueTypes::TYPE_EMPTY);
}

TEST(ExchangePlan, TheNameCompiles) {
	ibValue global = ibValue::CreateObject(wxT("GlobalContextManager"));
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	cc.AddContextVariable(wxT("GlobalContextManager"), &global, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var node;\n")
			wxT("    node = ExchangePlans.MasterNode();\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
