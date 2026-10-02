// =============================================================================
// Accumulation-register list at about one million movements.
//
// The list a register opens is ibCreateList on its movements queryable — the
// same object GetListForm hands the form (accumulationRegisterMetadata.cpp).
// This drives that object's GetFirstFetch / GetNextFetch / GetPrevFetch,
// AddFilter and AddSort, which is what the dataview control calls. It does
// not open a window: the desktop client is not a SQLite host, and this
// environment has no interactive display. A person still has to click the
// list; that pass is written into the report.
//
// There is no demo configuration and no bulk loader in the tree (the docs
// submodule is private). The register is built here, the same way the
// metadata tests build a catalog: a document GoodsReceipt and an accumulation
// register Goods (Warehouse, Item, Quantity), schema applied through
// BuildSchemaSnapshot + the create-all save, then RunDatabase so the
// register's queryable is in that configuration's factory, then a prepared INSERT
// of 10 000 documents x 100 lines. Totals triggers are dropped after DDL —
// the list reads movements, and maintaining totals is not what this measures.
//
// Not part of ctest. The binary is oes_register_list_scale.
//   OES_REGISTER_ROWS   default 1000000, rounded down to a multiple of 100
//   OES_REGISTER_DB     default /tmp/oes-goods-movements.db
//   OES_REGISTER_REPORT default <repo>/tests/register-list-scale-report.md
// =============================================================================

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <wx/init.h>
#include <wx/datetime.h>

#include "backend/appData.h"
#include "backend/backend_exception.h"
#include "backend/clsid.h"
#include "backend/compiler/value.h"
#include "backend/databaseLayer/connectionPool.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseResultSet.h"
#include "backend/databaseLayer/preparedStatement.h"
#include "backend/databaseLayer/sqllite/sqliteDatabaseLayer.h"
#include "backend/guid.h"
#include "backend/metaCollection/attribute/metaAttributeObject.h"
#include "backend/metaCollection/attribute/metaAttributeObjectEnum.h"
#include "backend/metaCollection/metaObject.h"
#include "backend/metaCollection/partial/accumulationRegister.h"
#include "backend/metaCollection/partial/document.h"
#include "backend/metadataConfiguration.h"
#include "backend/propertyManager/property/propertyBoolean.h"
#include "backend/propertyManager/property/propertyEnum.h"
#include "backend/query/queryColumn.h"
#include "backend/query/schemaSnapshot.h"
#include "backend/query/structureBuilder.h"
#include "backend/system/value/valueDynamicList.h"
#include "backend/tabularModel.h"
#include "backend/tabularModelView.h"
#include "backend/valueInfo.h"

