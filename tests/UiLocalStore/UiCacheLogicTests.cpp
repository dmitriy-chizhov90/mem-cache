#include "DummyTableData.hpp"
#include "UiCacheTestUtils.hpp"

#include "UiLocalStore/UiCacheLogic.hpp"

#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <boost/test/unit_test.hpp>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_UiCacheLogicTests)

struct UiCacheLogicTests : public BaseTestFixture
{
    using TData = DummyTableItem;
    using TMap = DummyTableItemMultiIndex;
    using TQueryApiPack = Basis::Pack<Model::DataWithAction<Data>>;

    using TDataItemBuilder = Basis::GMock;
    using TLocalStoreStateMachine = Basis::GMock;
    using TSubscriptionsContainer = Basis::GMock;
    using TSubscriptionActor = Basis::GMock;

    Basis::Tracer& Tracer;

    UiCacheLogic<UiCacheLogicTests> Logic;

    UiCacheLogicTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart()))
        , Logic(Tracer)
    {
    }

    void ExpectGetState(ILocalStoreStateMachine::State aState)
    {
        EXPECT_CALL(Logic.StateMachine, GetState()).WillOnce(Return(aState));
    }

    void ExpectChangeState(ILocalStoreStateMachine::Event aExpectedEvent)
    {
        EXPECT_CALL(Logic.StateMachine, ChangeState(Eq(aExpectedEvent)));
    }

    void ExpectEmplaceSubscription(bool aResult)
    {
        EXPECT_CALL(
            Logic.Subscriptions,
            EmplaceSubscription<std::shared_ptr<ISubscriptionActor::Logic<Basis::GMock>>>(_))
            .WillOnce(Return(aResult));
    }


    void ExpectEraseSubscription(const TUiSubscription::TId& aExpectedRequestId)
    {
        EXPECT_CALL(Logic.Subscriptions, EraseSubscription(Truly(UiRequestsComparer {aExpectedRequestId})));
    }

    void ProcessSubscription(
        const TUiSubscription::TId& aRequestId,
        const TradingSerialization::Table::SubscribeBase& aSubscription,
        std::optional<TableProcessorRejectType> aExpectedResult)
    {
        auto result = Logic.ProcessSubscription(aRequestId, aSubscription);
        BOOST_REQUIRE_EQUAL(result.has_value(), aExpectedResult.has_value());
        if (result.has_value())
        {
            BOOST_CHECK_EQUAL(*result, *aExpectedResult);
        }
    }

    void ProcessUnsubscription(const TUiSubscription::TId& aRequestId)
    {
        Logic.ProcessUnsubscription(aRequestId);
    }
};

BOOST_FIXTURE_TEST_CASE(RejectNotReadySubscription, UiCacheLogicTests)
{
    const auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;

    ExpectGetState(ILocalStoreStateMachine::State::NotReady);
    ProcessSubscription(requestId, subscription, TableProcessorRejectType::Disconnected);
}

BOOST_FIXTURE_TEST_CASE(RejectWrongSubscription, UiCacheLogicTests)
{
    const auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;

    ExpectGetState(ILocalStoreStateMachine::State::Idle);
    ExpectEmplaceSubscription(false);
    ProcessSubscription(requestId, subscription, TableProcessorRejectType::WrongSubscription);
}


BOOST_FIXTURE_TEST_CASE(ProcesSubscription, UiCacheLogicTests)
{
    const auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;

    ExpectGetState(ILocalStoreStateMachine::State::Idle);
    ExpectEmplaceSubscription(true);
    ExpectChangeState(ILocalStoreStateMachine::Event::NewRequestReceived);
    ProcessSubscription(requestId, subscription, std::nullopt);
}

BOOST_FIXTURE_TEST_CASE(ProcesUnsubscription, UiCacheLogicTests)
{
    const auto requestId = MakeRequestId();

    ExpectEraseSubscription(requestId);
    ProcessUnsubscription(requestId);
}

BOOST_FIXTURE_TEST_CASE(ProcesClear, UiCacheLogicTests)
{
    const auto requestId = MakeRequestId();
    Basis::Vector<TUiSubscription::TId> requests { requestId };

    ExpectChangeState(ILocalStoreStateMachine::Event::ApiDisconnected);
    EXPECT_CALL(Logic.Subscriptions, Clear())
        .WillOnce(Return(requests));
    auto result = Logic.Clear();

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_CHECK_EQUAL(result[0], requestId);
}

