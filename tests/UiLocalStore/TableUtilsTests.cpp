#include "TableUtilsTests.hpp"

#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include "UiLocalStore/TableUtils.hpp"

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_TableUtilsTests)

using namespace TradingSerialization::Table;

struct TableUtilsTests : public BaseTestFixture, public TableUtilsTestsBase
{
    using TTableUtils = TableUtils<QuoterSettings::QuoterSettingsTableSetup>;

    TableUtilsTests()
    {
    }

    template <typename TEnum, typename TFunction>
    void EnumTest(TEnum aMaxValue, TFunction aFunction, TColumnType aStartValue = 0)
    {
        BOOST_TEST_CONTEXT("Incorrect negative values")
        {
            BOOST_CHECK(!aFunction(aStartValue - 1));
        }

        BOOST_TEST_CONTEXT("Correct values")
        {
            TColumnType column = aStartValue;
            while (aFunction(column))
            {
                ++column;
            }
            BOOST_CHECK_EQUAL(column - 1, static_cast<TColumnType>(aMaxValue));
        }
    }

    template <typename TEnum, typename TFunction>
    void EnumValidationTest(
        TEnum aMaxValue,
        TFunction aFunction,
        const std::string& aErrorPrefix,
        TColumnType aStartValue = 0)
    {
        BOOST_TEST_CONTEXT("Incorrect negative values")
        {
            BOOST_CHECK_EQUAL(
                aErrorPrefix + Basis::ToString(aStartValue - 1),
                aFunction(static_cast<TEnum>(aStartValue - 1)));
        }

        BOOST_TEST_CONTEXT("Correct values")
        {
            int64_t index = aStartValue;
            while (aFunction(static_cast<TEnum>(index)) == TTableUtils::Ok)
            {
                ++index;
            }
            BOOST_CHECK_EQUAL(index - 1, static_cast<TColumnType>(aMaxValue));
        }
    }
};

BOOST_FIXTURE_TEST_CASE(GetColumnIndexTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("String filter")
    {
        auto index = TTableUtils::GetColumnIndex<TFilterMplVector, TString>();
        BOOST_CHECK_EQUAL(0, index);
    }
    BOOST_TEST_CONTEXT("Int filter")
    {
        auto index = TTableUtils::GetColumnIndex<TFilterMplVector, TInt>();
        BOOST_CHECK_EQUAL(1, index);
    }
    BOOST_TEST_CONTEXT("Float filter")
    {
        auto index = TTableUtils::GetColumnIndex<TFilterMplVector, TFloat>();
        BOOST_CHECK_EQUAL(2, index);
    }
    BOOST_TEST_CONTEXT("Unknown filter")
    {
        auto index = TTableUtils::GetColumnIndex<TFilterMplVector, TableUtilsTests>();
        BOOST_CHECK_EQUAL(3, index);
    }
}

BOOST_FIXTURE_TEST_CASE(GetFilterValueIndexTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("String filter")
    {
        auto index = TTableUtils::GetFilterValueIndex<TString>();
        BOOST_CHECK_EQUAL(FilterValueType::String, index);
    }
    BOOST_TEST_CONTEXT("Int filter")
    {
        auto index = TTableUtils::GetFilterValueIndex<TInt>();
        BOOST_CHECK_EQUAL(FilterValueType::Int, index);
    }
    BOOST_TEST_CONTEXT("Float filter")
    {
        auto index = TTableUtils::GetFilterValueIndex<TFloat>();
        BOOST_CHECK_EQUAL(FilterValueType::Float, index);
    }
    BOOST_TEST_CONTEXT("Unknown filter")
    {
        auto index = TTableUtils::GetFilterValueIndex<TableUtilsTests>();
        BOOST_CHECK_EQUAL(FilterValueType::Unknown, index);
    }
}