namespace {

constexpr int kLinesPerDocument = 100;
constexpr int kPageSize         = 100;
constexpr int kForwardPages     = 100;
constexpr int kWarehouses       = 20;
constexpr int kItems            = 50;
constexpr int kPeriodDays       = 730;

long EnvLong(const char* name, long fallback) {
	const char* v = std::getenv(name);
	if (v == nullptr || *v == '\0')
		return fallback;
	char* end = nullptr;
	const long n = std::strtol(v, &end, 10);
	if (end == v)
		return fallback;
	return n;
}

std::string EnvStr(const char* name, const std::string& fallback) {
	const char* v = std::getenv(name);
	return (v != nullptr && *v != '\0') ? std::string(v) : fallback;
}

long VmRssKb() {
	std::ifstream in("/proc/self/status");
	std::string line;
	while (std::getline(in, line)) {
		if (line.compare(0, 6, "VmRSS:") == 0) {
			const auto pos = line.find_first_of("0123456789");
			if (pos == std::string::npos)
				return -1;
			return std::stol(line.substr(pos));
		}
	}
	return -1;
}

double MsSince(std::chrono::steady_clock::time_point t0) {
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

wxString Describe(const std::exception& err) {
	return wxString::FromUTF8(err.what());
}

struct Scenario {
	std::string name;
	double      ms = 0;
	int         rows = 0;
	bool        ok = false;
	std::string note;
};

struct Report {
	std::string path;
	std::string dbPath;
	long        rows = 0;
	std::string table;
	double      schemaMs = 0;
	double      seedMs = 0;
	double      analyzeMs = 0;
	long long   counted = -1;
	long long   warehouse7 = -1;
	long        rssAfterSeedKb = -1;
	long        rssAfterListKb = -1;
	std::vector<Scenario> scenarios;
	std::vector<std::string> notes;
	std::string fatal;

	~Report() { Write(); }

	void Write() const {
		if (path.empty())
			return;
		std::ofstream out(path);
		if (!out)
			return;
		out << "# Register list at one million movements\n\n";
		out << "Headless pass of the accumulation-register list. The object under test is\n";
		out << "`ibCreateList` on the register's movements queryable — the same value\n";
		out << "`ibValueMetaObjectAccumulationRegister::GetListForm` puts on the form.\n";
		out << "Paging, filters and sorts go through `GetFirstFetch`, `GetNextFetch`,\n";
		out << "`GetPrevFetch`, `AddFilter` and `AddSort`, which is the door\n";
		out << "`datavgen.paged.cpp` calls. No window was opened.\n\n";

		out << "## How to reproduce\n\n";
		out << "```bash\n";
		out << "cmake --preset linux-release -DBUILD_TESTING=ON\n";
		out << "cmake --build build/linux-release --target oes_register_list_scale -j 2\n";
		out << "OES_REGISTER_ROWS=1000000 \\\n";
		out << "OES_REGISTER_DB=/tmp/oes-goods-movements.db \\\n";
		out << "OES_REGISTER_REPORT=tests/register-list-scale-report.md \\\n";
		out << "  ./build/linux-release/bin/Release/oes_register_list_scale\n";
		out << "```\n\n";
		out << "The binary is not registered with ctest. `OES_REGISTER_ROWS` is rounded\n";
		out << "down to a multiple of 100 (one document is 100 lines). A smaller value\n";
		out << "is only for debugging the harness.\n\n";

		out << "## What was loaded\n\n";
		out << "Accumulation register **Goods**, recorder document **GoodsReceipt**.\n";
		out << "Dimensions Warehouse and Item (number, indexed), resource Quantity\n";
		out << "(number, indexed). Period, Active, RecordType, Recorder and LineNumber\n";
		out << "are the register's own columns. Ten thousand documents would be one\n";
		out << "million lines; this run asked for **" << rows << "** lines.\n\n";
		out << "Distribution, deterministic: period is 2024-01-01 plus `(document % 730)`\n";
		out << "days (a document's lines share a day); line number is 1..100; warehouse\n";
		out << "is `(document + line) % 20`; item is `(document * 3 + line) % 50`;\n";
		out << "quantity is `1 + ((document * 100 + line) % 500)` so the range is 1..500.\n";
		out << "Warehouse 7 therefore matches 5 lines of every document.\n\n";
		out << "Schema is `BuildSchemaSnapshot` plus the create-all save (`OnSave` from an\n";
		out << "empty baseline) on a file SQLite database (`" << dbPath << "`).\n";
		out << "Physical table: `" << table << "`.\n";
		out << "Split totals is off, and every SQLite trigger is dropped after DDL, so\n";
		out << "the insert does not maintain balance or turnover tables. The list does\n";
		out << "not read those tables. Totals upkeep is out of scope.\n\n";

		out << "| Step | ms |\n|---|---:|\n";
		out << "| Schema | " << schemaMs << " |\n";
		out << "| Insert | " << seedMs << " |\n";
		out << "| ANALYZE | " << analyzeMs << " |\n\n";
		out << "COUNT(*) = " << counted << ". Rows with Warehouse = 7: " << warehouse7 << ".\n";
		out << "VmRSS after the insert: " << rssAfterSeedKb << " kB. After the list pass: "
		    << rssAfterListKb << " kB.\n\n";

		if (!fatal.empty()) {
			out << "## Stopped\n\n```\n" << fatal << "\n```\n\n";
		}

		out << "## List pass\n\n";
		out << "| Check | Rows | ms | Result |\n|---|---:|---:|---|\n";
		for (const Scenario& s : scenarios) {
			out << "| " << s.name << " | " << s.rows << " | " << s.ms << " | "
			    << (s.ok ? "pass" : "FAIL") << " |\n";
		}
		out << "\n";
		for (const Scenario& s : scenarios) {
			if (!s.note.empty())
				out << "- **" << s.name << ".** " << s.note << "\n";
		}
		if (!notes.empty()) {
			out << "\n";
			for (const std::string& n : notes)
				out << "- " << n << "\n";
		}

		out << "\n## What a person still has to do in the UI\n\n";
		out << "This process cannot host the desktop client. On a machine with a display,\n";
		out << "against a Firebird or PostgreSQL base (SQLite is the test driver, not the\n";
		out << "one Enterprise opens):\n\n";
		out << "1. Create the same Goods register, or point a loader at an existing one,\n";
		out << "   and insert on the order of 10^6 movements.\n";
		out << "2. Open the register's list form (the generated list, not a hand-built form).\n";
		out << "3. Scroll forward and back with the wheel and the scrollbar. The first\n";
		out << "   pages and a page a long way down should arrive in about the same time;\n";
		out << "   a pause that grows with the scroll position means the keyset is scanning.\n";
		out << "4. Click the Period, Quantity and Warehouse column headers and confirm the\n";
		out << "   order and that the next page continues it.\n";
		out << "5. Open the filter and restrict Warehouse, Quantity and Period. The grid\n";
		out << "   must show only matching rows, and an impossible value must show an empty list.\n\n";
		out << "## Defects\n\n";
		out << "SQLite bound every number as a double and read a 64-bit integer back through\n";
		out << "`sqlite3_column_int`. A reference class id near 2^60 does not survive that, and\n";
		out << "a register list keys its page on the recorder reference. Integer values now bind\n";
		out << "and read as integers; a fraction still goes through the double path.\n\n";
		out << "A dynamic list remembered its source as a table id and resolved that id through\n";
		out << "the property owner's configuration. A list that is not on a form answers with the\n";
		out << "active configuration, which is a different object from the one that registered the\n";
		out << "register. The resolve missed, `GetSourceQueryable()` was null, and the first page\n";
		out << "came back empty in a fraction of a millisecond with no error. The cell now keeps\n";
		out << "the configuration the queryable itself names. The configuration still has to be\n";
		out << "run (`RunDatabase`) so the source is registered; this harness does that after DDL.\n\n";
		out << "SQLite bound a blob with `SQLITE_STATIC`, and the cursor steps when the caller\n";
		out << "reads it, after that buffer is gone. A page that ties on a reference — many\n";
		out << "movements share a period — compared a dead guid and jumped to the next distinct\n";
		out << "sort value. The binder now copies the bytes, the same way it already copies text.\n\n";
		out << "Dragging the scrollbar to the end is not implemented. `datavgen.cpp` says so:\n";
		out << "a snap to the bottom would need N forward fetches, and there is no model API\n";
		out << "for the last batch. That waits on async fetch. It is not fixed here.\n\n";
		out << "Sorting by a dotted path (Recorder's number, for example) loads the whole\n";
		out << "ordered result in one shot (`tabularModelDb.cpp`, the dot-walk branch).\n";
		out << "A sort on a column of the register itself keyset-pages. The checks above\n";
		out << "use columns, not dotted paths.\n";
	}
};

struct Sig {
	long long period = 0;
	long      line = 0;
	long      warehouse = -1;
	long      item = -1;
	long      quantity = -1;
	unsigned long long recorder = 0;
	bool      ok = false;
};

bool operator==(const Sig& a, const Sig& b) {
	return a.ok && b.ok && a.period == b.period && a.line == b.line
		&& a.warehouse == b.warehouse && a.item == b.item && a.quantity == b.quantity
		&& a.recorder == b.recorder;
}

enum class Field { Other, Active, Period, RecordType, Recorder, Line, Warehouse, Item, Quantity };

struct Bind {
	enum class Role { Disc, Bool, Num, Date, Enum, RefType, RefId } role;
	Field field = Field::Other;
	int   tag = 0;
};

struct Ids {
	ibMetaID period = 0;
	ibMetaID line = 0;
	ibMetaID warehouse = 0;
	ibMetaID item = 0;
	ibMetaID quantity = 0;
	ibMetaID recorder = 0;
};

int TagFor(const std::vector<ibColumnSlot>& slots) {
	for (const ibColumnSlot& s : slots) {
		switch (s.m_role) {
		case ibColumnRole::Boolean:       return ibFieldTypes_Boolean;
		case ibColumnRole::Number:        return ibFieldTypes_Number;
		case ibColumnRole::Date:          return ibFieldTypes_Date;
		case ibColumnRole::Enum:          return ibFieldTypes_Enum;
		case ibColumnRole::ReferenceType: return ibFieldTypes_Reference;
		default: break;
		}
	}
	return ibFieldTypes_Empty;
}

void IndexField(ibValueMetaObjectAttribute* attr) {
	if (attr == nullptr)
		return;
	auto* prop = dynamic_cast<ibPropertyEnumBase*>(attr->GetProperty(wxT("Indexing")));
	if (prop != nullptr)
		prop->SetValue(static_cast<long>(ibIndexingMode::ibIndexingMode_Index));
}

wxString CatchText() {
	try {
		throw;
	}
	catch (const ibBackendException& err) {
		return err.GetErrorDescription();
	}
	catch (const std::exception& err) {
		return Describe(err);
	}
	catch (...) {
		return wxT("unknown exception");
	}
}

struct Page {
	std::vector<Sig> rows;
	ibDataViewItem   first;
	ibDataViewItem   last;
	double           ms = 0;
	wxString         error;
};

Sig ReadSig(ibValueDynamicList* list, const ibDataViewItem& item, const Ids& ids, wxString& error) {
	Sig s;
	auto num = [&](ibMetaID id, long& dst) {
		ibValue v;
		if (!list->GetValueByMetaID(item, id, v) || v.GetType() != ibValueTypes::TYPE_NUMBER) {
			error = wxT("number cell unreadable");
			return false;
		}
		dst = v.GetInteger();
		return true;
	};
	ibValue period;
	if (!list->GetValueByMetaID(item, ids.period, period) || period.GetType() != ibValueTypes::TYPE_DATE) {
		error = wxT("period cell unreadable");
		return s;
	}
	s.period = period.GetDate();
	if (!num(ids.line, s.line) || !num(ids.warehouse, s.warehouse)
		|| !num(ids.item, s.item) || !num(ids.quantity, s.quantity))
		return s;
	ibValue rec;
	if (list->GetValueByMetaID(item, ids.recorder, rec))
		s.recorder = static_cast<unsigned long long>(rec.GetClassType());
	s.ok = true;
	return s;
}

Page Fetch(ibValueDynamicList* list, const Ids& ids, bool backward, const ibDataViewItem& anchor) {
	Page page;
	ibDataViewItemArray items;
	const auto t0 = std::chrono::steady_clock::now();
	try {
		if (!anchor.IsOk())
			list->GetFirstFetch(s_constIgnoreParent, ibDataViewItem(), kPageSize, items);
		else if (backward)
			list->GetPrevFetch(s_constIgnoreParent, anchor, kPageSize, items);
		else
			list->GetNextFetch(s_constIgnoreParent, anchor, kPageSize, items);
	}
	catch (...) {
		page.error = CatchText();
		page.ms = MsSince(t0);
		return page;
	}
	page.ms = MsSince(t0);
	if (items.GetCount() > 0) {
		page.first = items[0];
		page.last  = items[items.GetCount() - 1];
	}
	page.rows.reserve(items.GetCount());
	for (size_t i = 0; i < items.GetCount(); ++i) {
		wxString err;
		Sig s = ReadSig(list, items[i], ids, err);
		if (!s.ok && page.error.empty())
			page.error = err;
		page.rows.push_back(s);
	}
	return page;
}

bool PeriodOrdered(const std::vector<Sig>& rows, bool ascending, std::string& why) {
	for (size_t i = 1; i < rows.size(); ++i) {
		if (!rows[i].ok || !rows[i - 1].ok) {
			why = "a row did not read back";
			return false;
		}
		const bool step = ascending ? rows[i].period >= rows[i - 1].period
		                            : rows[i].period <= rows[i - 1].period;
		if (!step) {
			why = "period left the sort order";
			return false;
		}
	}
	return true;
}

} // namespace

TEST(RegisterListScale, MillionMovementsNavigateFilterSort) {
	const std::string reportPath = EnvStr("OES_REGISTER_REPORT",
		"/workspace/tests/register-list-scale-report.md");
	Report report;
	report.path   = reportPath;
	report.dbPath = EnvStr("OES_REGISTER_DB", "/tmp/oes-goods-movements.db");
	long rows = EnvLong("OES_REGISTER_ROWS", 1000000);
	if (rows < kLinesPerDocument)
		rows = kLinesPerDocument;
	rows -= rows % kLinesPerDocument;
	report.rows = rows;
	const long documents = rows / kLinesPerDocument;

	auto fail = [&](const wxString& why) {
		report.fatal = std::string(why.ToUTF8());
		ADD_FAILURE() << report.fatal;
	};

	wxInitializer wxInit;
	if (!wxInit.IsOk()) {
		fail(wxT("wx init failed — the list door needs wxBase (run under xvfb if there is no display)"));
		return;
	}
	if (!ibApplicationData::CreateAppDataEnv(ibRunMode::eRUNTIME_MODE)) {
		fail(wxT("CreateAppDataEnv failed"));
		return;
	}
	struct EnvGuard {
		~EnvGuard() {
			if (ibApplicationData::Get() != nullptr)
				ibApplicationData::DestroyAppDataEnv();
		}
	} envGuard;

	ibConnectionPool* pool = ibApplicationData::GetConnectionPool();
	if (pool == nullptr) {
		fail(wxT("no connection pool"));
		return;
	}
	std::remove(report.dbPath.c_str());
	std::remove((report.dbPath + "-wal").c_str());
	std::remove((report.dbPath + "-shm").c_str());

	auto db = std::make_shared<ibDatabaseLayerSQLite>();
	if (!db->Open(wxString::FromUTF8(report.dbPath.c_str()))) {
		fail(wxT("SQLite open failed"));
		return;
	}
	pool->Init(db, /*maxSize=*/1, /*minIdle=*/0);

	ibMetaDataConfigurationFile cfg;
	ibValueMetaObject* root = cfg.GetCommonMetaObject();
	if (root == nullptr) {
		fail(wxT("configuration has no root"));
		return;
	}

	auto* doc = dynamic_cast<ibValueMetaObjectDocument*>(
		cfg.CreateMetaObject(g_metaDocumentCLSID, root, /*runObject*/ false));
	auto* reg = dynamic_cast<ibValueMetaObjectAccumulationRegister*>(
		cfg.CreateMetaObject(g_metaAccumulationRegisterCLSID, root, /*runObject*/ false));
	if (doc == nullptr || reg == nullptr) {
		fail(wxT("document or accumulation register was not created"));
		return;
	}
	ASSERT_TRUE(cfg.RenameMetaObject(doc, wxT("GoodsReceipt")));
	ASSERT_TRUE(cfg.RenameMetaObject(reg, wxT("Goods")));

	auto* warehouse = dynamic_cast<ibValueMetaObjectAttribute*>(
		cfg.CreateMetaObject(g_metaDimensionCLSID, reg, false));
	auto* item = dynamic_cast<ibValueMetaObjectAttribute*>(
		cfg.CreateMetaObject(g_metaDimensionCLSID, reg, false));
	auto* quantity = dynamic_cast<ibValueMetaObjectAttribute*>(
		cfg.CreateMetaObject(g_metaResourceCLSID, reg, false));
	if (warehouse == nullptr || item == nullptr || quantity == nullptr) {
		fail(wxT("dimension or resource was not created"));
		return;
	}
	ASSERT_TRUE(cfg.RenameMetaObject(warehouse, wxT("Warehouse")));
	ASSERT_TRUE(cfg.RenameMetaObject(item, wxT("Item")));
	ASSERT_TRUE(cfg.RenameMetaObject(quantity, wxT("Quantity")));
	warehouse->GetTypeDesc().SetDefaultMetaType(ibValueTypes::TYPE_NUMBER);
	item->GetTypeDesc().SetDefaultMetaType(ibValueTypes::TYPE_NUMBER);
	quantity->GetTypeDesc().SetDefaultMetaType(ibValueTypes::TYPE_NUMBER);
	IndexField(warehouse);
	IndexField(item);
	IndexField(quantity);
	reg->GetRegisterRecorder()->GetTypeDesc().SetDefaultMetaType(reference_to_clsid(doc->GetMetaID()));
	if (auto* split = dynamic_cast<ibPropertyBoolean*>(reg->GetProperty(wxT("SplitTotals"))))
		split->SetValue(false);

	const wxString table = reg->GetPhysicalTableName();
	report.table = std::string(table.ToUTF8());
	const unsigned long long recorderClsid = reference_to_clsid(doc->GetMetaID());

	const auto schemaT0 = std::chrono::steady_clock::now();
	try {
		ibStructureBuilder builder;
		// Recreate drops every declared table first. A fresh file has none of them, so
		// that drop is "no such table". The first apply of a configuration is the
		// create-all save: an empty baseline.
		const ibSchemaSnapshot snapshot = cfg.BuildSchemaSnapshot();
		if (!builder.OnBeforeSave()) {
			fail(wxT("schema transaction did not open"));
			return;
		}
		try {
			builder.OnSave(nullptr, snapshot);
			builder.OnAfterSave(false);
		}
		catch (...) {
			try { builder.OnAfterSave(true); } catch (...) {}
			throw;
		}
	}
	catch (...) {
		fail(wxT("schema create failed: ") + CatchText());
		return;
	}
	report.schemaMs = MsSince(schemaT0);

	// The list re-resolves its source through the configuration's factory. That
	// factory exists only while the configuration is open, and the register
	// registers its queryable in OnAfterRun. Creating the metaobjects is not enough.
	if (!cfg.RunDatabase()) {
		fail(wxT("configuration did not run"));
		return;
	}
	struct CloseRun {
		ibMetaDataConfigurationFile* cfg;
		~CloseRun() {
			if (cfg != nullptr && cfg->IsConfigOpen())
				cfg->CloseDatabase(forceCloseFlag);
		}
	} closeRun{&cfg};

	std::vector<wxString> triggers;
	{
		ibDatabaseResultSet* rs = db->RunQueryWithResults(
			wxT("SELECT name FROM sqlite_master WHERE type = 'trigger'"));
		while (rs != nullptr && rs->Next())
			triggers.push_back(rs->GetResultString(wxT("name")));
		if (rs != nullptr)
			db->CloseResultSet(rs);
	}
	for (const wxString& name : triggers)
		db->RunStatement(wxT("DROP TRIGGER IF EXISTS \"") + name + wxT("\""));
	report.notes.push_back("Dropped " + std::to_string(triggers.size())
		+ " SQLite triggers after DDL so the insert does not maintain totals.");

	db->RunStatement(wxT("PRAGMA journal_mode = OFF"));
	db->RunStatement(wxT("PRAGMA synchronous = OFF"));
	db->RunStatement(wxT("PRAGMA locking_mode = EXCLUSIVE"));
	db->RunStatement(wxT("PRAGMA temp_store = MEMORY"));
	db->RunStatement(wxT("PRAGMA cache_size = -262144"));

	std::vector<ibValueMetaObjectAttributeBase*> attrs = reg->GetGenericAttributeArrayObject();
	wxString columns;
	wxString placeholders;
	std::vector<Bind> binds;
	auto addCol = [&](const wxString& name) {
		if (!columns.empty()) {
			columns += wxT(", ");
			placeholders += wxT(", ");
		}
		columns += name;
		placeholders += wxT("?");
	};
	for (ibValueMetaObjectAttributeBase* attr : attrs) {
		if (attr == nullptr || attr->GetQueryColumn() == nullptr)
			continue;
		const std::vector<ibColumnSlot> slots = attr->GetQueryColumn()->DescribeLayout();
		const int tag = TagFor(slots);
		Field field = Field::Other;
		if (attr == reg->GetRegisterActive())         field = Field::Active;
		else if (attr == reg->GetRegisterPeriod())    field = Field::Period;
		else if (attr == reg->GetRegisterRecordType()) field = Field::RecordType;
		else if (attr == reg->GetRegisterRecorder())  field = Field::Recorder;
		else if (attr == reg->GetRegisterLineNumber()) field = Field::Line;
		else if (attr == warehouse)                   field = Field::Warehouse;
		else if (attr == item)                        field = Field::Item;
		else if (attr == quantity)                    field = Field::Quantity;
		for (const ibColumnSlot& slot : slots) {
			addCol(slot.m_name);
			Bind b;
			b.field = field;
			b.tag = tag;
			switch (slot.m_role) {
			case ibColumnRole::Discriminator: b.role = Bind::Role::Disc; break;
			case ibColumnRole::Boolean:       b.role = Bind::Role::Bool; break;
			case ibColumnRole::Number:        b.role = Bind::Role::Num; break;
			case ibColumnRole::Date:          b.role = Bind::Role::Date; break;
			case ibColumnRole::Enum:          b.role = Bind::Role::Enum; break;
			case ibColumnRole::ReferenceType: b.role = Bind::Role::RefType; break;
			case ibColumnRole::ReferenceId:   b.role = Bind::Role::RefId; break;
			default:
				fail(wxT("unexpected column slot ") + slot.m_name);
				return;
			}
			binds.push_back(b);
		}
	}

	const wxString sql = wxT("INSERT INTO ") + table + wxT(" (") + columns
		+ wxT(") VALUES (") + placeholders + wxT(")");
	ibPreparedStatement* insert = nullptr;
	try {
		insert = db->PrepareStatement(sql);
	}
	catch (...) {
		fail(wxT("prepare failed: ") + CatchText() + wxT(" SQL: ") + sql);
		return;
	}
	if (insert == nullptr) {
		fail(wxT("prepare returned null: ") + sql);
		return;
	}

	std::vector<ibReference> refs;
	refs.reserve(static_cast<size_t>(documents));
	for (long d = 0; d < documents; ++d) {
		std::array<unsigned char, 16> bytes{};
		bytes[15] = 1;
		const uint32_t n = static_cast<uint32_t>(d + 1);
		bytes[0] = static_cast<unsigned char>(n);
		bytes[1] = static_cast<unsigned char>(n >> 8);
		bytes[2] = static_cast<unsigned char>(n >> 16);
		bytes[3] = static_cast<unsigned char>(n >> 24);
		refs.push_back(ibReference(static_cast<ibGuidImpl>(ibGuid(bytes))));
	}
	std::vector<wxDateTime> dates(kPeriodDays);
	const wxDateTime epoch(1, wxDateTime::Jan, 2024, 0, 0, 0, 0);
	for (int i = 0; i < kPeriodDays; ++i) {
		dates[static_cast<size_t>(i)] = epoch;
		dates[static_cast<size_t>(i)] += wxDateSpan::Days(i);
	}

	const auto seedT0 = std::chrono::steady_clock::now();
	try {
		db->BeginTransaction();
		for (long d = 0; d < documents; ++d) {
			const int day = static_cast<int>(d % kPeriodDays);
			const ibReference& ref = refs[static_cast<size_t>(d)];
			for (int line = 0; line < kLinesPerDocument; ++line) {
				const long wh  = (d + line) % kWarehouses;
				const long it  = (d * 3 + line) % kItems;
				const long qty = 1 + ((d * kLinesPerDocument + line) % 500);
				int p = 1;
				for (const Bind& b : binds) {
					switch (b.role) {
					case Bind::Role::Disc:    insert->SetParamInt(p++, b.tag); break;
					case Bind::Role::Bool:    insert->SetParamBool(p++, true); break;
					case Bind::Role::Date:    insert->SetParamDate(p++, dates[static_cast<size_t>(day)]); break;
					case Bind::Role::Enum:    insert->SetParamInt(p++, static_cast<int>(ibRecordType::eReceipt)); break;
					case Bind::Role::RefType: insert->SetParamNumber(p++, ibNumber(recorderClsid)); break;
					case Bind::Role::RefId:   insert->SetParamBlob(p++, &ref, static_cast<long>(sizeof ref)); break;
					case Bind::Role::Num:
						switch (b.field) {
						case Field::Line:      insert->SetParamNumber(p++, ibNumber(line + 1)); break;
						case Field::Warehouse: insert->SetParamNumber(p++, ibNumber(wh)); break;
						case Field::Item:      insert->SetParamNumber(p++, ibNumber(it)); break;
						case Field::Quantity:  insert->SetParamNumber(p++, ibNumber(qty)); break;
						default:
							fail(wxT("number slot on an unexpected field"));
							db->CloseStatement(insert);
							return;
						}
						break;
					}
				}
				insert->RunQuery();
			}
			if ((d + 1) % 500 == 0) {
				db->Commit();
				std::fprintf(stderr, "seeded %ld / %ld documents\n", d + 1, documents);
				if (d + 1 < documents)
					db->BeginTransaction();
			}
		}
		if (db->IsActiveTransaction())
			db->Commit();
	}
	catch (...) {
		if (db->IsActiveTransaction()) {
			try { db->RollBack(); } catch (...) {}
		}
		db->CloseStatement(insert);
		fail(wxT("insert failed: ") + CatchText());
		return;
	}
	db->CloseStatement(insert);
	report.seedMs = MsSince(seedT0);
	report.rssAfterSeedKb = VmRssKb();

	{
		ibDatabaseResultSet* rs = db->RunQueryWithResults(
			wxT("SELECT COUNT(*) AS c FROM %s"), table);
		if (rs != nullptr && rs->Next())
			report.counted = rs->GetResultLong(wxT("c"));
		if (rs != nullptr)
			db->CloseResultSet(rs);
	}
	if (report.counted != rows) {
		fail(wxString::Format(wxT("COUNT(*) = %lld, expected %ld"), report.counted, rows));
		return;
	}

	const std::vector<ibColumnSlot> whSlots = warehouse->GetQueryColumn()->DescribeLayout();
	wxString whType, whNum;
	for (const ibColumnSlot& s : whSlots) {
		if (s.m_role == ibColumnRole::Discriminator) whType = s.m_name;
		if (s.m_role == ibColumnRole::Number)        whNum  = s.m_name;
	}
	{
		ibDatabaseResultSet* rs = db->RunQueryWithResults(
			wxT("SELECT COUNT(*) AS c FROM %s WHERE %s = %d AND %s = %d"),
			table, whType, static_cast<int>(ibFieldTypes_Number), whNum, 7);
		if (rs != nullptr && rs->Next())
			report.warehouse7 = rs->GetResultLong(wxT("c"));
		if (rs != nullptr)
			db->CloseResultSet(rs);
	}

	const auto analyzeT0 = std::chrono::steady_clock::now();
	db->RunStatement(wxT("ANALYZE"));
	report.analyzeMs = MsSince(analyzeT0);

	Ids ids;
	ids.period    = reg->GetRegisterPeriod()->GetMetaID();
	ids.line      = reg->GetRegisterLineNumber()->GetMetaID();
	ids.warehouse = warehouse->GetMetaID();
	ids.item      = item->GetMetaID();
	ids.quantity  = quantity->GetMetaID();
	ids.recorder  = reg->GetRegisterRecorder()->GetMetaID();

	const ibBackendQueryable* queryable = reg->GetQueryable();
	{
		std::unique_ptr<ibValueDynamicList> probe(
			ibCreateList(queryable, reg->GetRegisterPeriod()->GetQueryColumn()));
		if (probe == nullptr || probe->GetSourceQueryable() == nullptr) {
			fail(wxT("list source did not resolve"));
			return;
		}
	}
	auto consider = [&](Scenario s) {
		std::fprintf(stderr, "%s  rows=%d  ms=%.1f  %s\n",
			s.name.c_str(), s.rows, s.ms, s.ok ? "ok" : "FAIL");
		if (!s.ok)
			ADD_FAILURE() << s.name << ": " << s.note;
		report.scenarios.push_back(std::move(s));
	};

	// --- forward, the default Period sort the list form installs -----------
	{
		std::unique_ptr<ibValueDynamicList> list(
			ibCreateList(queryable, reg->GetRegisterPeriod()->GetQueryColumn()));
		Scenario sc;
		sc.name = "First page (Period ascending, 100 rows)";
		Page page = Fetch(list.get(), ids, false, ibDataViewItem());
		sc.ms = page.ms;
		sc.rows = static_cast<int>(page.rows.size());
		std::string why;
		sc.ok = page.error.empty() && sc.rows == kPageSize && PeriodOrdered(page.rows, true, why);
		if (!page.error.empty())
			sc.note = std::string(page.error.ToUTF8());
		else if (!why.empty())
			sc.note = why;
		else if (sc.rows != kPageSize)
			sc.note = "page size " + std::to_string(sc.rows);
		if (sc.ok && page.rows.front().recorder != recorderClsid) {
			sc.ok = false;
			sc.note = "recorder clsid read back as " + std::to_string(page.rows.front().recorder)
				+ ", expected " + std::to_string(recorderClsid);
		}
		consider(sc);

		std::vector<double> pageMs;
		pageMs.push_back(page.ms);
		ibDataViewItem anchor = page.last;
		Sig prev = page.rows.empty() ? Sig{} : page.rows.back();
		Page second;
		bool boundaryOk = true;
		std::string boundaryWhy;
		for (int n = 1; n < kForwardPages && boundaryOk && page.error.empty(); ++n) {
			Page next = Fetch(list.get(), ids, false, anchor);
			pageMs.push_back(next.ms);
			if (!next.error.empty() || next.rows.size() != static_cast<size_t>(kPageSize)) {
				boundaryOk = false;
				boundaryWhy = next.error.empty()
					? "page " + std::to_string(n) + " returned " + std::to_string(next.rows.size())
					: std::string(next.error.ToUTF8());
				break;
			}
			std::string orderWhy;
			if (!PeriodOrdered(next.rows, true, orderWhy) || !(prev.period <= next.rows.front().period)
				|| prev == next.rows.front()) {
				boundaryOk = false;
				boundaryWhy = orderWhy.empty()
					? "page " + std::to_string(n) + " repeated the anchor or stepped backward"
					: orderWhy;
				break;
			}
			if (n == 1)
				second = next;
			prev = next.rows.back();
			anchor = next.last;
		}
		Scenario walk;
		walk.name = "100 forward pages (keyset)";
		walk.rows = static_cast<int>(pageMs.size()) * kPageSize;
		walk.ms = 0;
		for (double m : pageMs) walk.ms += m;
		walk.ok = boundaryOk && pageMs.size() == static_cast<size_t>(kForwardPages);
		const double first = pageMs.empty() ? 0 : pageMs.front();
		const double last  = pageMs.empty() ? 0 : pageMs.back();
		double worst = 0;
		for (double m : pageMs) worst = std::max(worst, m);
		walk.note = "first page " + std::to_string(first) + " ms, last page " + std::to_string(last)
			+ " ms, slowest " + std::to_string(worst) + " ms";
		if (!boundaryWhy.empty())
			walk.note += ". " + boundaryWhy;
		if (walk.ok && last > first * 20.0 && last > 500.0) {
			walk.ok = false;
			walk.note += ". The last page is far slower than the first — the keyset looks like a scan.";
		}
		consider(walk);

		if (second.first.IsOk() && !page.rows.empty()) {
			Scenario back;
			back.name = "Page back from the second page";
			Page prevPage = Fetch(list.get(), ids, true, second.first);
			back.ms = prevPage.ms;
			back.rows = static_cast<int>(prevPage.rows.size());
			back.ok = prevPage.error.empty() && prevPage.rows.size() == page.rows.size()
				&& prevPage.rows.front() == page.rows.front()
				&& prevPage.rows.back() == page.rows.back();
			auto brief = [](const Sig& s) {
				return "p=" + std::to_string(s.period) + " line=" + std::to_string(s.line)
					+ " wh=" + std::to_string(s.warehouse) + " item=" + std::to_string(s.item);
			};
			if (!prevPage.error.empty())
				back.note = std::string(prevPage.error.ToUTF8());
			else if (!back.ok) {
				back.note = "the page before the second page is not the first page. page1 "
					+ brief(page.rows.front()) + " .. " + brief(page.rows.back())
					+ "; page2 " + brief(second.rows.front()) + " .. " + brief(second.rows.back())
					+ "; back " + brief(prevPage.rows.empty() ? Sig{} : prevPage.rows.front())
					+ " .. " + brief(prevPage.rows.empty() ? Sig{} : prevPage.rows.back());
			}
			consider(std::move(back));
		}
	}

	auto filtered = [&](const wxString& title, const wxString& field, const wxString& op,
	                    const ibValue& value, auto check) {
		std::unique_ptr<ibValueDynamicList> list(
			ibCreateList(queryable, reg->GetRegisterPeriod()->GetQueryColumn()));
		Scenario sc;
		sc.name = std::string(title.ToUTF8());
		try {
			list->AddFilter(field, op, value);
		}
		catch (...) {
			sc.note = std::string(CatchText().ToUTF8());
			consider(std::move(sc));
			return;
		}
		Page page = Fetch(list.get(), ids, false, ibDataViewItem());
		sc.ms = page.ms;
		sc.rows = static_cast<int>(page.rows.size());
		if (!page.error.empty()) {
			sc.note = std::string(page.error.ToUTF8());
			consider(std::move(sc));
			return;
		}
		std::string why;
		sc.ok = check(page, why);
		sc.note = why;
		consider(std::move(sc));
	};

	filtered(wxT("Filter Warehouse = 7"), warehouse->GetName(), wxT("="), ibValue(ibNumber(7)),
		[&](const Page& page, std::string& why) {
			if (page.rows.size() != static_cast<size_t>(kPageSize)) {
				why = "expected a full page of matches";
				return false;
			}
			for (const Sig& s : page.rows) {
				if (!s.ok || s.warehouse != 7) {
					why = "a row was not Warehouse 7";
					return false;
				}
			}
			why = "SQL count of Warehouse 7 is " + std::to_string(report.warehouse7);
			return true;
		});

	filtered(wxT("Filter Quantity >= 400"), quantity->GetName(), wxT(">="), ibValue(ibNumber(400)),
		[&](const Page& page, std::string& why) {
			if (page.rows.empty()) {
				why = "the predicate matched nothing";
				return false;
			}
			for (const Sig& s : page.rows) {
				if (!s.ok || s.quantity < 400) {
					why = "a row was below 400";
					return false;
				}
			}
			return true;
		});

	filtered(wxT("Filter Warehouse = 99 (empty)"), warehouse->GetName(), wxT("="), ibValue(ibNumber(99)),
		[&](const Page& page, std::string& why) {
			if (!page.rows.empty()) {
				why = "an impossible warehouse returned rows";
				return false;
			}
			return true;
		});

	// Day 700 is inside a million-row load (10 000 documents walk the 730-day cycle).
	// A shorter load only reaches document-count days, so the bound stays inside the data.
	const int boundDay = std::min(700, static_cast<int>(documents) - 1);
	const wxDateTime late = dates[static_cast<size_t>(boundDay)];
	const long long lateKey = ibValue(late).GetDate();
	filtered(wxString::Format(wxT("Filter Period >= day %d"), boundDay),
		reg->GetRegisterPeriod()->GetName(), wxT(">="), ibValue(late),
		[&](const Page& page, std::string& why) {
			if (page.rows.empty()) {
				why = "the seek matched nothing";
				return false;
			}
			if (page.rows.front().period < lateKey) {
				why = "the first row is earlier than the bound";
				return false;
			}
			return true;
		});

	{
		std::unique_ptr<ibValueDynamicList> list(new ibValueDynamicList(queryable));
		Scenario sc;
		sc.name = "Sort Quantity descending";
		bool sorted = false;
		try {
			list->AddSort(quantity->GetName(), /*ascending*/ false);
			sorted = true;
		}
		catch (...) {
			sc.note = std::string(CatchText().ToUTF8());
			consider(std::move(sc));
		}
		if (sorted) {
			Page page = Fetch(list.get(), ids, false, ibDataViewItem());
			sc.ms = page.ms;
			sc.rows = static_cast<int>(page.rows.size());
			sc.ok = page.error.empty() && !page.rows.empty() && page.rows.front().quantity == 500;
			if (!page.error.empty())
				sc.note = std::string(page.error.ToUTF8());
			else if (!sc.ok)
				sc.note = "first quantity is " + std::to_string(page.rows.empty() ? -1 : page.rows.front().quantity)
					+ ", expected 500";
			else {
				for (size_t i = 1; i < page.rows.size(); ++i) {
					if (page.rows[i].quantity > page.rows[i - 1].quantity) {
						sc.ok = false;
						sc.note = "quantity rose inside a descending page";
						break;
					}
				}
			}
			// One more page, still descending, and not a repeat of the anchor.
			if (sc.ok) {
				Page next = Fetch(list.get(), ids, false, page.last);
				sc.ms += next.ms;
				if (!next.error.empty() || next.rows.empty() || next.rows.front() == page.rows.back()
					|| next.rows.front().quantity > page.rows.back().quantity) {
					sc.ok = false;
					sc.note = next.error.empty()
						? "the next Quantity page repeated the anchor or increased"
						: std::string(next.error.ToUTF8());
				}
			}
			consider(std::move(sc));
		}
	}

	report.rssAfterListKb = VmRssKb();
	report.notes.push_back(
		"Scrollbar snap-to-end is unimplemented (datavgen.cpp: no model API for the last batch). "
		"Left as a known limit.");
	report.notes.push_back(
		"A dotted sort still fetches the whole ordered result (tabularModelDb.cpp). "
		"Not exercised; the sorts above are columns of the register.");
}