BOOST_FIXTURE_TEST_CASE(ProcesGetRejectedSubscriptions, UiCacheLogicTests)
{
    const auto requestId = MakeRequestId();
    Basis::Vector<TUiSubscription::TId> requests { requestId };

    EXPECT_CALL(Logic.Subscriptions, GetRejectedSubscriptions())
        .WillOnce(Return(requests));
    auto result = Logic.GetRejectedSubscriptions();

    BOOST_REQUIRE_EQUAL(result.size(), 1);
    BOOST_CHECK_EQUAL(result[0], requestId);
}

BOOST_FIXTURE_TEST_CASE(ProcesUpdateData, UiCacheLogicTests)
{
    auto item1 = Basis::MakeSPtr<Model::DataWithAction<Data>>(101, Model::ActionType::New);
    auto item2 = Basis::MakeSPtr<Model::DataWithAction<Data>>(102, Model::ActionType::New);
    auto item3 = Basis::MakeSPtr<Model::DataWithAction<Data>>(103, Model::ActionType::New);

    using TPack = Basis::Pack<Model::DataWithAction<Data>>;
    TPack modelDataPack {{ item1, item2, item3 }};
    InSequence seq;
    EXPECT_CALL(Logic.Data.ItemBuilder, CreateItem<decltype(item1)>(Eq(item1)))
        .WillOnce(Return(Basis::MakeSPtr<TData>(101, "101")));
    EXPECT_CALL(Logic.Data.ItemBuilder, CreateItem<decltype(item1)>(Eq(item2)))
        .WillOnce(Return(Basis::MakeSPtr<TData>(102, "102")));
    EXPECT_CALL(Logic.Data.ItemBuilder, CreateItem<decltype(item1)>(Eq(item3)))
        .WillOnce(Return(Basis::MakeSPtr<TData>(103, "103")));

    ExpectChangeState(ILocalStoreStateMachine::Event::UpdatesReceived);
    Logic.ProcessDataUpdate(Basis::MakeSPtr<TPack>(modelDataPack));
}

BOOST_FIXTURE_TEST_CASE(CheckIsRecallNeeded, UiCacheLogicTests)
{
    ExpectGetState(ILocalStoreStateMachine::State::Processing);
    BOOST_CHECK(Logic.IsRecallNeeded());
}

BOOST_FIXTURE_TEST_CASE(CheckIsRecallNotNeeded, UiCacheLogicTests)
{
    ExpectGetState(ILocalStoreStateMachine::State::Idle);
    BOOST_CHECK(!Logic.IsRecallNeeded());
}

BOOST_FIXTURE_TEST_CASE(SkipDefferedTasks, UiCacheLogicTests)
{
    ExpectGetState(ILocalStoreStateMachine::State::Idle);
    auto result = Logic.ProcessDefferedTasks();
    BOOST_CHECK(!result.Processed);
}

BOOST_FIXTURE_TEST_CASE(ProcessDefferedTasks, UiCacheLogicTests)
{
    TResult expectedResult;
    expectedResult.Processed = true;
    expectedResult.ResultState = ISubscriptionActor::ResultState::IntermediateResult;
    expectedResult.SubscriptionId = MakeRequestId();

    ExpectGetState(ILocalStoreStateMachine::State::Processing);
    EXPECT_CALL(Logic.Subscriptions, ProcessNextSubscription<TData>(Eq(0)))
        .WillOnce(Return(expectedResult));

    auto result = Logic.ProcessDefferedTasks();
    CompareValidResults(result, expectedResult);
}

BOOST_FIXTURE_TEST_CASE(ProcessAllTasks, UiCacheLogicTests)
{
    TResult expectedResult;
    expectedResult.Processed = false;

    ExpectGetState(ILocalStoreStateMachine::State::Processing);
    EXPECT_CALL(Logic.Subscriptions, ProcessNextSubscription<TData>(Eq(0)))
        .WillOnce(Return(expectedResult));
    ExpectChangeState(ILocalStoreStateMachine::Event::AllProcessed);
    auto result = Logic.ProcessDefferedTasks();
    BOOST_CHECK(!result.Processed);
}

