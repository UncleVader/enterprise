// A selection reads the same attributes the object does, including the predefined ones.
//
// Date, Number and a characteristic's Type are not custom attributes. A selection that listed
// only the custom ones had no such member, and the runtime then said the name was a global
// function — Date() and Type() — instead of reading the field.
//
// The selector's constructor asks whether the designer is running, so this needs the application
// data a real host always has.

#include <gtest/gtest.h>

#include "backend/appData.h"
#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/partial/catalog.h"
#include "backend/metaCollection/partial/chartOfCharacteristicTypes.h"
#include "backend/metaCollection/partial/document.h"
#include "backend/metaCollection/partial/selector/objectSelector.h"

#include <wx/image.h>
#include <wx/init.h>
#include <wx/log.h>

namespace {

struct SelectionMembers : ::testing::Test {
	wxInitializer m_wxInit;
	ibMetaDataConfigurationFile m_cfg;

	void SetUp() override {
		if (!m_wxInit.IsOk())
			GTEST_SKIP() << "wxBase init failed";
		wxInitAllImageHandlers();
		if (wxLog::GetActiveTarget() != nullptr)
			delete wxLog::SetActiveTarget(new wxLogStderr());
		if (ibApplicationInstance::Get() == nullptr
			&& !ibApplicationInstance::CreateAppDataEnv(ibRunMode::eFILE_MODE))
			GTEST_SKIP() << "appData env unavailable headless";
	}

	ibValue SelectionOf(ibClassID clsid) {
		ibValueMetaObject* const created = m_cfg.CreateMetaObject(
			clsid, m_cfg.GetCommonMetaObject(), /*runObject*/ false);
		auto* const meta = dynamic_cast<ibValueMetaObjectRecordDataMutableRef*>(created);
		EXPECT_NE(meta, nullptr);
		if (meta == nullptr)
			return ibValue();
		return new ibValueSelectorRecordDataObject(meta);
	}
};

} // namespace

TEST_F(SelectionMembers, ADocumentSelectionHasItsDateAndNumber)
{
	const ibValue selection = SelectionOf(g_metaDocumentCLSID);
	ASSERT_GE(selection.FindProp(wxT("Date")), 0);
	ASSERT_GE(selection.FindProp(wxT("Number")), 0);
	ASSERT_GE(selection.FindProp(wxT("Posted")), 0);
	EXPECT_GE(selection.FindMethod(wxT("Next")), 0);
	EXPECT_LT(selection.FindMethod(wxT("Date")), 0);
}

TEST_F(SelectionMembers, ACharacteristicSelectionHasItsType)
{
	const ibValue selection = SelectionOf(g_metaChartOfCharacteristicTypesCLSID);
	ASSERT_GE(selection.FindProp(wxT("Type")), 0);
	ASSERT_GE(selection.FindProp(wxT("Code")), 0);
	EXPECT_LT(selection.FindMethod(wxT("Type")), 0);
}

TEST_F(SelectionMembers, ACatalogSelectionHasItsCode)
{
	const ibValue selection = SelectionOf(g_metaCatalogCLSID);
	ASSERT_GE(selection.FindProp(wxT("Code")), 0);
	ASSERT_GE(selection.FindProp(wxT("Description")), 0);
}
