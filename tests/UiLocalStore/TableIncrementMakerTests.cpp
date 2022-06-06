#include "DummyTableData.hpp"

#include "UiLocalStore/TableIncrementMaker.hpp"
#include "UiLocalStore/TableSorter.hpp"
#include "UiLocalStore/TableFilterman.hpp"
#include "TradingSerialization/Table/Columns.hpp"


#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <boost/iterator/zip_iterator.hpp>
#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_TableIncrementMakerTests)

using namespace TradingSerialization::Table;

struct TFiltermanSetup
{
    static constexpr int MaxCount = 500;
    using TData = DummyTableItem;
    using TTableSetup = DummyTableSetup;

    using TIndexedDataRanges = Basis::GMock;
};

struct TSorterSetup
{
    static constexpr int MaxCount = 500;
    using TData = DummyTableItem;
    using TTableSetup = DummyTableSetup;
};

struct TMakerSetup
{
    using TData = DummyTableItem;
    using TTableSorter = Basis::GMock;
    using TTableSorterInit = TableSorter<TSorterSetup>::TInit;
    using TTableIndexFilterman = Basis::GMock;
    using TTableIndexFiltermanInit = TableFilterman<TFiltermanSetup, Basis::GMock>::TInit;
    using TIndexedDataRanges = Basis::GMock;
    using TIndexedDataRangesInit = DummyTableItemIndexRangesInit;
    using TMap = DummyTableItemMultiIndex;
};

struct TableIncrementMakerTests : public BaseTestFixture
{
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TUiFilter = TradingSerialization::Table::Filter;
    using TUiFilters = TradingSerialization::Table::FilterGroup;
    using TDataPack = Basis::Pack<DummyTableItem>;

    using TIncrementMaker = TableIncrementMaker<TMakerSetup>;

    TUiFilters FilterExpression;
    TSortOrder SortOrder;

    TDataPack Deleted;
    TDataPack Added;
    TDataPack TmpBuffer;

    Basis::Tracer& Tracer;
    TMakerSetup::TMap Increment;

    TIncrementMaker IncrementMaker;

    TDataVersion Version = 1;

    TableIncrementMakerTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart()))
        , Increment(Tracer)
        , IncrementMaker(Tracer)
    {
        SortOrder.push_back(static_cast<TColumnType>(DummyColumnType::Id));

        TUiFilter filter;
        filter.Column = static_cast<TColumnType>(DummyColumnType::Id);
        filter.Operator = FilterOperator::LessEq;
        filter.Values.Add(5);
        FilterExpression.Filters.insert(filter);
        FilterExpression.Relation = FilterRelation::And;

        std::srand(unsigned(std::time(nullptr)));

        FillDeletedIncrement();
        FillAddedIncrement();
        FillTmpBuffer();
        FillExpectedDeletedIncrement();
        FillExpectedAddedIncrement();
    }

    void FillPack(TDataPack& outPack, const Basis::Vector<int>& aData, const std::string& aValue)
    {
        outPack.clear();
        std::transform(aData.cbegin(), aData.cend(), std::back_inserter(outPack), [&](auto aId)
        {
            return Basis::MakeSPtr<DummyTableItem>(aId, aValue);
        });
    }

    void FillMap(
        DummyTableItemMultiIndex& outMap,
        const Basis::Vector<int>& aData,
        const std::string& aValue,
        TDataVersion aVersion)
    {
        std::for_each(aData.cbegin(), aData.cend(), [&](auto aId)
        {
            outMap.Emplace(Basis::MakeSPtr<DummyTableItem>(aId, aValue), aVersion);
        });
    }

    void FillMapErased(
        DummyTableItemMultiIndex& outMap,
        const Basis::Vector<int>& aData,
        TDataVersion aVersion)
    {
        std::for_each(aData.cbegin(), aData.cend(), [&](auto aId)
        {
            outMap.Erase(aId, aVersion);
        });
    }

    void FillExpectedDeletedIncrement()
    {
        FillPack(Deleted, Basis::Vector<int> { 1, 2, 3 }, "ExpDeleted");
    }
    void FillExpectedAddedIncrement()
    {
        FillPack(Added, Basis::Vector<int> { 4, 5, 6 }, "ExpAdded");
    }
    void FillDeletedIncrement()
    {
        FillMap(Increment, Basis::Vector<int> { 10, 11, 12 }, "Deleted", 0);
        FillMapErased(Increment, Basis::Vector<int> { 10, 11, 12 }, 1);
    }
    void FillAddedIncrement()
    {
        FillMap(Increment, Basis::Vector<int> { 10, 11, 12 }, "Added", 1);
    }
    void FillTmpBuffer()
    {
        FillPack(TmpBuffer, Basis::Vector<int> { 7, 8, 9 }, "Tmp");
    }

    void ExpectDeletedRangeInit()
    {
        EXPECT_CALL(IncrementMaker.DeletedRange, Init<DummyTableItemIndexRangesInit>(_))
            .WillOnce(Invoke([=](const DummyTableItemIndexRangesInit& aInit)
        {
            BOOST_CHECK_EQUAL(&aInit.Map, &Increment);
            BOOST_CHECK_EQUAL(aInit.FilterExpression, FilterExpression);
            BOOST_CHECK_EQUAL(aInit.Version, 1);
            BOOST_REQUIRE(aInit.IncrementAction);
            BOOST_CHECK_EQUAL(*aInit.IncrementAction, Model::ActionType::Delete);

            return true;
        }));
    }
    void ExpectAddedRangeInit()
    {
        EXPECT_CALL(IncrementMaker.AddedRange, Init<DummyTableItemIndexRangesInit>(_))
            .WillOnce(Invoke([=](const DummyTableItemIndexRangesInit& aInit)
        {
            BOOST_CHECK_EQUAL(&aInit.Map, &Increment);
            BOOST_CHECK_EQUAL(aInit.FilterExpression, FilterExpression);
            BOOST_CHECK_EQUAL(aInit.Version, 1);
            BOOST_REQUIRE(aInit.IncrementAction);
            BOOST_CHECK_EQUAL(*aInit.IncrementAction, Model::ActionType::New);

            return true;
        }));
    }

    void Init(
        TableIncrementMakerState aState = TableIncrementMakerState::DeletedFiltration)
    {
        BOOST_CHECK(!IncrementMaker.IsInitialized());

        ExpectDeletedRangeInit();
        ExpectAddedRangeInit();

        IncrementMaker.Init(typename TIncrementMaker::TInit
        {
            FilterExpression,
            SortOrder,
            Deleted,
            Added,
            Increment,
            TmpBuffer,
            Version
        });
        BOOST_CHECK(IncrementMaker.IsInitialized());
        BOOST_CHECK_EQUAL(aState, IncrementMaker.GetState());
    }

    void ExpectInitFilterman(
        TMakerSetup::TIndexedDataRanges* aExpectedRange,
        TDataPack* aExpectedPack)
    {
        EXPECT_CALL(IncrementMaker.Filterman, Init<TMakerSetup::TTableIndexFiltermanInit>(_))
            .WillOnce(Invoke([=](const TMakerSetup::TTableIndexFiltermanInit& aInit)
        {
            BOOST_CHECK_EQUAL(&aInit.Filters, &FilterExpression);
            BOOST_CHECK_EQUAL(&aInit.IndexRanges, aExpectedRange);
            BOOST_CHECK_EQUAL(&aInit.Result, aExpectedPack);
        }));
    }

    void ExpectInitSorter(TDataPack* aExpectedPack)
    {
        EXPECT_CALL(IncrementMaker.Sorter, Init<TMakerSetup::TTableSorterInit>(_))
            .WillOnce(Invoke([=](const TMakerSetup::TTableSorterInit& aInit)
        {
            BOOST_CHECK_EQUAL(&aInit.SortOrder, &SortOrder);
            BOOST_CHECK_EQUAL(&aInit.TmpBuffer, &TmpBuffer);
            BOOST_CHECK_EQUAL(&aInit.Result, aExpectedPack);
        }));
    }

    void ExpectInitDeletedFilterman()
    {
        ExpectInitFilterman(&IncrementMaker.DeletedRange.Get(), &Deleted);
    }

    void ExpectInitAddedFilterman()
    {
        ExpectInitFilterman(&IncrementMaker.AddedRange.Get(), &Added);
    }

    void ExpectInitDeletedSorter()
    {
        ExpectInitSorter(&Deleted);
    }

    void ExpectInitAddedSorter()
    {
        ExpectInitSorter(&Added);
    }

    void ProcessFiltration(TableFiltermanState aFilterState, TableIncrementMakerState aNextState, bool aReset)
    {
        EXPECT_CALL(IncrementMaker.Filterman, Process())
            .WillOnce(Return(aFilterState));

        if (aReset)
        {
            EXPECT_CALL(IncrementMaker.Filterman, Reset());
        }

        BOOST_CHECK_EQUAL(aNextState, IncrementMaker.Process());
    }

    void ProcessSorting(TableSorterState aSorterState, TableIncrementMakerState aNextState, bool aReset)
    {
        EXPECT_CALL(IncrementMaker.Sorter, Process())
            .WillOnce(Return(aSorterState));

        if (aReset)
        {
            EXPECT_CALL(IncrementMaker.Sorter, Reset());
        }

        BOOST_CHECK_EQUAL(aNextState, IncrementMaker.Process());
    }

    void CompleteFiltrationTest(TableIncrementMakerState aNextState)
    {
        EXPECT_CALL(IncrementMaker.Filterman, IsInitialized()).WillOnce(Return(true));
        ProcessFiltration(TableFiltermanState::Completed, aNextState, true);
    }

    void CompleteSortingTest(TableIncrementMakerState aNextState)
    {
        EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
        ProcessSorting(TableSorterState::Completed, aNextState, true);
    }

    void CompleteDeletedFiltrationTest()
    {
        Init();
        CompleteFiltrationTest(TableIncrementMakerState::AddedFiltration);
    }

    void CompleteAddedFiltrationTest()
    {
        CompleteDeletedFiltrationTest();
        CompleteFiltrationTest(TableIncrementMakerState::DeletedSorting);
    }

    void CompleteDeletedSortingTest()
    {
        CompleteAddedFiltrationTest();
        CompleteSortingTest(TableIncrementMakerState::AddedSorting);
    }
};

