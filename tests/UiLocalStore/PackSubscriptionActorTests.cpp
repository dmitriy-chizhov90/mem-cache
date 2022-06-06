#include "DummyTableData.hpp"

#include "UiLocalStore/PackSubscriptionActor.hpp"
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

BOOST_AUTO_TEST_SUITE(UiServer_PackSubscriptionActorTests)

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

    constexpr static size_t MaxCount = 5;
};

struct PackSubscriptionActorTests : public BaseTestFixture
{
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TUiFilter = TradingSerialization::Table::Filter;
    using TUiFilters = TradingSerialization::Table::FilterGroup;

    using TUiSubscription = TradingSerialization::Table::SubscribeBase;
    using TUiRequestId = TUiSubscription::TId;

    using TDataPack = Basis::Pack<DummyTableItem>;

    using TSubscriptionActor = PackSubscriptionActor<TSubscriptionActorSetup>;

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
    TUiRequestId RequestId = Basis::UniqueIdGenerator<TUiRequestId> {}.GetNext();

    TSubscriptionActor Actor;

    PackSubscriptionActorTests()
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

    void ExpectInitRange(bool aInitResult, std::optional<Model::ActionType> aActionType = std::nullopt)
    {
        EXPECT_CALL(Actor.Ranges, Init<DummyTableItemIndexRangesInit>(_))
             .WillOnce(Invoke([=](const DummyTableItemIndexRangesInit& aInit)
        {
            BOOST_CHECK_EQUAL(&aInit.Map, &RawData);
            BOOST_CHECK_EQUAL(aInit.FilterExpression, FilterExpression);
            BOOST_CHECK_EQUAL(aInit.Version, Version);
            BOOST_REQUIRE_EQUAL(aInit.IncrementAction.has_value(), aActionType.has_value());
            if (aInit.IncrementAction)
            {
                BOOST_CHECK_EQUAL(*aInit.IncrementAction, *aActionType);
            }

             return aInitResult;
        }));
    }

    void SuccessInitTest()
    {
        PreprocessCheckState(TSubscriptionActor::TState::Initializing);
        EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::Initialized)));
        BOOST_CHECK(Actor.Process());
    }

    void SuccessFiltrationTests(TSubscriptionActor::TState aState)
    {
        PreprocessCheckState(aState);
        EXPECT_CALL(Actor.Ranges, IsInitialized()).WillOnce(Return(true));
        EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(true));
        EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Processing));
        BOOST_CHECK(Actor.Process());
    }

    void SuccessInitRangeTests(
        TSubscriptionActor::TState aState,
        std::optional<Model::ActionType> aActionType = std::nullopt)
    {
        PreprocessCheckState(aState);
        EXPECT_CALL(Actor.Ranges, IsInitialized()).WillOnce(Return(false));
        ExpectInitRange(true, aActionType);
        EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(true));
        EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Processing));
        BOOST_CHECK(Actor.Process());
    }

    void FailedInitRangeTests(
        TSubscriptionActor::TState aState,
        std::optional<Model::ActionType> aActionType = std::nullopt)
    {
        PreprocessCheckState(aState);

        EXPECT_CALL(Actor.Ranges, IsInitialized()).WillOnce(Return(false));
        ExpectInitRange(false, aActionType);
        EXPECT_CALL(Actor.State, ReportError(Eq("Cannot init ranges")));

        BOOST_CHECK(!Actor.Process());
    }

    void SuccessInitFilterTest(TSubscriptionActor::TState aState)
    {
        PreprocessCheckState(aState);
        EXPECT_CALL(Actor.Ranges, IsInitialized()).WillOnce(Return(true));
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

    void CompleteFilterTest(
        TSubscriptionActor::TState aState,
        TSubscriptionActor::TEvent aEvent = TSubscriptionActor::TEvent::PendingResultCompleted)
    {
        SuccessInitTest();

        PreprocessCheckState(aState);
        EXPECT_CALL(Actor.Ranges, IsInitialized()).WillOnce(Return(true));
        EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(true));
        EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Completed));
        EXPECT_CALL(Actor.State, ChangeState(Eq(aEvent)));
        EXPECT_CALL(Actor.Filterman, Reset());
        EXPECT_CALL(Actor.Ranges, Reset());
        BOOST_CHECK(Actor.Process());
    }

    void CompleteFilterFinalTest(TSubscriptionActor::TState aState, TSubscriptionActor::TEvent aEvent)
    {
        CompleteFilterTest(aState, aEvent);

        EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::GetNextReceived)));
        BOOST_CHECK(Actor.ProcessGetNext());
    }

    void FailedFilterTest(TSubscriptionActor::TState aState)
    {
        PreprocessCheckState(aState);
        EXPECT_CALL(Actor.Ranges, IsInitialized()).WillOnce(Return(true));
        EXPECT_CALL(Actor.Filterman, IsInitialized()).WillOnce(Return(true));
        EXPECT_CALL(Actor.Filterman, Process()).WillOnce(Return(TableFiltermanState::Error));
        EXPECT_CALL(Actor.State, ReportError(Eq("Filtration failed")));
        BOOST_CHECK(!Actor.Process());
    }
};