BOOST_FIXTURE_TEST_CASE(ProcessFailedTasks, UiCacheLogicTests)
{
    TResult expectedResult;
    expectedResult.Processed = true;

    ExpectGetState(ILocalStoreStateMachine::State::Processing);
    EXPECT_CALL(Logic.Subscriptions, ProcessNextSubscription<TData>(Eq(0)))
        .WillOnce(Return(expectedResult));

    auto result = Logic.ProcessDefferedTasks();
    BOOST_CHECK(!result.Processed);
}

BOOST_FIXTURE_TEST_CASE(UpdateSomeTasks, UiCacheLogicTests)
{
    TResult expectedResult;
    expectedResult.Processed = true;

    ExpectGetState(ILocalStoreStateMachine::State::Updating);
    EXPECT_CALL(Logic.Subscriptions, UpdateSubscriptions(Eq(0))).WillOnce(Return(false));
    auto result = Logic.ProcessDefferedTasks();
    BOOST_CHECK(!result.Processed);
}

BOOST_FIXTURE_TEST_CASE(UpdateAllTasks, UiCacheLogicTests)
{
    TResult expectedResult;
    expectedResult.Processed = true;

    ExpectGetState(ILocalStoreStateMachine::State::Updating);
    EXPECT_CALL(Logic.Subscriptions, UpdateSubscriptions(Eq(0))).WillOnce(Return(true));
    EXPECT_CALL(Logic.Subscriptions, GetOldestVersion()).WillOnce(Return(0));
    ExpectChangeState(ILocalStoreStateMachine::Event::AllReadyToProcessing);
    auto result = Logic.ProcessDefferedTasks();
    BOOST_CHECK(!result.Processed);
}

BOOST_FIXTURE_TEST_CASE(DontProcessGetNextIfNotReady, UiCacheLogicTests)
{
    auto requestId = MakeRequestId();
    ExpectGetState(ILocalStoreStateMachine::State::NotReady);
    auto result = Logic.ProcessGetNext(requestId);
    BOOST_CHECK(!result.Processed);
}

BOOST_FIXTURE_TEST_CASE(ProcessGetNextWithoutResult, UiCacheLogicTests)
{
    auto requestId = MakeRequestId();
    TResult expectedResult;
    expectedResult.Processed = false;

    ExpectGetState(ILocalStoreStateMachine::State::Processing);
    EXPECT_CALL(Logic.Subscriptions, ProcessGetNext<TData>(Truly(UiRequestsComparer { requestId }), Eq(0)))
        .WillOnce(Return(expectedResult));
    auto result = Logic.ProcessGetNext(requestId);
    BOOST_CHECK(!result.Processed);
}

BOOST_FIXTURE_TEST_CASE(ProcessGetNext, UiCacheLogicTests)
{
    auto requestId = MakeRequestId();
    TResult expectedResult;
    expectedResult.Processed = true;
    expectedResult.ResultState = ISubscriptionActor::ResultState::IntermediateResult;
    expectedResult.SubscriptionId = requestId;

    ExpectGetState(ILocalStoreStateMachine::State::Processing);
    EXPECT_CALL(Logic.Subscriptions, ProcessGetNext<TData>(Truly(UiRequestsComparer { requestId }), Eq(0)))
        .WillOnce(Return(expectedResult));
    ExpectChangeState(ILocalStoreStateMachine::Event::NewRequestReceived);
    auto result = Logic.ProcessGetNext(requestId);
    CompareValidResults(result, expectedResult);
}


BOOST_FIXTURE_TEST_CASE(ProcessGetNextWithInvalidResult, UiCacheLogicTests)
{
    auto requestId = MakeRequestId();
    TResult expectedResult;
    expectedResult.Processed = true;
    expectedResult.ResultState = ISubscriptionActor::ResultState::NoResult;

    ExpectGetState(ILocalStoreStateMachine::State::Processing);
    EXPECT_CALL(Logic.Subscriptions, ProcessGetNext<TData>(Truly(UiRequestsComparer { requestId }), Eq(0)))
        .WillOnce(Return(expectedResult));
    ExpectChangeState(ILocalStoreStateMachine::Event::NewRequestReceived);
    auto result = Logic.ProcessGetNext(requestId);
    BOOST_CHECK(!result.Processed);
}


BOOST_AUTO_TEST_SUITE_END()
}