BOOST_FIXTURE_TEST_CASE(FilterTypeVisitorTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("String filter")
    {
        TCellVariant variant = std::string {"text"};
        auto type = boost::apply_visitor(TTableUtils::FilterTypeVisitor {}, variant);
        BOOST_CHECK_EQUAL(FilterValueType::String, type);
    }
    BOOST_TEST_CONTEXT("Int filter")
    {
        TCellVariant variant = TInt(100005);
        auto type = boost::apply_visitor(TTableUtils::FilterTypeVisitor {}, variant);
        BOOST_CHECK_EQUAL(FilterValueType::Int, type);
    }
    BOOST_TEST_CONTEXT("Float filter")
    {
        TCellVariant variant = TFloat(1.0001);
        auto type = boost::apply_visitor(TTableUtils::FilterTypeVisitor {}, variant);
        BOOST_CHECK_EQUAL(FilterValueType::Float, type);
    }
    BOOST_TEST_CONTEXT("Default filter")
    {
        TCellVariant variant;
        auto type = boost::apply_visitor(TTableUtils::FilterTypeVisitor {}, variant);
        BOOST_CHECK_EQUAL(FilterValueType::String, type);
    }
}

BOOST_FIXTURE_TEST_CASE(ValidateEnumValueTests, TableUtilsTests)
{
    enum class TestEnum
    {
        Value1 = 0,
        Value2,
        Value3
    };

    BOOST_TEST_CONTEXT("Correct Three State Enum")
    {
        BOOST_CHECK(TTableUtils::ValidateEnumValue(TestEnum::Value1, TestEnum::Value1, TestEnum::Value3));
        BOOST_CHECK(TTableUtils::ValidateEnumValue(TestEnum::Value2, TestEnum::Value1, TestEnum::Value3));
        BOOST_CHECK(TTableUtils::ValidateEnumValue(TestEnum::Value3, TestEnum::Value1, TestEnum::Value3));

        BOOST_CHECK(TTableUtils::ValidateEnumValue(0, TestEnum::Value1, TestEnum::Value3));
        BOOST_CHECK(TTableUtils::ValidateEnumValue(1, TestEnum::Value1, TestEnum::Value3));
        BOOST_CHECK(TTableUtils::ValidateEnumValue(2, TestEnum::Value1, TestEnum::Value3));
    }

    BOOST_TEST_CONTEXT("Incorrect Three State Enum")
    {
        BOOST_CHECK(!TTableUtils::ValidateEnumValue(-1, TestEnum::Value1, TestEnum::Value3));
        BOOST_CHECK(!TTableUtils::ValidateEnumValue(3, TestEnum::Value1, TestEnum::Value3));
    }

    BOOST_TEST_CONTEXT("Correct One State Enum")
    {
        BOOST_CHECK(TTableUtils::ValidateEnumValue(TestEnum::Value2, TestEnum::Value2, TestEnum::Value2));
        BOOST_CHECK(TTableUtils::ValidateEnumValue(1, TestEnum::Value2, TestEnum::Value2));
    }

    BOOST_TEST_CONTEXT("Incorrect One State Enum")
    {
        BOOST_CHECK(!TTableUtils::ValidateEnumValue(TestEnum::Value1, TestEnum::Value2, TestEnum::Value2));
        BOOST_CHECK(!TTableUtils::ValidateEnumValue(TestEnum::Value3, TestEnum::Value2, TestEnum::Value2));
    }
}

BOOST_FIXTURE_TEST_CASE(IsOperationAllowedTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("Correct String operations")
    {
        BOOST_CHECK(TTableUtils::IsOperationAllowed(FilterValueType::String, FilterOperator::In));
    }

    BOOST_TEST_CONTEXT("Incorrect String operations")
    {
        BOOST_CHECK(!TTableUtils::IsOperationAllowed(FilterValueType::String, FilterOperator::GreaterEq));
        BOOST_CHECK(!TTableUtils::IsOperationAllowed(FilterValueType::String, FilterOperator::LessEq));
    }

    BOOST_TEST_CONTEXT("Correct Int operations")
    {
        BOOST_CHECK(TTableUtils::IsOperationAllowed(FilterValueType::Int, FilterOperator::In));
        BOOST_CHECK(TTableUtils::IsOperationAllowed(FilterValueType::Int, FilterOperator::GreaterEq));
        BOOST_CHECK(TTableUtils::IsOperationAllowed(FilterValueType::Int, FilterOperator::LessEq));
    }

    BOOST_TEST_CONTEXT("Correct Float operations")
    {
        BOOST_CHECK(TTableUtils::IsOperationAllowed(FilterValueType::Float, FilterOperator::In));
        BOOST_CHECK(TTableUtils::IsOperationAllowed(FilterValueType::Float, FilterOperator::GreaterEq));
        BOOST_CHECK(TTableUtils::IsOperationAllowed(FilterValueType::Float, FilterOperator::LessEq));
    }
}