BOOST_FIXTURE_TEST_CASE(SuccessCreationTest, PackSubscriptionActorTests)
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

BOOST_FIXTURE_TEST_CASE(UpdateRawDataTest, PackSubscriptionActorTests)
{
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::UpdatesReceived)));
    Actor.IncreaseVersion();

    BOOST_CHECK_EQUAL(Actor.GetVersion(), Version + 1);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInit, PackSubscriptionActorTests)
{
    SuccessInitTest();
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessFilter, PackSubscriptionActorTests)
{
    SuccessFiltrationTests(TSubscriptionActor::TState::Filtration);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessDeletedFilter, PackSubscriptionActorTests)
{
    SuccessFiltrationTests(TSubscriptionActor::TState::DeletedFiltration);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessAddedFilter, PackSubscriptionActorTests)
{
    SuccessFiltrationTests(TSubscriptionActor::TState::AddedFiltration);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessFilterInitRange, PackSubscriptionActorTests)
{
    SuccessInitRangeTests(TSubscriptionActor::TState::Filtration);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessDeletedFilterInitRange, PackSubscriptionActorTests)
{
    SuccessInitRangeTests(TSubscriptionActor::TState::DeletedFiltration, Model::ActionType::Delete);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessAddedFilterInitRange, PackSubscriptionActorTests)
{
    SuccessInitRangeTests(TSubscriptionActor::TState::AddedFiltration, Model::ActionType::New);
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedFilterInitRange, PackSubscriptionActorTests)
{
    FailedInitRangeTests(TSubscriptionActor::TState::Filtration);
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedDeletedFilterInitRange, PackSubscriptionActorTests)
{
    FailedInitRangeTests(TSubscriptionActor::TState::DeletedFiltration, Model::ActionType::Delete);
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedAddedFilterInitRange, PackSubscriptionActorTests)
{
    FailedInitRangeTests(TSubscriptionActor::TState::AddedFiltration, Model::ActionType::New);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInitFilter, PackSubscriptionActorTests)
{
    SuccessInitFilterTest(TSubscriptionActor::TState::Filtration);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInitDeletedFilter, PackSubscriptionActorTests)
{
    SuccessInitFilterTest(TSubscriptionActor::TState::DeletedFiltration);
}

BOOST_FIXTURE_TEST_CASE(ProcessSuccessInitAddedFilter, PackSubscriptionActorTests)
{
    SuccessInitFilterTest(TSubscriptionActor::TState::AddedFiltration);
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteFilter, PackSubscriptionActorTests)
{
    CompleteFilterTest(TSubscriptionActor::TState::Filtration, TSubscriptionActor::TEvent::FiltrationCompleted);
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteFilterFinal, PackSubscriptionActorTests)
{
    CompleteFilterFinalTest(TSubscriptionActor::TState::Filtration, TSubscriptionActor::TEvent::FiltrationCompleted);
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteDeletedFilter, PackSubscriptionActorTests)
{
    CompleteFilterTest(
        TSubscriptionActor::TState::DeletedFiltration,
        TSubscriptionActor::TEvent::DeletedFiltrationCompleted);
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteDeletedFinalFilter, PackSubscriptionActorTests)
{
    CompleteFilterFinalTest(
        TSubscriptionActor::TState::DeletedFiltration,
        TSubscriptionActor::TEvent::DeletedFiltrationCompleted);
}

BOOST_FIXTURE_TEST_CASE(ProcessCompleteAddedFilter, PackSubscriptionActorTests)
{
    CompleteFilterTest(
        TSubscriptionActor::TState::AddedFiltration,
        TSubscriptionActor::TEvent::AddedFiltrationCompleted);
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedFilter, PackSubscriptionActorTests)
{
    FailedFilterTest(TSubscriptionActor::TState::Filtration);
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedDeletedFilter, PackSubscriptionActorTests)
{
    FailedFilterTest(TSubscriptionActor::TState::DeletedFiltration);
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedAddedFilter, PackSubscriptionActorTests)
{
    FailedFilterTest(TSubscriptionActor::TState::AddedFiltration);
}

BOOST_FIXTURE_TEST_CASE(ProcessUpdating, PackSubscriptionActorTests)
{
    PreprocessCheckState(TSubscriptionActor::TState::Updating);
    EXPECT_CALL(Actor.State, ChangeState(Eq(TSubscriptionActor::TEvent::RawDataUpdated)));
    BOOST_CHECK(Actor.Process());
}


BOOST_AUTO_TEST_SUITE_END()
}
