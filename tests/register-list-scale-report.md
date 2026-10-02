# Register list at one million movements

Headless pass of the accumulation-register list. The object under test is
`ibCreateList` on the register's movements queryable — the same value
`ibValueMetaObjectAccumulationRegister::GetListForm` puts on the form.
Paging, filters and sorts go through `GetFirstFetch`, `GetNextFetch`,
`GetPrevFetch`, `AddFilter` and `AddSort`, which is the door
`datavgen.paged.cpp` calls. No window was opened.

## How to reproduce

```bash
cmake --preset linux-release -DBUILD_TESTING=ON
cmake --build build/linux-release --target oes_register_list_scale -j 2
OES_REGISTER_ROWS=1000000 \
OES_REGISTER_DB=/tmp/oes-goods-movements.db \
OES_REGISTER_REPORT=tests/register-list-scale-report.md \
  ./build/linux-release/bin/Release/oes_register_list_scale
```

The binary is not registered with ctest. `OES_REGISTER_ROWS` is rounded
down to a multiple of 100 (one document is 100 lines). A smaller value
is only for debugging the harness.

## What was loaded

Accumulation register **Goods**, recorder document **GoodsReceipt**.
Dimensions Warehouse and Item (number, indexed), resource Quantity
(number, indexed). Period, Active, RecordType, Recorder and LineNumber
are the register's own columns. Ten thousand documents would be one
million lines; this run asked for **1000000** lines.

Distribution, deterministic: period is 2024-01-01 plus `(document % 730)`
days (a document's lines share a day); line number is 1..100; warehouse
is `(document + line) % 20`; item is `(document * 3 + line) % 50`;
quantity is `1 + ((document * 100 + line) % 500)` so the range is 1..500.
Warehouse 7 therefore matches 5 lines of every document.

Schema is `BuildSchemaSnapshot` plus the create-all save (`OnSave` from an
empty baseline) on a file SQLite database (`/tmp/oes-goods-movements.db`).
Physical table: `AccumulationRegister1015`.
Split totals is off, and every SQLite trigger is dropped after DDL, so
the insert does not maintain balance or turnover tables. The list does
not read those tables. Totals upkeep is out of scope.

| Step | ms |
|---|---:|
| Schema | 6.46813 |
| Insert | 5334.74 |
| ANALYZE | 290.872 |

COUNT(*) = 1000000. Rows with Warehouse = 7: 50000.
VmRSS after the insert: 342296 kB. After the list pass: 343180 kB.

## List pass

| Check | Rows | ms | Result |
|---|---:|---:|---|
| First page (Period ascending, 100 rows) | 100 | 81.6804 | pass |
| 100 forward pages (keyset) | 10000 | 19651.5 | pass |
| Page back from the second page | 100 | 2.60246 | pass |
| Filter Warehouse = 7 | 100 | 31.6665 | pass |
| Filter Quantity >= 400 | 100 | 100.715 | pass |
| Filter Warehouse = 99 (empty) | 0 | 0.203014 | pass |
| Filter Period >= day 700 | 100 | 2.58438 | pass |
| Sort Quantity descending | 100 | 746.52 | pass |

- **100 forward pages (keyset).** first page 81.680434 ms, last page 193.281972 ms, slowest 233.058278 ms
- **Filter Warehouse = 7.** SQL count of Warehouse 7 is 50000

- Dropped 3 SQLite triggers after DDL so the insert does not maintain totals.
- Scrollbar snap-to-end is unimplemented (datavgen.cpp: no model API for the last batch). Left as a known limit.
- A dotted sort still fetches the whole ordered result (tabularModelDb.cpp). Not exercised; the sorts above are columns of the register.

## What a person still has to do in the UI

This process cannot host the desktop client. On a machine with a display,
against a Firebird or PostgreSQL base (SQLite is the test driver, not the
one Enterprise opens):

1. Create the same Goods register, or point a loader at an existing one,
   and insert on the order of 10^6 movements.
2. Open the register's list form (the generated list, not a hand-built form).
3. Scroll forward and back with the wheel and the scrollbar. The first
   pages and a page a long way down should arrive in about the same time;
   a pause that grows with the scroll position means the keyset is scanning.
4. Click the Period, Quantity and Warehouse column headers and confirm the
   order and that the next page continues it.
5. Open the filter and restrict Warehouse, Quantity and Period. The grid
   must show only matching rows, and an impossible value must show an empty list.

## Defects

SQLite bound every number as a double and read a 64-bit integer back through
`sqlite3_column_int`. A reference class id near 2^60 does not survive that, and
a register list keys its page on the recorder reference. Integer values now bind
and read as integers; a fraction still goes through the double path.

A dynamic list remembered its source as a table id and resolved that id through
the property owner's configuration. A list that is not on a form answers with the
active configuration, which is a different object from the one that registered the
register. The resolve missed, `GetSourceQueryable()` was null, and the first page
came back empty in a fraction of a millisecond with no error. The cell now keeps
the configuration the queryable itself names. The configuration still has to be
run (`RunDatabase`) so the source is registered; this harness does that after DDL.

SQLite bound a blob with `SQLITE_STATIC`, and the cursor steps when the caller
reads it, after that buffer is gone. A page that ties on a reference — many
movements share a period — compared a dead guid and jumped to the next distinct
sort value. The binder now copies the bytes, the same way it already copies text.

Dragging the scrollbar to the end is not implemented. `datavgen.cpp` says so:
a snap to the bottom would need N forward fetches, and there is no model API
for the last batch. That waits on async fetch. It is not fixed here.

Sorting by a dotted path (Recorder's number, for example) loads the whole
ordered result in one shot (`tabularModelDb.cpp`, the dot-walk branch).
A sort on a column of the register itself keyset-pages. The checks above
use columns, not dotted paths.
