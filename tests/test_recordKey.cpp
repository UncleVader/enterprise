// CreateRecordKey(Structure) copies the named dimensions onto the key.
// A name that is not a dimension is refused. With no argument the key is empty,
// and each dimension still reads as the empty value of its type.
//
// The same verb answers to СоздатьКлючЗаписи. A record set publishes its filter
// as Filter and as Отбор, and Filter.Dimension.Set writes that dimension.

#include <gtest/gtest.h>

#include <memory>

#include <wx/init.h>
#include <wx/image.h>
#include <wx/log.h>

#include "backend/appData.h"
#include "backend/databaseLayer/connectionPool.h"
#include "backend/databaseLayer/sqllite/sqliteDatabaseLayer.h"
#include "backend/metadataConfiguration.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/partial/informationRegister.h"
#include "backend/metaCollection/partial/informationRegisterManager.h"
#include "backend/session/session.h"
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
	const long blankField = empty.FindProp(wxT("Warehouse"));
	ASSERT_GE(blankField, 0);
	ibValue blank;
	ASSERT_TRUE(empty.GetPropVal(blankField, blank));
	EXPECT_TRUE(blank.IsEmpty());

	ibValue wrong = StructureOf(wxT("Missing"), ibValue(wxT("x")));
	ibValue* bad[] = { &wrong };
	ibValue refused;
	EXPECT_THROW(manager.CallAsFunc(create, refused, bad, 1), ibBackendException);

	const long russian = Method(manager, wxT("СоздатьКлючЗаписи"));
	ASSERT_GE(russian, 0);
	EXPECT_NE(russian, create);
	ibValue russianKey;
	ASSERT_TRUE(manager.CallAsFunc(russian, russianKey, args, 1));
	const long russianWarehouse = russianKey.FindProp(wxT("Warehouse"));
	ASSERT_GE(russianWarehouse, 0);
	ibValue russianHeld;
	ASSERT_TRUE(russianKey.GetPropVal(russianWarehouse, russianHeld));
	EXPECT_EQ(russianHeld.GetString(), wxString(wxT("A")));
}

// The set's filter is bound when the set is brought up, which is the running
// configuration: a session, its module manager, and the register's type.
struct RecordSetFilter : ::testing::Test {
	wxInitializer m_wxInit;
	std::shared_ptr<ibDatabaseLayerSQLite> db;
	std::unique_ptr<ibMetaDataConfigurationFile> cfg;
	std::shared_ptr<ibSession> session;
	std::unique_ptr<ibSessionScope> bound;
	ibValueMetaObjectInformationRegister* reg = nullptr;
	bool ready = false;

	void SetUp() override {
		if (!m_wxInit.IsOk())
			GTEST_SKIP() << "wxBase init failed (no wxApp host)";
		wxInitAllImageHandlers();
		if (wxLog::GetActiveTarget() != nullptr)
			delete wxLog::SetActiveTarget(new wxLogStderr());
		if (!ibApplicationInstance::CreateAppDataEnv(ibRunMode::eFILE_MODE))
			GTEST_SKIP() << "appData env unavailable headless";
		ibConnectionPool* pool = ibApplicationInstance::GetConnectionPool();
		if (pool == nullptr)
			GTEST_SKIP() << "no connection pool after CreateAppDataEnv";
		db = std::make_shared<ibDatabaseLayerSQLite>();
		if (!db->Open(wxT(":memory:")))
			GTEST_SKIP() << "in-memory SQLite open failed";
		pool->Init(db, /*maxSize=*/1, /*minIdle=*/0);

		session = std::make_shared<ibSession>(wxString(wxT("record-set-filter")), ibSessionKind::Enterprise);
		bound = std::make_unique<ibSessionScope>(session.get());

		cfg = std::make_unique<ibMetaDataConfigurationFile>();
		ibValueMetaObjectConfiguration* root = cfg->GetCommonMetaObject();
		ASSERT_NE(root, nullptr);
		reg = dynamic_cast<ibValueMetaObjectInformationRegister*>(
			cfg->CreateMetaObject(g_metaInformationRegisterCLSID, root, false));
		ASSERT_NE(reg, nullptr);
		ASSERT_NE(cfg->CreateMetaObject(g_metaDimensionCLSID, reg, false), nullptr);
		reg->GetDimensionArrayObject().front()->SetName(wxT("Warehouse"));
		reg->GetDimensionArrayObject().front()->GetTypeDesc().SetDefaultMetaType(ibValueTypes::TYPE_STRING);
		ASSERT_TRUE(cfg->RunDatabase()) << "the configuration in memory did not run";
		ASSERT_NE(session->CreateRoot(cfg.get()), nullptr);
		ready = true;
	}

	void TearDown() override {
		bound.reset();
		session.reset();
		if (cfg && cfg->IsConfigOpen())
			cfg->CloseDatabase();
		cfg.reset();
		if (ibApplicationInstance::Get() != nullptr)
			ibApplicationInstance::DestroyAppDataEnv();
	}
};

TEST_F(RecordSetFilter, FilterAndItsRussianNameAddressTheSameDimension)
{
	if (!ready)
		GTEST_SKIP();

	const ibValuePtr<ibValueRecordSetObject> created = reg->CreateRecordSetObjectValue(true);
	ibValueRecordSetObject* set = created;
	ASSERT_NE(set, nullptr);

	const long filterProp = set->FindProp(wxT("Filter"));
	const long russianProp = set->FindProp(wxT("Отбор"));
	ASSERT_GE(filterProp, 0);
	ASSERT_GE(russianProp, 0);

	ibValue filter;
	ibValue russian;
	ASSERT_TRUE(set->GetPropVal(filterProp, filter));
	ASSERT_TRUE(set->GetPropVal(russianProp, russian));
	EXPECT_EQ(filter.GetRef(), russian.GetRef());

	const long warehouse = filter.FindProp(wxT("Warehouse"));
	ASSERT_GE(warehouse, 0);
	ibValue description;
	ASSERT_TRUE(filter.GetPropVal(warehouse, description));

	const long setMethod = description.FindMethod(wxT("Set"));
	ASSERT_GE(setMethod, 0);
	ibValue value(wxT("A"));
	ibValue* args[] = { &value };
	ibValue ignored;
	ASSERT_TRUE(description.CallAsFunc(setMethod, ignored, args, 1));

	const long valueProp = description.FindProp(wxT("Value"));
	const long useProp = description.FindProp(wxT("Use"));
	ASSERT_GE(valueProp, 0);
	ASSERT_GE(useProp, 0);
	ibValue held;
	ibValue used;
	ASSERT_TRUE(description.GetPropVal(valueProp, held));
	ASSERT_TRUE(description.GetPropVal(useProp, used));
	EXPECT_EQ(held.GetString(), wxString(wxT("A")));
	EXPECT_TRUE(used.GetBoolean());
}
