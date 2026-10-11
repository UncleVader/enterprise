// =============================================================================
// OES Enterprise — ibValueArray tests
//
// ibValueArray (backend/system/value/valueArray.h) is the script Array value:
// Add / Insert / Remove / Find / Contains / Sort / Sum over a vector<ibValue>.
// The LINQ pipeline and block-syntax orderby/distinct lean on it. Pure (no DB).
//
// RemoveByIndex is a regression guard: Remove(index) used to std::find the
// element whose VALUE equalled `index` (remove-by-value) instead of erasing the
// element AT that index — now fixed.
// =============================================================================

#include <gtest/gtest.h>
#include <vector>
#include "backend/system/value/valueArray.h"
#include "core/serialize/dataBuilder.h"

TEST(ValueArray, EmptyByDefault) {
    ibValueArray a;
    EXPECT_TRUE(a.IsEmpty());
    EXPECT_EQ(a.Count(), 0u);
}

TEST(ValueArray, AddIncreasesCount) {
    ibValueArray a;
    a.Add(ibValue(ibNumber(1)));
    a.Add(ibValue(wxString(wxT("x"))));
    EXPECT_FALSE(a.IsEmpty());
    EXPECT_EQ(a.Count(), 2u);
}

TEST(ValueArray, ContainsAndFind) {
    ibValueArray a;
    a.Add(ibValue(ibNumber(10)));
    a.Add(ibValue(ibNumber(20)));
    EXPECT_TRUE(a.Contains(ibValue(ibNumber(20))));
    EXPECT_FALSE(a.Contains(ibValue(ibNumber(99))));
    EXPECT_EQ(a.Find(ibValue(ibNumber(20))).GetInteger(), 1);   // index 1
    EXPECT_TRUE(a.Find(ibValue(ibNumber(99))).IsEmpty());        // miss -> TYPE_EMPTY
}

TEST(ValueArray, InsertAtIndex) {
    ibValueArray a;
    a.Add(ibValue(ibNumber(1)));
    a.Add(ibValue(ibNumber(3)));
    a.Insert(1, ibValue(ibNumber(2)));                           // [1, 2, 3]
    EXPECT_EQ(a.Count(), 3u);
    EXPECT_EQ(a.Find(ibValue(ibNumber(2))).GetInteger(), 1);
}

TEST(ValueArray, RemoveByIndex) {
    ibValueArray a;
    a.Add(ibValue(ibNumber(10)));
    a.Add(ibValue(ibNumber(20)));
    a.Add(ibValue(ibNumber(30)));
    a.Remove(1);                                                 // element AT index 1 (value 20)
    EXPECT_EQ(a.Count(), 2u);
    EXPECT_TRUE (a.Contains(ibValue(ibNumber(10))));
    EXPECT_FALSE(a.Contains(ibValue(ibNumber(20))));            // the removed one
    EXPECT_TRUE (a.Contains(ibValue(ibNumber(30))));
}

TEST(ValueArray, ClearEmpties) {
    ibValueArray a;
    a.Add(ibValue(ibNumber(1)));
    a.Clear();
    EXPECT_TRUE(a.IsEmpty());
}

TEST(ValueArray, SortAscending) {
    ibValueArray a;
    a.Add(ibValue(ibNumber(3)));
    a.Add(ibValue(ibNumber(1)));
    a.Add(ibValue(ibNumber(2)));
    a.Sort();
    EXPECT_EQ(a.Find(ibValue(ibNumber(1))).GetInteger(), 0);
    EXPECT_EQ(a.Find(ibValue(ibNumber(3))).GetInteger(), 2);
}

TEST(ValueArray, ConstructFromVector) {
    std::vector<ibValue> v{ ibValue(ibNumber(5)), ibValue(ibNumber(6)) };
    ibValueArray a(v);
    EXPECT_EQ(a.Count(), 2u);
    EXPECT_TRUE(a.Contains(ibValue(ibNumber(6))));
}

TEST(ValueStorage, GetReturnsWhatWasStoredAndTheValueRoundTrips) {
    ibValue held(wxString(wxT("kept")));
    ibValue* args[] = { &held };
    ibValue box = ibValue::CreateObject(wxT("ValueStorage"), args, 1);
    ibValue got;
    ASSERT_TRUE(box.CallAsFunc(box.FindMethod(wxT("Get")), got, nullptr, 0));
    EXPECT_EQ(got.GetString(), wxString(wxT("kept")));
    ibDataNode node;
    ASSERT_TRUE(box.Serialize(node));
    ibValue restored = ibValue::FromNode(node);
    ibValue again;
    ASSERT_TRUE(restored.CallAsFunc(restored.FindMethod(wxT("Get")), again, nullptr, 0));
    EXPECT_EQ(again.GetString(), wxString(wxT("kept")));
    ibValue empty = ibValue::CreateObject(wxT("ValueStorage"));
    ibValue blank;
    ASSERT_TRUE(empty.CallAsFunc(empty.FindMethod(wxT("Get")), blank, nullptr, 0));
    EXPECT_TRUE(blank.IsEmpty());
}

TEST(ValueArray, SumNumeric) {
    ibValueArray a;
    a.Add(ibValue(ibNumber(10)));
    a.Add(ibValue(ibNumber(20)));
    a.Add(ibValue(ibNumber(30)));
    EXPECT_EQ(a.Sum().GetInteger(), 60);
}
