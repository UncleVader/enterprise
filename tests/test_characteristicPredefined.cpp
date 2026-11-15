// =============================================================================
// `Predefined` on a chart of characteristic types, as a query names it.
//
// A query against a chart asks for Predefined — the item the configuration
// itself declared. What is stored is the predefined name. The two are the same
// column: an empty name is an ordinary item. An attribute the chart itself
// calls Predefined stays that attribute.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/partial/chartOfCharacteristicTypes.h"

namespace {

struct CharacteristicChart {
	ibMetaDataConfigurationFile cfg;
	ibValueMetaObjectChartOfCharacteristicTypes* chart = nullptr;

	CharacteristicChart() {
		ibValueMetaObjectConfiguration* root = cfg.GetCommonMetaObject();
		chart = dynamic_cast<ibValueMetaObjectChartOfCharacteristicTypes*>(
			cfg.CreateMetaObject(g_metaChartOfCharacteristicTypesCLSID, root, /*runObject*/ false));
	}
};

} // namespace

TEST(CharacteristicPredefined, AQueryNamesPredefined) {
	CharacteristicChart f;
	ASSERT_NE(f.chart, nullptr);
	const ibBackendQueryable* queryable = f.chart->GetQueryable();
	ASSERT_NE(queryable, nullptr);

	const ibBackendQueryColumn* predefined = queryable->ResolveColumnByName(wxT("Predefined"));
	const ibBackendQueryColumn* stored = queryable->ResolveColumnByName(wxT("PredefinedName"));
	ASSERT_NE(predefined, nullptr);
	EXPECT_EQ(predefined, stored);

	bool listed = false;
	for (const ibBackendQueryColumn* column : queryable->GetColumns())
		if (column != nullptr && column->GetName() == wxT("PredefinedName"))
			listed = true;
	EXPECT_TRUE(listed);
}

// The alias is only the name nobody declared. A column the chart calls Predefined
// is that column, and the predefined name stays beside it.
TEST(CharacteristicPredefined, AnAttributeNamedPredefinedIsThatAttribute) {
	CharacteristicChart f;
	ASSERT_NE(f.chart, nullptr);
	ibValueMetaObject* const attribute = f.cfg.CreateMetaObject(
		g_metaAttributeCLSID, f.chart, /*runObject*/ false);
	ASSERT_NE(attribute, nullptr);
	attribute->SetName(wxT("Predefined"));

	const ibBackendQueryable* queryable = f.chart->GetQueryable();
	ASSERT_NE(queryable, nullptr);
	const ibBackendQueryColumn* named = queryable->ResolveColumnByName(wxT("Predefined"));
	const ibBackendQueryColumn* stored = queryable->ResolveColumnByName(wxT("PredefinedName"));
	ASSERT_NE(named, nullptr);
	ASSERT_NE(stored, nullptr);
	EXPECT_EQ(named->GetName(), wxT("Predefined"));
	EXPECT_NE(named, stored);
}