BOOST_FIXTURE_TEST_CASE(SuccessCreationTest, TableIncrementMakerTests)
{
    Init();
}

BOOST_FIXTURE_TEST_CASE(CompletedDeletedFiltrationTest, TableIncrementMakerTests)
{
   CompleteDeletedFiltrationTest();
}

BOOST_FIXTURE_TEST_CASE(InitDeletedFiltermanTest, TableIncrementMakerTests)
{
    Init();

    EXPECT_CALL(IncrementMaker.Filterman, IsInitialized()).WillOnce(Return(false));
    ExpectInitDeletedFilterman();
    ProcessFiltration(TableFiltermanState::Completed, TableIncrementMakerState::AddedFiltration, true);
}

BOOST_FIXTURE_TEST_CASE(ContinueDeletedFiltrationTest, TableIncrementMakerTests)
{
    Init();

    EXPECT_CALL(IncrementMaker.Filterman, IsInitialized()).WillOnce(Return(true));
    ProcessFiltration(TableFiltermanState::Processing, TableIncrementMakerState::DeletedFiltration, false);
}

BOOST_FIXTURE_TEST_CASE(ErrorDeletedFiltrationTest, TableIncrementMakerTests)
{
    Init();

    EXPECT_CALL(IncrementMaker.Filterman, IsInitialized()).WillOnce(Return(true));
    ProcessFiltration(TableFiltermanState::Error, TableIncrementMakerState::Error, false);
}

BOOST_FIXTURE_TEST_CASE(CompletedAddedFiltrationTest, TableIncrementMakerTests)
{
   CompleteAddedFiltrationTest();
}

BOOST_FIXTURE_TEST_CASE(InitAddedFiltermanTest, TableIncrementMakerTests)
{
    CompleteDeletedFiltrationTest();

    EXPECT_CALL(IncrementMaker.Filterman, IsInitialized()).WillOnce(Return(false));
    ExpectInitAddedFilterman();
    ProcessFiltration(TableFiltermanState::Completed, TableIncrementMakerState::DeletedSorting, true);
}

BOOST_FIXTURE_TEST_CASE(ContinueAddedFiltrationTest, TableIncrementMakerTests)
{
    CompleteDeletedFiltrationTest();

    EXPECT_CALL(IncrementMaker.Filterman, IsInitialized()).WillOnce(Return(true));
    ProcessFiltration(TableFiltermanState::Processing, TableIncrementMakerState::AddedFiltration, false);
}

