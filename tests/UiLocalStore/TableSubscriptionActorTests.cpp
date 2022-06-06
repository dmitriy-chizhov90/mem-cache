#include "DummyTableData.hpp"

#include "UiLocalStore/TableSubscriptionActor.hpp"
#include "UiLocalStore/TableSorter.hpp"
#include "UiLocalStore/TableFilterman.hpp"
#include "UiLocalStore/TableIncrementApplicator.hpp"
#include "UiLocalStore/TableIncrementMaker.hpp"
#include "TradingSerialization/Table/Columns.hpp"


#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <boost/iterator/zip_iterator.hpp>
#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_TableSubscriptionActorTests)

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

struct TApplicatorSetup
{
    static constexpr int MaxCount = 500;
    using TData = DummyTableItem;
    using TTableSetup = DummyTableSetup;
};

struct TSubscriptionActorSetup
{
    using TData = DummyTableItem;
    using TMap = DummyTableItemMultiIndex;

    using TTableSorter = Basis::GMock;
    using TTableSorterInit = TableSorter<TSorterSetup>::TInit;
    using TTableIndexFilterman = Basis::GMock;
    using TTableIndexFiltermanInit = TableFilterman<TFiltermanSetup, Basis::GMock>::TInit;
    using TPackDataRanges = Basis::GMock;

    using TTableIncrementMaker = Basis::GMock;
    using TTableIncrementMakerInit = TableIncrementMaker<TMakerSetup>::TInit;
    using TTableIncrementApplicator = Basis::GMock;
    using TTableIncrementApplicatorInit = TableIncrementApplicator<TApplicatorSetup>::TInit;
    using TIndexedDataRanges = Basis::GMock;
    using TIndexedDataRangesInit = DummyTableItemIndexRangesInit;

    using TSubscriptionStateMachine = Basis::GMock;
};

struct TableSubscriptionActorTests : public BaseTestFixture
{
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TUiFilter = TradingSerialization::Table::Filter;
    using TUiFilters = TradingSerialization::Table::FilterGroup;

    using TUiSubscription = TradingSerialization::Table::SubscribeBase;

    using TDataPack = Basis::Pack<DummyTableItem>;

    using TSubscriptionActor = TableSubscriptionActor<TSubscriptionActorSetup>;

    using TMap = typename TSubscriptionActor::TMap;

    TUiFilters FilterExpression;
    TSortOrder SortOrder;
    TUiSubscription UiSubscription;

    TDataPack Deleted;
    TDataPack Added;
    DataIncrement<DummyTableItem> Increment;
    TDataPack TmpBuffer;

    Basis::Tracer& Tracer;

    TMap RawData;

    TDataVersion Version = 50;
    TUiSubscription::TId RequestId {
        Basis::UniqueIdGenerator<TUiSubscription::TId> {}.GetNext() };

    TSubscriptionActor Actor;

    TableSubscriptionActorTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart()))
        , RawData(Tracer)
        , Actor(
            Tracer,
            RequestId,
            InitUiSubscription(),
            RawData,
            Version)
    {
    }

    const TUiSubscription& InitUiSubscription()
    {
        SortOrder.push_back(static_cast<TColumnType>(DummyColumnType::Id));

        TUiFilter filter;
        filter.Column = static_cast<TColumnType>(DummyColumnType::Id);
        filter.Operator = FilterOperator::LessEq;
        filter.Values.Add(5);
        FilterExpression.Filters.insert(filter);
        FilterExpression.Relation = FilterRelation::And;

        UiSubscription.SortOrder = SortOrder;
        UiSubscription.FilterExpression = FilterExpression;

        return UiSubscription;
    }

    void PreprocessCheckState(TSubscriptionActor::TState aState)
    {
        EXPECT_CALL(Actor.State, GetProcessingState())
            .WillOnce(Return(ISubscriptionStateMachine::ProcessingState::Processing));
        EXPECT_CALL(Actor.State, GetState())
            .WillOnce(Return(aState));
    }

    void ExpectInitRange(bool aInitResult)
    {
        EXPECT_CALL(Actor.Ranges, Init<DummyTableItemIndexRangesInit>(_))
             .WillOnce(Invoke([=](const DummyTableItemIndexRangesInit& aInit)
        {
            BOOST_CHECK_EQUAL(&aInit.Map, &RawData);
            BOOST_CHECK_EQUAL(aInit.FilterExpression, FilterExpression);
            BOOST_CHECK_EQUAL(aInit.Version, Version);
            BOOST_CHECK(!aInit.IncrementAction);

             return aInitResult;
        }));
    }

    void SuccessInitTest()
    {
        PreprocessCheckState(TSubscriptionActor::TState::Initializing);
        ExpectInitRange(true);
        EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::Initialized)));
        BOOST_CHECK(Actor.Process());
    }
};