BOOST_FIXTURE_TEST_CASE(ColumnTypeIsValidTests, TableUtilsTests)
{
    EnumTest(MaxColumnType, TTableUtils::ColumnTypeIsValid);
}

BOOST_FIXTURE_TEST_CASE(FilterColumnTypeIsValidTests, TableUtilsTests)
{
    EnumTest(Filters::FullTextSearch, TTableUtils::FilterColumnTypeIsValid, StartFilterValue);
}

BOOST_FIXTURE_TEST_CASE(ValidateColumnTests, TableUtilsTests)
{
    BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateColumn(0));
    BOOST_CHECK_EQUAL("Wrong column number: -1", TTableUtils::ValidateColumn(-1));
    BOOST_CHECK_EQUAL("Wrong column number: 1000", TTableUtils::ValidateColumn(StartFilterValue));
    BOOST_CHECK_EQUAL("Wrong column number: 999", TTableUtils::ValidateColumn(StartFilterValue - 1));
    BOOST_CHECK_EQUAL("Wrong column number: 1001", TTableUtils::ValidateColumn(StartFilterValue + 1));
}

BOOST_FIXTURE_TEST_CASE(ValidateFilterColumnTests, TableUtilsTests)
{
    BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateFilterColumn(0));
    BOOST_CHECK_EQUAL("Wrong filter column number: -1", TTableUtils::ValidateFilterColumn(-1));
    BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateFilterColumn(StartFilterValue));
    BOOST_CHECK_EQUAL("Wrong filter column number: 999", TTableUtils::ValidateFilterColumn(StartFilterValue - 1));
    BOOST_CHECK_EQUAL("Wrong filter column number: 1001", TTableUtils::ValidateFilterColumn(StartFilterValue + 1));
}

BOOST_FIXTURE_TEST_CASE(ValidateRelationTests, TableUtilsTests)
{
    EnumValidationTest(FilterRelation::And, TTableUtils::ValidateRelation, "Wrong filter relation: ");
}

BOOST_FIXTURE_TEST_CASE(ValidateOperatorTests, TableUtilsTests)
{
    EnumValidationTest(FilterOperator::LessEq, TTableUtils::ValidateOperator, "Wrong filter operator: ");
}

BOOST_FIXTURE_TEST_CASE(GetMaxFilterValuesNumberForOperatorTests, TableUtilsTests)
{
    BOOST_CHECK_EQUAL(
        std::numeric_limits<size_t>::max(),
        TTableUtils::GetMaxFilterValuesNumberForOperator(FilterOperator::In));
    BOOST_CHECK_EQUAL(
        1,
        TTableUtils::GetMaxFilterValuesNumberForOperator(FilterOperator::GreaterEq));
    BOOST_CHECK_EQUAL(
        1,
        TTableUtils::GetMaxFilterValuesNumberForOperator(FilterOperator::LessEq));
}

BOOST_FIXTURE_TEST_CASE(GetColumnInfoTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("Correct columns")
    {
        Compare(
            ColumnsInfo { QuoterSettings::ColumnType::Id, FilterValueType::Int },
            TTableUtils::GetColumnsInfo(static_cast<TColumnType>(QuoterSettings::ColumnType::Id)));
        Compare(
            ColumnsInfo { MaxColumnType, FilterValueType::Float },
            TTableUtils::GetColumnsInfo(static_cast<TColumnType>(MaxColumnType)));
        Compare(
            ColumnsInfo { Filters::FullTextSearch, FilterValueType::String },
            TTableUtils::GetColumnsInfo(static_cast<TColumnType>(Filters::FullTextSearch)));
    }
    BOOST_TEST_CONTEXT("Incorrect columns")
    {
        BOOST_CHECK(!TTableUtils::GetColumnsInfo(-1));
        BOOST_CHECK(!TTableUtils::GetColumnsInfo(static_cast<TColumnType>(MaxColumnType) + 1));

        BOOST_CHECK(!TTableUtils::GetColumnsInfo(static_cast<TColumnType>(Filters::FullTextSearch) - 1));
        BOOST_CHECK(!TTableUtils::GetColumnsInfo(static_cast<TColumnType>(Filters::FullTextSearch) + 1));
    }
}

