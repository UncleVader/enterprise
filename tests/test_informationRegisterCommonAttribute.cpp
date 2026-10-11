// =============================================================================
// A common attribute on an information register.
//
// Checking an object into a common attribute's composition is what turns the
// declaration into a column. A catalog accepts that. An information register
// used to refuse it — "the catalog / document line accepts one and a register
// does not" — so a declaration used everywhere (DieselService's АвторСотрудник)
// could not land on the register at all.
//
// What is pinned: the register accepts the copy, and the copy is a column of the
// table, a field of the record, and an attribute of the query. An accumulation
// register still refuses — that family's composition has not been decided.
// DB-free: a fresh file configuration, tree edits only.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/metaCommonAttributeObject.h"
#include "backend/metaCollection/partial/informationRegister.h"
#include "backend/query/schemaSnapshot.h"

namespace {

struct RegisterCommonAttribute {
	ibMetaDataConfigurationFile cfg;
	ibValueMetaObjectConfiguration* root = nullptr;
	ibValueMetaObjectInformationRegister* reg = nullptr;
	ibValueMetaObjectCommonAttribute* declaration = nullptr;

	RegisterCommonAttribute() {
		root = cfg.GetCommonMetaObject();
		reg = dynamic_cast<ibValueMetaObjectInformationRegister*>(
			cfg.CreateMetaObject(g_metaInformationRegisterCLSID, root, /*runObject*/ false));
		declaration = dynamic_cast<ibValueMetaObjectCommonAttribute*>(
			cfg.CreateMetaObject(g_metaCommonAttributeCLSID, root, /*runObject*/ false));
		if (declaration != nullptr)
			declaration->SetName(wxT("АвторСотрудник"));
	}

	ibValueMetaObject* Copy() const {
		if (reg == nullptr)
			return nullptr;
		for (unsigned int i = 0; i < reg->GetChildCount(); ++i) {
			ibValueMetaObject* child = reg->GetChild(i);
			if (child != nullptr && child->GetClassType() == g_metaCommonAttributeColumnCLSID)
				return child;
		}
		return nullptr;
	}
};

} // namespace

TEST(InformationRegisterCommonAttribute, TheRegisterAcceptsOne) {
	RegisterCommonAttribute f;
	ASSERT_NE(f.reg, nullptr);
	ASSERT_NE(f.declaration, nullptr);

	EXPECT_TRUE(f.reg->IsCompositionAllowed());
	EXPECT_GT(f.reg->ResolveChild(g_metaCommonAttributeColumnCLSID), 0u);

	ASSERT_TRUE(f.declaration->SetCompositionObject(f.reg, true));
	ASSERT_NE(f.Copy(), nullptr);
	EXPECT_EQ(f.Copy()->GetName(), wxT("АвторСотрудник"));
	EXPECT_TRUE(f.declaration->IsCompositionObject(f.reg));
}

// The three places a carried attribute has to show up: the table the register
// declares, the record a script holds, and the query written against the register.
TEST(InformationRegisterCommonAttribute, ItIsAColumnARecordFieldAndAQueryAttribute) {
	RegisterCommonAttribute f;
	ASSERT_NE(f.reg, nullptr);
	ASSERT_TRUE(f.declaration->SetCompositionObject(f.reg, true));
	ibValueMetaObject* const copy = f.Copy();
	ASSERT_NE(copy, nullptr);

	bool inAttributes = false;
	for (const ibValueMetaObjectAttributeBase* a : f.reg->GetAttributeArrayObject())
		if (a != nullptr && a->GetMetaID() == copy->GetMetaID())
			inAttributes = true;
	EXPECT_TRUE(inAttributes);

	ibSchemaSnapshot snapshot;
	f.reg->ContributeTables(snapshot);
	bool inSchema = false;
	for (const ibSchemaTable& table : snapshot.Tables()) {
		if (table.m_derived)
			continue;
		for (const ibSchemaColumn& column : table.m_columns)
			if (column.m_id == copy->GetMetaID())
				inSchema = true;
	}
	EXPECT_TRUE(inSchema);

	// The public record-manager constructor. CreateRecordManagerObjectRegValue is the
	// metadata's own factory and stays protected; this is the same object it news.
	const ibValuePtr<ibValueRecordManagerObject> record(
		new ibValueRecordManagerObjectInformationRegister(f.reg));
	ASSERT_TRUE(record);
	EXPECT_GE(record->FindProp(wxT("АвторСотрудник")), 0);

	const ibBackendQueryable* queryable = f.reg->GetQueryable();
	ASSERT_NE(queryable, nullptr);
	const ibBackendQueryColumn* named = queryable->ResolveColumnByName(wxT("АвторСотрудник"));
	ASSERT_NE(named, nullptr);
	EXPECT_EQ(named->GetName(), wxT("АвторСотрудник"));

	bool inSelectStar = false;
	for (const ibBackendQueryColumn* column : queryable->GetColumns())
		if (column != nullptr && column->GetName() == wxT("АвторСотрудник"))
			inSelectStar = true;
	EXPECT_TRUE(inSelectStar);
}

// The other register families still say no. Opening every one of them is a
// separate decision; this one is the information register.
TEST(InformationRegisterCommonAttribute, AnAccumulationRegisterStillRefuses) {
	RegisterCommonAttribute f;
	ASSERT_NE(f.declaration, nullptr);
	ibValueMetaObject* const accumulation = f.cfg.CreateMetaObject(
		g_metaAccumulationRegisterCLSID, f.root, /*runObject*/ false);
	ASSERT_NE(accumulation, nullptr);

	EXPECT_FALSE(accumulation->IsCompositionAllowed());
	EXPECT_EQ(accumulation->ResolveChild(g_metaCommonAttributeColumnCLSID), 0u);
	EXPECT_FALSE(f.declaration->SetCompositionObject(accumulation, true));
}