BOOST_FIXTURE_TEST_CASE(SuccessCreationTest, TableSubscriptionActorTests)
{
    BOOST_CHECK_EQUAL(Actor.GetVersion(), Version);
    BOOST_CHECK_EQUAL(Actor.GetRequestId(), RequestId);

    EXPECT_CALL(Actor.State, GetState())
        .WillOnce(Return(TSubscriptionActor::TState::Ok));
    BOOST_CHECK_EQUAL(Actor.GetState(), TSubscriptionActor::TState::Ok);

    EXPECT_CALL(Actor.State, GetState())
        .WillOnce(Return(TSubscriptionActor::TState::Ok));
    BOOST_CHECK(!Actor.GetResult().HasValue());
}

BOOST_FIXTURE_TEST_CASE(UpdateRawDataTest, TableSubscriptionActorTests)
{
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::UpdatesReceived)));
    Actor.IncreaseVersion();

    BOOST_CHECK_EQUAL(Actor.GetVersion(), Version + 1);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInit, TableSubscriptionActorTests)
{
    SuccessInitTest();
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedInit, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Initializing);
    ExpectInitRange(false);

    EXPECT_CALL(Actor.State, ReportError(Eq("Cannot init ranges")));
    BOOST_CHECK(!Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessFilter, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Filtration);
    EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Processing));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInitFilter, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Filtration);
    EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(false));

    EXPECT_CALL(Actor.Filterman, Init<TSubscriptionActorSetup::TTableIndexFiltermanInit>(_))
         .WillOnce(Invoke([=](const TSubscriptionActorSetup::TTableIndexFiltermanInit& aInit)
    {
         BOOST_CHECK_EQUAL(&aInit.IndexRanges, &Actor.Ranges.Get());
         BOOST_CHECK_EQUAL(aInit.Filters, FilterExpression);
    }));

    EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Processing));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteFilter, TableSubscriptionActorTests)
{
    SuccessInitTest();

    PreprocessCheckState(TSubscriptionActor::TState::Filtration);
    EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Completed));
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::FiltrationCompleted)));
    EXPECT_CALL(Actor.Filterman, Reset());
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedFilter, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Filtration);
    EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Error));
    EXPECT_CALL(Actor.State, ReportError(Eq("Filtration failed")));
    BOOST_CHECK(!Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessSorting, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Sorting);
    EXPECT_CALL(Actor.Sorter, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.Sorter, Process()).WillOnce(Return(TableSorterState::PartSort));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInitSorter, TableSubscriptionActorTests)
{
    SuccessInitTest();

    PreprocessCheckState(TSubscriptionActor::TState::Sorting);
    EXPECT_CALL(Actor.Sorter, IsInitialized()).WillOnce(Return(false));

    EXPECT_CALL(Actor.Sorter, Init<TSubscriptionActorSetup::TTableSorterInit>(_))
         .WillOnce(Invoke([=](const TSubscriptionActorSetup::TTableSorterInit& aInit)
    {
         BOOST_CHECK_EQUAL(aInit.SortOrder, SortOrder);
    }));

    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteSorting, TableSubscriptionActorTests)
{
    SuccessInitTest();

    PreprocessCheckState(TSubscriptionActor::TState::Sorting);
    EXPECT_CALL(Actor.Sorter, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.Sorter, Process()).WillOnce(Return(TableSorterState::Completed));
    EXPECT_CALL(Actor.Sorter, Reset());
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::SortingCompleted)));
    EXPECT_CALL(Actor.Ranges, Reset());
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedSorting, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Sorting);
    EXPECT_CALL(Actor.Sorter, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.Sorter, Process()).WillOnce(Return(TableSorterState::Error));
    EXPECT_CALL(Actor.State, ReportError(Eq("ProcessSortingState: cannot process sorting")));
    BOOST_CHECK(!Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessUpdating, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Updating);
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::RawDataUpdated)));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessIncrementMaking, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::IncrementMaking);
    EXPECT_CALL(Actor.IncrementMaker, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.IncrementMaker, Process()).WillOnce(Return(TableIncrementMakerState::DeletedSorting));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInitIncrementMaking, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::IncrementMaking);
    EXPECT_CALL(Actor.IncrementMaker, IsInitialized()).WillOnce(Return(false));

    EXPECT_CALL(Actor.IncrementMaker, Init<TSubscriptionActorSetup::TTableIncrementMakerInit>(_))
         .WillOnce(Invoke([=](const TSubscriptionActorSetup::TTableIncrementMakerInit& aInit)
    {
        BOOST_CHECK_EQUAL(aInit.Filters, FilterExpression);
        BOOST_CHECK_EQUAL(aInit.SortOrder, SortOrder);
        BOOST_CHECK_EQUAL(&aInit.Map, &RawData);
        BOOST_CHECK_EQUAL(aInit.Version, Version);
    }));

    EXPECT_CALL(Actor.IncrementMaker, Process()).WillOnce(Return(TableIncrementMakerState::DeletedSorting));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteIncrementMaking, TableSubscriptionActorTests)
{
    SuccessInitTest();

    PreprocessCheckState(TSubscriptionActor::TState::IncrementMaking);
    EXPECT_CALL(Actor.IncrementMaker, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.IncrementMaker, Process()).WillOnce(Return(TableIncrementMakerState::Completed));
    EXPECT_CALL(Actor.IncrementMaker, Reset());
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::IncrementMade)));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedIncrementMaking, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::IncrementMaking);
    EXPECT_CALL(Actor.IncrementMaker, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.IncrementMaker, Process()).WillOnce(Return(TableIncrementMakerState::Error));
    EXPECT_CALL(Actor.State, ReportError(Eq("Increment making failed")));
    BOOST_CHECK(!Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessIncrementApplying, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::IncrementApplying);
    EXPECT_CALL(Actor.IncrementApplicator, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.IncrementApplicator, Process())
        .WillOnce(Return(TableIncrementApplicatorState::Processing));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInitIncrementApplying, TableSubscriptionActorTests)
{
    SuccessInitTest();

    PreprocessCheckState(TSubscriptionActor::TState::IncrementApplying);
    EXPECT_CALL(Actor.IncrementApplicator, IsInitialized()).WillOnce(Return(false));

    EXPECT_CALL(Actor.IncrementApplicator, Init<TSubscriptionActorSetup::TTableIncrementApplicatorInit>(_))
         .WillOnce(Invoke([=](const TSubscriptionActorSetup::TTableIncrementApplicatorInit& aInit)
    {
         BOOST_CHECK_EQUAL(aInit.SortOrder, SortOrder);
    }));

    EXPECT_CALL(Actor.IncrementApplicator, Process()).WillOnce(Return(TableIncrementApplicatorState::Processing));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteIncrementApplying, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::IncrementApplying);
    EXPECT_CALL(Actor.IncrementApplicator, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.IncrementApplicator, Process()).WillOnce(Return(TableIncrementApplicatorState::Completed));
    EXPECT_CALL(Actor.IncrementApplicator, Reset());
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::IncrementApplied)));
    BOOST_CHECK(Actor.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedIncrementApplying, TableSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::IncrementApplying);
    EXPECT_CALL(Actor.IncrementApplicator, IsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(Actor.IncrementApplicator, Process()).WillOnce(Return(TableIncrementApplicatorState::Error));
    EXPECT_CALL(Actor.State, ReportError(Eq("Increment applying failed")));
    BOOST_CHECK(!Actor.Process());
}


BOOST_AUTO_TEST_SUITE_END()
}