BOOST_FIXTURE_TEST_CASE(ValidateFilterTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("Correct filter")
    {
        BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateFilter(MakeIntFilter()));
        BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateFilter(MakeStringFilter()));
    }

    BOOST_TEST_CONTEXT("Incorrect operator")
    {
        Filter filter = MakeIntFilter();
        filter.Operator = static_cast<FilterOperator>(-1);

        BOOST_CHECK_EQUAL("Wrong filter operator: -1", TTableUtils::ValidateFilter(filter));
    }

    BOOST_TEST_CONTEXT("Incorrect empty values")
    {
        Filter filter = MakeIntFilter();
        filter.Values = ValueSet {};

        BOOST_CHECK_EQUAL("Filter empty", TTableUtils::ValidateFilter(filter));
    }

    BOOST_TEST_CONTEXT("Incorrect too many values")
    {
        Filter filter = MakeIntFilter();
        filter.Operator = FilterOperator::GreaterEq;

        BOOST_CHECK_EQUAL(2, filter.Values.Size());
        BOOST_CHECK_EQUAL("Too many filter values", TTableUtils::ValidateFilter(filter));
    }

    BOOST_TEST_CONTEXT("Incorrect column")
    {
        Filter filter = MakeIntFilter();
        filter.Column = -1;

        BOOST_CHECK_EQUAL("Wrong column", TTableUtils::ValidateFilter(filter));
    }

    BOOST_TEST_CONTEXT("Incorrect filter value")
    {
        Filter filter = MakeIntFilter();
        filter.Values = ValueSet {};
        filter.Values.Add(TString {"Incorrect"});

        BOOST_CHECK_EQUAL("Wrong filter type", TTableUtils::ValidateFilter(filter));
    }

    BOOST_TEST_CONTEXT("Incorrect operator for column type")
    {
        Filter filter = MakeStringFilter();
        filter.Operator = FilterOperator::GreaterEq;
        filter.Values.StringValues.pop_back();

        BOOST_CHECK_EQUAL(1, filter.Values.Size());
        BOOST_CHECK_EQUAL("Operator GreaterEq not allowed for type String", TTableUtils::ValidateFilter(filter));
    }
}

BOOST_FIXTURE_TEST_CASE(ValidateFilterGroupTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("Correct filter group")
    {
        BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateFilterGroup(MakerFilterGroup()));
    }

    BOOST_TEST_CONTEXT("Incorrect relation")
    {
        auto group = MakerFilterGroup();
        group.Relation = static_cast<FilterRelation>(-1);
        BOOST_CHECK_EQUAL("Wrong filter relation: -1", TTableUtils::ValidateFilterGroup(group));
    }

    BOOST_TEST_CONTEXT("Incorrect too many filters")
    {
        auto group = MakerFilterGroup();
        while (group.Filters.size() <= FilterGroup::MaxCount)
        {
            auto newFilter = *group.Filters.rbegin();
            newFilter.Column += 1;
            group.Filters.emplace(newFilter);
        }
        BOOST_CHECK_EQUAL("Too many filters: 101", TTableUtils::ValidateFilterGroup(group));
    }

    BOOST_TEST_CONTEXT("Incorrect wrong column")
    {
        auto group = MakerFilterGroup();
        auto filter = MakeIntFilter();
        filter.Column = -1;
        group.Filters.insert(filter);

        BOOST_CHECK_EQUAL("Wrong filter column number: -1", TTableUtils::ValidateFilterGroup(group));
    }

    BOOST_TEST_CONTEXT("Incorrect first filter")
    {
        auto group = MakerFilterGroup();
        auto it = group.Filters.begin();
        auto copy = *it;
        copy.Values = ValueSet {};
        group.Filters.erase(it);
        group.Filters.insert(copy);

        BOOST_CHECK_EQUAL("Filter empty", TTableUtils::ValidateFilterGroup(group));
    }

    BOOST_TEST_CONTEXT("Incorrect second filter")
    {
        auto group = MakerFilterGroup();
        auto it = group.Filters.end();
        --it;
        auto copy = *it;
        copy.Values = ValueSet {};
        group.Filters.erase(it);
        group.Filters.insert(copy);

        BOOST_CHECK_EQUAL("Filter empty", TTableUtils::ValidateFilterGroup(group));
    }
}

