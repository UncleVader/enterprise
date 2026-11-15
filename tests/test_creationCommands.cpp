// =============================================================================
// AddCreationBasedOnCommands — the commands for creating an object on the basis
// of this one (ДобавитьКомандыСозданияНаОсновании).
//
// The list is the object's ListGeneration. The procedure adds one row per kind
// into the value table it is given: Presentation is what a person reads, Id is
// the metadata name. An empty list adds nothing. The catalog's own verbs stay
// where they were.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/partial/catalog.h"
#include "backend/metaCollection/partial/document.h"
#include "backend/objCtor.h"
#include "backend/system/value/valueTable.h"

namespace {

ibValue ManagerOf(ibValueMetaObjectGenericData* meta)
{
	ibCtorMetaValueTypeManager door(meta);
	return door.CreateObject();
}

} // namespace

TEST(CreationCommands, TheGenerationListBecomesRows) {
	ibMetaDataConfigurationFile cfg;
	ibValueMetaObjectConfiguration* root = cfg.GetCommonMetaObject();
	ASSERT_NE(root, nullptr);
	auto* catalog = dynamic_cast<ibValueMetaObjectCatalog*>(
		cfg.CreateMetaObject(g_metaCatalogCLSID, root, /*runObject*/ false));
	auto* document = dynamic_cast<ibValueMetaObjectDocument*>(
		cfg.CreateMetaObject(g_metaDocumentCLSID, root, /*runObject*/ false));
	ASSERT_NE(catalog, nullptr);
	ASSERT_NE(document, nullptr);
	document->SetName(wxT("Invoice"));

	ibValue managerValue = ManagerOf(catalog);
	ibValueManagerDataObject* manager = nullptr;
	ASSERT_TRUE(managerValue.ConvertToValue(manager));
	ASSERT_NE(manager, nullptr);

	const long method = manager->FindMethod(wxT("AddCreationBasedOnCommands"));
	ASSERT_GE(method, 0);
	EXPECT_GE(manager->FindMethod(wxT("CreateElement")), 0);
	EXPECT_GE(manager->FindMethod(wxT("EmptyRef")), 0);

	ibValue commands(new ibValueModelTable());
	ibValue* params[] = { &commands };
	ASSERT_TRUE(manager->CallAsProc(method, params, 1));

	ibValueModelTable* table = nullptr;
	ASSERT_TRUE(commands.ConvertToValue(table));
	ASSERT_NE(table, nullptr);
	EXPECT_EQ(table->GetRowCount(), 0);

	catalog->GetGenerationDescription().AppendMetaType(document->GetMetaID());
	ASSERT_TRUE(manager->CallAsProc(method, params, 1));
	ASSERT_EQ(table->GetRowCount(), 1);

	const ibMetaID presentation = table->GetColumnIDByName(wxT("Presentation"));
	const ibMetaID id = table->GetColumnIDByName(wxT("Id"));
	ibValue shown;
	ibValue named;
	ASSERT_TRUE(table->GetValueByMetaID(table->GetItem(0), presentation, shown));
	ASSERT_TRUE(table->GetValueByMetaID(table->GetItem(0), id, named));
	EXPECT_EQ(wxString(shown.GetString()), wxT("Invoice"));
	EXPECT_EQ(wxString(named.GetString()), wxT("Invoice"));
}
