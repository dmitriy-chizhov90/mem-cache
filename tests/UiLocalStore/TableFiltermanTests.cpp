#include "DummyTableData.hpp"

#include "UiLocalStore/TableFilterman.hpp"
#include "TradingSerialization/Table/Columns.hpp"


#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_TableFiltermanTests)

using namespace TradingSerialization::Table;

template <int _MaxCount>
struct TFiltermanSetup
{
    static constexpr int MaxCount = _MaxCount;
    using TData = DummyTableItem;
    using TTableSetup = DummyTableSetup;

    using TIndexedDataRanges = Basis::GMock;
};

struct TableFiltermanTests : public BaseTestFixture
{
    using TUiFilters = TradingSerialization::Table::FilterGroup;
    using TUiFilter = TradingSerialization::Table::Filter;

    using TDataPack = Basis::Pack<DummyTableItem>;

    TUiFilters FilterExpression;

    TDataPack Data;
    TDataPack ExpectedData;
    TDataPack Source;

    Basis::Tracer& Tracer;

    Basis::GMock IndexedRanges;

    TableFiltermanTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart()))
    {
        TUiFilter filter;
        filter.Column = static_cast<TColumnType>(DummyColumnType::Id);
        filter.Operator = FilterOperator::LessEq;
        filter.Values.Add(5);
        FilterExpression.Filters.insert(filter);
        FilterExpression.Relation = FilterRelation::And;
    }

    template <int _MaxCount>
    auto MakeFilterman()
    {
        using TFilterman = TableFilterman<TFiltermanSetup<_MaxCount>, Basis::GMock>;
        TFilterman filterman(Tracer);
        BOOST_CHECK(!filterman.IsInitialized());
        filterman.Init(typename TFilterman::TInit { FilterExpression, Data, IndexedRanges });
        BOOST_CHECK(filterman.IsInitialized());
        return filterman;
    }

    void FillSource(const Basis::Vector<int>& aValues)
    {
        Source.clear();
        std::transform(aValues.cbegin(), aValues.cend(), std::back_inserter(Source), [](const auto aValue)
        {
            return Basis::MakeSPtr<DummyTableItem>(aValue);
        });
    }

    void CheckResult()
    {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            ExpectedData.cbegin(), ExpectedData.cend(),
            Data.cbegin(), Data.cend());
    }

    template <typename TFilterman>
    void ProcessSourceData(TFilterman& aFilterman, bool aIsFinal = true)
    {
        InSequence s;
        for (const auto& item : Source)
        {
            EXPECT_CALL(aFilterman.Ranges, GetNext()).WillOnce(Return(item));
        }
        auto state = TableFiltermanState::Processing;
        if (aIsFinal)
        {
            EXPECT_CALL(aFilterman.Ranges, GetNext()).WillOnce(Return(Basis::NullSPtr<DummyTableItem>()));
            state = TableFiltermanState::Completed;
        }
        BOOST_CHECK_EQUAL(state, aFilterman.Process());
        CheckResult();
    }
};

BOOST_FIXTURE_TEST_CASE(ProcessPartFilterTest, TableFiltermanTests)
{
    FillSource(Basis::Vector<int> { 1, 21, 4, 17 });
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(1));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(4));

    auto filterman = MakeFilterman<10>();
    ProcessSourceData(filterman);
}

BOOST_FIXTURE_TEST_CASE(ProcessEmptyFilterTest, TableFiltermanTests)
{
    FillSource(Basis::Vector<int> { 1, 21, 4, 17 });
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(1));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(21));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(4));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(17));
    FilterExpression.Filters.clear();

    auto filterman = MakeFilterman<10>();
    ProcessSourceData(filterman);
}

BOOST_FIXTURE_TEST_CASE(ProcessAllFilteredTest, TableFiltermanTests)
{
    FillSource(Basis::Vector<int> { 10, 21, 40, 17 });

    auto filterman = MakeFilterman<10>();
    ProcessSourceData(filterman);
}

BOOST_FIXTURE_TEST_CASE(ProcessEmptySourceTest, TableFiltermanTests)
{
    FillSource(Basis::Vector<int> {});

    auto filterman = MakeFilterman<10>();
    ProcessSourceData(filterman);
}

BOOST_FIXTURE_TEST_CASE(ProcessMultiPartsTest, TableFiltermanTests)
{
    auto filterman = MakeFilterman<4>();

    FillSource(Basis::Vector<int> { 1, 21, 4, 17 });
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(1));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(4));
    ProcessSourceData(filterman, false);

    FillSource(Basis::Vector<int> { 2, 3, 18, 17 });
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(2));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(3));
    ProcessSourceData(filterman, false);

    FillSource(Basis::Vector<int> {});
    ProcessSourceData(filterman);
}

BOOST_FIXTURE_TEST_CASE(ProcessAlmostCompletedLastPartTest, TableFiltermanTests)
{
    auto filterman = MakeFilterman<4>();

    FillSource(Basis::Vector<int> { 1, 21, 4, 17 });
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(1));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(4));
    ProcessSourceData(filterman, false);

    FillSource(Basis::Vector<int> { 2, 3, 18 });
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(2));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(3));
    ProcessSourceData(filterman);
}

BOOST_FIXTURE_TEST_CASE(FailureTest, TableFiltermanTests)
{
    FilterExpression.Relation = static_cast<FilterRelation>(-1);
    auto filterman = MakeFilterman<4>();

    EXPECT_CALL(filterman.Ranges, GetNext()).WillOnce(Return(Basis::MakeSPtr<DummyTableItem>(1)));
    BOOST_CHECK_EQUAL(TableFiltermanState::Error, filterman.Process());
    CheckResult();
}

BOOST_FIXTURE_TEST_CASE(ResetTest, TableFiltermanTests)
{
    auto filterman = MakeFilterman<10>();

    FillSource(Basis::Vector<int> { 1, 21, 4, 17 });
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(1));
    ExpectedData.push_back(Basis::MakeSPtr<DummyTableItem>(4));

    filterman.Reset();
    filterman.Init(typename decltype(filterman)::TInit { FilterExpression, Data, IndexedRanges });

    ProcessSourceData(filterman);
}

BOOST_AUTO_TEST_SUITE_END()
}
