// CreateRecordKey(Structure) copies the named dimensions onto the key.
// A name that is not a dimension is refused. With no argument the key is empty,
// and each dimension still reads as the empty value of its type.

#include <gtest/gtest.h>

#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/partial/informationRegister.h"
#include "backend/metaCollection/partial/informationRegisterManager.h"
#include "backend/system/value/valueMap.h"
#include "backend_exception.h"

namespace {

ibValue StructureOf(const wxString& name, const ibValue& value)
{
	ibValueStructure* structure = new ibValueStructure();
	structure->Insert(name, value);
	return structure;
}

long Method(ibValue& manager, const wxString& name)
{
	return manager.FindMethod(name);
}

} // namespace

TEST(RecordKey, CreateRecordKeyCopiesNamedDimensions)
{
	ibMetaDataConfigurationFile cfg;
	ibValueMetaObjectConfiguration* root = cfg.GetCommonMetaObject();
	ASSERT_NE(root, nullptr);

	ibValueMetaObject* created = cfg.CreateMetaObject(g_metaInformationRegisterCLSID, root, false);
	auto* reg = dynamic_cast<ibValueMetaObjectInformationRegister*>(created);
	ASSERT_NE(reg, nullptr);
	ASSERT_NE(cfg.CreateMetaObject(g_metaDimensionCLSID, reg, false), nullptr);
	ASSERT_EQ(reg->GetDimensionArrayObject().size(), 1u);
	reg->GetDimensionArrayObject().front()->SetName(wxT("Warehouse"));
	reg->GetDimensionArrayObject().front()->GetTypeDesc().SetDefaultMetaType(ibValueTypes::TYPE_STRING);

	ibValueManagerDataObjectInformationRegister manager(reg);
	const long create = Method(manager, wxT("CreateRecordKey"));
	ASSERT_GE(create, 0);

	ibValue filter = StructureOf(wxT("Warehouse"), ibValue(wxT("A")));
	ibValue* args[] = { &filter };
	ibValue key;
	ASSERT_TRUE(manager.CallAsFunc(create, key, args, 1));
	ASSERT_FALSE(key.IsEmpty());

	const long warehouse = key.FindProp(wxT("Warehouse"));
	ASSERT_GE(warehouse, 0);
	ibValue held;
	ASSERT_TRUE(key.GetPropVal(warehouse, held));
	EXPECT_EQ(held.GetString(), wxString(wxT("A")));

	ibValue empty;
	ASSERT_TRUE(manager.CallAsFunc(create, empty, nullptr, 0));
	ibValue blank;
	ASSERT_TRUE(empty.GetPropVal(warehouse, blank));
	EXPECT_TRUE(blank.IsEmpty());

	ibValue wrong = StructureOf(wxT("Missing"), ibValue(wxT("x")));
	ibValue* bad[] = { &wrong };
	ibValue refused;
	EXPECT_THROW(manager.CallAsFunc(create, refused, bad, 1), ibBackendException);
}
