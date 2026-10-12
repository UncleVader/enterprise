// =============================================================================
// An index that covers a retyped column is not the same index.
//
// config_apply changed a register dimension's type and then ALTERed the column
// while the key index still stood on it. Firebird refuses that ("column ... is
// referenced in index") and rolls the whole apply back. The differ already drops
// an index and rebuilds it around a column change when it can see that the index
// changed. It compared columns by model id only, and a type change keeps the id.
//
// The predicate below is what the differ asks. False means the index comes down
// before the column is altered and goes back up after.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/query/queryColumn.h"
#include "backend/query/schemaSnapshot.h"

namespace {

ibSchemaIndex IndexOver(const ibBackendQueryColumn* column)
{
    ibSchemaIndex index;
    index.m_name = wxT("INFORMATIONREGISTER_INDEX");
    index.m_unique = true;
    index.m_columns.push_back(column);
    return index;
}

} // namespace

TEST(SchemaIndex, TheSameColumnAtTheSameTypeIsTheSameIndex)
{
    const ibBackendColumnRawDB was = ibBackendColumnRawDB::String(wxT("D"), 7, 10);
    const ibBackendColumnRawDB now = ibBackendColumnRawDB::String(wxT("D"), 7, 10);
    EXPECT_TRUE(ibSameSchemaIndex(IndexOver(&was), IndexOver(&now)));
}

TEST(SchemaIndex, ARetypedColumnIsNotTheSameIndex)
{
    const ibBackendColumnRawDB text = ibBackendColumnRawDB::String(wxT("D"), 7, 10);
    const ibBackendColumnRawDB number = ibBackendColumnRawDB::Number(wxT("D"), 7, 15, 2);
    const ibBackendColumnRawDB reference = ibBackendColumnRawDB::Reference(wxT("D"), 0, wxEmptyString, 7);

    EXPECT_FALSE(ibSameSchemaIndex(IndexOver(&text), IndexOver(&number)));
    EXPECT_FALSE(ibSameSchemaIndex(IndexOver(&text), IndexOver(&reference)));
}

TEST(SchemaIndex, AWiderStringIsNotTheSameIndex)
{
    const ibBackendColumnRawDB narrow = ibBackendColumnRawDB::String(wxT("D"), 7, 10);
    const ibBackendColumnRawDB wide = ibBackendColumnRawDB::String(wxT("D"), 7, 50);
    EXPECT_FALSE(ibSameSchemaIndex(IndexOver(&narrow), IndexOver(&wide)));
}

// A non-unique attribute index (<table>_<field>_IX), not the key index. Indexing = Index
// on a register attribute builds this one. A type change keeps the model id, and Firebird
// refuses ALTER while INFORMATIONREGISTER7295_7305_IX still stands on FLD7305_S. The same
// predicate covers it: the name is not what the differ asks, the physical layout is.
TEST(SchemaIndex, AnAttributeIndexIsNotTheSameAfterARetype)
{
    const ibBackendColumnRawDB text = ibBackendColumnRawDB::String(wxT("D"), 7305, 10);
    const ibBackendColumnRawDB reference = ibBackendColumnRawDB::Reference(wxT("D"), 0, wxEmptyString, 7305);

    ibSchemaIndex was;
    was.m_name = wxT("INFORMATIONREGISTER7295_7305_IX");
    was.m_unique = false;
    was.m_columns.push_back(&text);

    ibSchemaIndex standing = was;
    const ibBackendColumnRawDB stillText = ibBackendColumnRawDB::String(wxT("D"), 7305, 10);
    standing.m_columns.clear();
    standing.m_columns.push_back(&stillText);
    EXPECT_TRUE(ibSameSchemaIndex(was, standing));

    ibSchemaIndex retyped = was;
    retyped.m_columns.clear();
    retyped.m_columns.push_back(&reference);
    EXPECT_FALSE(ibSameSchemaIndex(was, retyped));
}
