// FindByAttribute — the first item whose attribute equals a value.
//
// The lookup itself needs a base. What is pinned here is the verb: it is on the
// manager, it takes the attribute's name and the value, a name that is not an
// attribute answers with an empty reference, and an empty value does not go
// looking. FindByCode stays the method it was.

#include <gtest/gtest.h>

#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/partial/catalog.h"
#include "backend/metaCollection/partial/reference/reference.h"
#include "backend/objCtor.h"

namespace {

ibValueReferenceDataObject* AsRef(const ibValue& value)
{
	ibValueReferenceDataObject* ref = nullptr;
	if (!value.ConvertToValue(ref))
		return nullptr;
	return ref;
}

} // namespace

TEST(FindByAttribute, AnUnknownNameAndAnEmptyValueAreEmptyReferences)
{
	ibMetaDataConfigurationFile cfg;
	ibValueMetaObject* const created = cfg.CreateMetaObject(
		g_metaCatalogCLSID, cfg.GetCommonMetaObject(), /*runObject*/ false);
	auto* const catalog = dynamic_cast<ibValueMetaObjectCatalog*>(created);
	ASSERT_NE(catalog, nullptr);

	// The factory that builds a manager is protected. The public door is the same one
	// the runtime uses when a script writes Catalogs.<Name>: the manager type constructor.
	ibCtorMetaValueTypeManager door(catalog);
	ibValue created = door.CreateObject();
	ibValueManagerDataObject* manager = nullptr;
	ASSERT_TRUE(created.ConvertToValue(manager));

	const long method = manager->FindMethod(wxT("FindByAttribute"));
	ASSERT_GE(method, 0);
	EXPECT_EQ(manager->GetNParams(method), 2);
	EXPECT_GE(manager->FindMethod(wxT("FindByCode")), 0);
	EXPECT_GE(manager->FindMethod(wxT("EmptyRef")), 0);

	ibValue name;
	name.SetString(wxT("NoSuch"));
	ibValue value;
	value.SetString(wxT("x"));
	ibValue* params[] = { &name, &value };
	ibValue result;
	ASSERT_TRUE(manager->CallAsFunc(method, result, params, 2));

	ibValueReferenceDataObject* const ref = AsRef(result);
	ASSERT_NE(ref, nullptr);
	EXPECT_TRUE(ref->IsEmpty());

	ibValue codeName;
	codeName.SetString(wxT("Code"));
	ibValue nothing;
	ibValue* emptyParams[] = { &codeName, &nothing };
	ibValue emptyResult;
	ASSERT_TRUE(manager->CallAsFunc(method, emptyResult, emptyParams, 2));

	ibValueReferenceDataObject* const emptyRef = AsRef(emptyResult);
	ASSERT_NE(emptyRef, nullptr);
	EXPECT_TRUE(emptyRef->IsEmpty());
}