BOOST_FIXTURE_TEST_CASE(ValidateSortOrderTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("Correct sort order")
    {
        auto sortOrder = MakeSortOrder();
        auto expectedSortOrder = sortOrder;
        BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateSortOrder(sortOrder));
        expectedSortOrder.push_back(static_cast<TColumnType>(QuoterSettings::ColumnType::Id));
        BOOST_CHECK_EQUAL_COLLECTIONS(
            sortOrder.cbegin(), sortOrder.cend(),
            expectedSortOrder.cbegin(), expectedSortOrder.cend());
    }

    BOOST_TEST_CONTEXT("Correct sort order with id")
    {
        std::optional<TSortOrder> newSortOrder;
        auto sortOrder = MakeSortOrder();
        sortOrder.push_back(static_cast<TColumnType>(QuoterSettings::ColumnType::Id));
        auto expectedSortOrder = sortOrder;
        BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateSortOrder(sortOrder));
        BOOST_CHECK_EQUAL_COLLECTIONS(
            sortOrder.cbegin(), sortOrder.cend(),
            expectedSortOrder.cbegin(), expectedSortOrder.cend());
    }

    BOOST_TEST_CONTEXT("Incorrect column")
    {
        auto order = TSortOrder { static_cast<TColumnType>( Filters::FullTextSearch ) };
        auto expectedSortOrder = order;
        BOOST_CHECK_EQUAL("Wrong column number: 1000", TTableUtils::ValidateSortOrder(order));
        BOOST_CHECK_EQUAL_COLLECTIONS(
            order.cbegin(), order.cend(),
            expectedSortOrder.cbegin(), expectedSortOrder.cend());
    }

    BOOST_TEST_CONTEXT("Incorrect columns duplicated")
    {
        auto order = MakeSortOrder();
        order.push_back(order.front());
        auto expectedSortOrder = order;
        BOOST_CHECK_EQUAL("Sort column duplicated: Owner Firm", TTableUtils::ValidateSortOrder(order));
        BOOST_CHECK_EQUAL_COLLECTIONS(
            order.cbegin(), order.cend(),
            expectedSortOrder.cbegin(), expectedSortOrder.cend());
    }
}

BOOST_FIXTURE_TEST_CASE(ValidateSubscriptionTests, TableUtilsTests)
{
    BOOST_TEST_CONTEXT("Correct subscription with new")
    {
        auto subscription = MakeSubscription();
        auto expectedSubscription = subscription;
        BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateSubscription(subscription));
        expectedSubscription.SortOrder.push_back(static_cast<TColumnType>(QuoterSettings::ColumnType::Id));
        BOOST_CHECK_EQUAL(subscription, expectedSubscription);
    }

    BOOST_TEST_CONTEXT("Correct subscription")
    {
        auto subscription = MakeSubscription();
        subscription.SortOrder.push_back(static_cast<TColumnType>(QuoterSettings::ColumnType::Id));
        auto expectedSubscription = subscription;
        BOOST_CHECK_EQUAL(TTableUtils::Ok, TTableUtils::ValidateSubscription(subscription));
        BOOST_CHECK_EQUAL(subscription, expectedSubscription);
    }

    BOOST_TEST_CONTEXT("Incorrect filter")
    {
        auto subscription = MakeSubscription();
        subscription.FilterExpression.Relation = static_cast<FilterRelation>(-1);
        auto expectedSubscription = subscription;
        BOOST_CHECK_EQUAL("Wrong filter relation: -1", TTableUtils::ValidateSubscription(subscription));
        expectedSubscription.SortOrder.push_back(static_cast<TColumnType>(QuoterSettings::ColumnType::Id));
        BOOST_CHECK_EQUAL(subscription, expectedSubscription);
    }

    BOOST_TEST_CONTEXT("Incorrect sort order")
    {
        auto subscription = MakeSubscription();
        subscription.SortOrder = TSortOrder { -1 };
        auto expectedSubscription = subscription;
        BOOST_CHECK_EQUAL("Wrong column number: -1", TTableUtils::ValidateSubscription(subscription));
        BOOST_CHECK_EQUAL(subscription, expectedSubscription);
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