BOOST_FIXTURE_TEST_CASE(ErrorAddedFiltrationTest, TableIncrementMakerTests)
{
    CompleteDeletedFiltrationTest();

    EXPECT_CALL(IncrementMaker.Filterman, IsInitialized()).WillOnce(Return(true));
    ProcessFiltration(TableFiltermanState::Error, TableIncrementMakerState::Error, false);
}

BOOST_FIXTURE_TEST_CASE(CompletedDeletedSortingTest, TableIncrementMakerTests)
{
   CompleteDeletedSortingTest();
}

BOOST_FIXTURE_TEST_CASE(InitDeletedSortingTest, TableIncrementMakerTests)
{
    CompleteAddedFiltrationTest();

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(false));
    ExpectInitDeletedSorter();
    BOOST_CHECK_EQUAL(TableIncrementMakerState::DeletedSorting, IncrementMaker.Process());
}

BOOST_FIXTURE_TEST_CASE(ContinueDeletedSortingTest, TableIncrementMakerTests)
{
    CompleteAddedFiltrationTest();

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
    ProcessSorting(TableSorterState::PartSort, TableIncrementMakerState::DeletedSorting, false);

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
    ProcessSorting(TableSorterState::MergeSort, TableIncrementMakerState::DeletedSorting, false);
}

BOOST_FIXTURE_TEST_CASE(ErrorDeletedSortingTest, TableIncrementMakerTests)
{
    CompleteAddedFiltrationTest();

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
    ProcessSorting(TableSorterState::Error, TableIncrementMakerState::Error, false);
}

BOOST_FIXTURE_TEST_CASE(CompletedAddedSortingTest, TableIncrementMakerTests)
{
   CompleteDeletedSortingTest();
   CompleteSortingTest(TableIncrementMakerState::Completed);
}

BOOST_FIXTURE_TEST_CASE(InitAddedSortingTest, TableIncrementMakerTests)
{
    CompleteDeletedSortingTest();

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(false));
    ExpectInitAddedSorter();
    BOOST_CHECK_EQUAL(TableIncrementMakerState::AddedSorting, IncrementMaker.Process());
}

BOOST_FIXTURE_TEST_CASE(ContinueAddedSortingTest, TableIncrementMakerTests)
{
    CompleteDeletedSortingTest();

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
    ProcessSorting(TableSorterState::PartSort, TableIncrementMakerState::AddedSorting, false);

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
    ProcessSorting(TableSorterState::MergeSort, TableIncrementMakerState::AddedSorting, false);
}

BOOST_FIXTURE_TEST_CASE(ErrorAddedSortingTest, TableIncrementMakerTests)
{
    CompleteDeletedSortingTest();

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
    ProcessSorting(TableSorterState::Error, TableIncrementMakerState::Error, false);
}

BOOST_FIXTURE_TEST_CASE(ProcessAfterErrorTest, TableIncrementMakerTests)
{
    CompleteDeletedSortingTest();

    EXPECT_CALL(IncrementMaker.Sorter, IsInitialized()).WillOnce(Return(true));
    ProcessSorting(TableSorterState::Error, TableIncrementMakerState::Error, false);

    BOOST_CHECK_EQUAL(TableIncrementMakerState::Error, IncrementMaker.Process());
}


BOOST_FIXTURE_TEST_CASE(ProcessAfterCompletedTest, TableIncrementMakerTests)
{
    CompleteDeletedSortingTest();
    CompleteSortingTest(TableIncrementMakerState::Completed);

    BOOST_CHECK_EQUAL(TableIncrementMakerState::Error, IncrementMaker.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessAfterResetTest, TableIncrementMakerTests)
{
    CompleteDeletedSortingTest();
    CompleteSortingTest(TableIncrementMakerState::Completed);

    EXPECT_CALL(IncrementMaker.DeletedRange, Reset());
    EXPECT_CALL(IncrementMaker.AddedRange, Reset());
    IncrementMaker.Reset();

    CompleteDeletedFiltrationTest();
}

BOOST_FIXTURE_TEST_CASE(ProcessBeforeInitTest, TableIncrementMakerTests)
{
    BOOST_CHECK_EQUAL(TableIncrementMakerState::Error, IncrementMaker.Process());
}


BOOST_AUTO_TEST_SUITE_END()
}
