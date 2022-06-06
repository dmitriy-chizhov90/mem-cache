#include "DummyTableData.hpp"
#include "UiCacheTestUtils.hpp"
#include "UiLocalStore/UiCacheComponent.hpp"

#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <boost/test/unit_test.hpp>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_UiCacheComponentTests)

template <SubscriptionType Type>
struct UiCacheComponentTests : public BaseTestFixture
{
    using TData = DummyTableItem;

    static constexpr SubscriptionType StoreType = Type;

    using TTechnicalControlApiClient = Basis::GMock;
    using TQueryApiWrapper = Basis::GMock;
    using TTableProcessorApiStore = Basis::GMock;
    using TLocalStoreLogic = Basis::GMock;
    using TTableProcessorApiDbReaderClient = Basis::GMock;
    using TSubscriptionRouter = Basis::GMock;

    Basis::ComponentId CacheId = "CacheId";
    Basis::Part::TId CachePartId = Basis::PartLocator::CreateComponentPart(CacheId, CacheId.GetName());

    Basis::EventRegistry Registry;

    UiCacheComponent<UiCacheComponentTests> Component;

    UiCacheComponentTests()
        : Component(
            CacheId,
            "SettingsStore",
            "DbReader",
            1, // aDbReadersCount
            Registry)
    {
    }

    void ManageRecalls()
    {
        EXPECT_CALL(Component.Logic, GetRejectedSubscriptions())
            .WillOnce(Return(Basis::Vector<TUiRequestId> {}));
        EXPECT_CALL(Component.Logic, IsRecallNeeded())
            .WillOnce(Return(false));
    }

    void ManageRecallsNeededAndSet()
    {
        EXPECT_CALL(Component.Logic, GetRejectedSubscriptions())
            .WillOnce(Return(Basis::Vector<TUiRequestId> {}));
        EXPECT_CALL(Component.Logic, IsRecallNeeded())
            .WillOnce(Return(true));
        EXPECT_CALL(Component.TableProcessor, IsRecallNeeded())
            .WillOnce(Return(true));
    }

    void ManageRecallsNeededAndNotSet(bool aForce)
    {
        EXPECT_CALL(Component.Logic, GetRejectedSubscriptions())
            .WillOnce(Return(Basis::Vector<TUiRequestId> {}));
        EXPECT_CALL(Component.Logic, IsRecallNeeded())
            .WillOnce(Return(true));
        EXPECT_CALL(Component.TableProcessor, IsRecallNeeded())
            .WillOnce(Return(false));
        EXPECT_CALL(Component.TableProcessor, SetIsRecallNeeded(Eq(true), Eq(aForce)));
    }
};

using UiChunkCacheComponentTests = UiCacheComponentTests<SubscriptionType::Chunk>;
using UiTableCacheComponentTests = UiCacheComponentTests<SubscriptionType::Table>;

BOOST_FIXTURE_TEST_CASE(TechnicalStart, UiChunkCacheComponentTests)
{
    EXPECT_CALL(Component.QueryApiWrapper, StartSession());
    EXPECT_CALL(Component.DbReader, StartSession());
    EXPECT_CALL(Component.TechnicalControlApiClient, SendState(Eq(Model::TechnicalStateType::Started)));
    Component.ProcessTechnicalStart();
}

BOOST_FIXTURE_TEST_CASE(TechnicalStop, UiChunkCacheComponentTests)
{
    EXPECT_CALL(Component.QueryApiWrapper, StopSession());
    EXPECT_CALL(Component.DbReader, StopSession());
    EXPECT_CALL(Component.Logic, Clear());
    EXPECT_CALL(Component.TechnicalControlApiClient, SendState(Eq(Model::TechnicalStateType::Stopped)));
    Component.ProcessTechnicalStop();
}

BOOST_FIXTURE_TEST_CASE(ProcessSubscription, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;

    EXPECT_CALL(Component.Logic, IsReady()).WillOnce(Return(true));
    EXPECT_CALL(Component.SubscriptionRouter, IsDbQuery(_)).WillOnce(Return(false));
    EXPECT_CALL(Component.Logic, ProcessSubscription(Truly(UiRequestsComparer {requestId}), _))
        .WillOnce(Return(std::nullopt));
    ManageRecalls();

    Component.ProcessSubscription(requestId, subscription, nullptr, SubscriptionType::Chunk);
}

BOOST_FIXTURE_TEST_CASE(ProcessSubscriptionRecallsNeededAndSet, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;

    EXPECT_CALL(Component.Logic, IsReady()).WillOnce(Return(true));
    EXPECT_CALL(Component.SubscriptionRouter, IsDbQuery(_)).WillOnce(Return(false));
    EXPECT_CALL(Component.Logic, ProcessSubscription(Truly(UiRequestsComparer {requestId}), _))
        .WillOnce(Return(std::nullopt));
    ManageRecallsNeededAndSet();

    Component.ProcessSubscription(requestId, subscription, nullptr, SubscriptionType::Chunk);
}

BOOST_FIXTURE_TEST_CASE(ProcessSubscriptionRecallsNeededAndNotSet, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;

    EXPECT_CALL(Component.Logic, IsReady()).WillOnce(Return(true));
    EXPECT_CALL(Component.SubscriptionRouter, IsDbQuery(_)).WillOnce(Return(false));
    EXPECT_CALL(Component.Logic, ProcessSubscription(Truly(UiRequestsComparer {requestId}), _))
        .WillOnce(Return(std::nullopt));
    ManageRecallsNeededAndNotSet(false);

    Component.ProcessSubscription(requestId, subscription, nullptr, SubscriptionType::Chunk);
}

BOOST_FIXTURE_TEST_CASE(RejectNewSubscription, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;
    const auto reason = TableProcessorRejectType::Disconnected;

    EXPECT_CALL(Component.Logic, IsReady()).WillOnce(Return(true));
    EXPECT_CALL(Component.SubscriptionRouter, IsDbQuery(_)).WillOnce(Return(false));
    EXPECT_CALL(Component.Logic, ProcessSubscription(Truly(UiRequestsComparer {requestId}), _))
        .WillOnce(Return(reason));
    EXPECT_CALL(Component.TableProcessor, RejectSubscription(Truly(UiRequestsComparer {requestId}), Eq(reason)));

    Component.ProcessSubscription(requestId, subscription, nullptr, SubscriptionType::Chunk);
}

BOOST_FIXTURE_TEST_CASE(RejectTableSubscription, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;
    EXPECT_CALL(
        Component.TableProcessor,
        RejectSubscription(Truly(UiRequestsComparer {requestId}), Eq(TableProcessorRejectType::WrongSubscription)));

    Component.ProcessSubscription(requestId, subscription, nullptr, SubscriptionType::Table);
}

BOOST_FIXTURE_TEST_CASE(RejectChunkSubscription, UiTableCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TradingSerialization::Table::SubscribeBase subscription;
    EXPECT_CALL(
        Component.TableProcessor,
        RejectSubscription(Truly(UiRequestsComparer {requestId}), Eq(TableProcessorRejectType::WrongSubscription)));

    Component.ProcessSubscription(requestId, subscription, nullptr, SubscriptionType::Chunk);
}

BOOST_FIXTURE_TEST_CASE(ProcessUnsubscription, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    EXPECT_CALL(Component.Logic, ProcessUnsubscription(Truly(UiRequestsComparer {requestId})));
    Component.ProcessUnsubscription(requestId);
}

BOOST_FIXTURE_TEST_CASE(ProcessChunkGetNext, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TResult expectedResult;
    expectedResult.Processed = true;
    expectedResult.ResultState = ISubscriptionActor::ResultState::IntermediateResult;
    expectedResult.SubscriptionId = requestId;

    EXPECT_CALL(Component.Logic, ProcessGetNext(Truly(UiRequestsComparer {requestId})))
        .WillOnce(Return(expectedResult));
    EXPECT_CALL(Component.TableProcessor, SendChunkSnapshot(Truly(UiRequestsComparer {requestId}), _, _, _));
    ManageRecalls();

    Component.ProcessGetNext(requestId);
}

BOOST_FIXTURE_TEST_CASE(ProcessTableGetNext, UiTableCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TResult expectedResult;
    expectedResult.Processed = true;
    expectedResult.ResultState = ISubscriptionActor::ResultState::IntermediateResult;
    expectedResult.SubscriptionId = requestId;
    expectedResult.Result = Basis::MakeSPtrPack<DummyTableItem>();

    EXPECT_CALL(Component.Logic, ProcessGetNext(Truly(UiRequestsComparer {requestId})))
        .WillOnce(Return(expectedResult));
    EXPECT_CALL(Component.TableProcessor, SendDataSnapshot(Truly(UiRequestsComparer {requestId}), _));
    ManageRecalls();

    Component.ProcessGetNext(requestId);
}

BOOST_FIXTURE_TEST_CASE(ProcessRecall, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    TResult expectedResult;
    expectedResult.Processed = true;
    expectedResult.ResultState = ISubscriptionActor::ResultState::IntermediateResult;
    expectedResult.SubscriptionId = requestId;

    EXPECT_CALL(Component.TableProcessor, SetIsRecallNeeded(Eq(false), Eq(false)));
    EXPECT_CALL(Component.Logic, ProcessDefferedTasks())
        .WillOnce(Return(expectedResult));
    EXPECT_CALL(Component.TableProcessor, SendChunkSnapshot(Truly(UiRequestsComparer {requestId}), _, _, _));
    ManageRecalls();

    Component.ProcessRecall(Basis::DateTime {});
}

BOOST_FIXTURE_TEST_CASE(ProcessDataUpdate, UiChunkCacheComponentTests)
{
    auto item1 = Basis::MakeSPtr<Model::DataWithAction<Data>>(101, Model::ActionType::New);
    auto item2 = Basis::MakeSPtr<Model::DataWithAction<Data>>(102, Model::ActionType::New);
    auto item3 = Basis::MakeSPtr<Model::DataWithAction<Data>>(103, Model::ActionType::New);

    using TPack = Basis::Pack<Model::DataWithAction<Data>>;
    TPack modelDataPack {{ item1, item2, item3 }};

    EXPECT_CALL(Component.Logic, ProcessDataUpdate<Basis::Pack<Model::DataWithAction<Data>>>(_));
    ManageRecallsNeededAndNotSet(true);

    Component.ProcessDataUpdate(Basis::MakeSPtr<TPack>(modelDataPack));
}

BOOST_FIXTURE_TEST_CASE(ProcessApiConnected, UiChunkCacheComponentTests)
{
    auto requestId = MakeRequestId();
    EXPECT_CALL(Component.Logic, Clear())
        .WillOnce(Return(Basis::Vector<TUiRequestId> { requestId }));

    EXPECT_CALL(
        Component.TableProcessor,
        RejectSubscription(Truly(UiRequestsComparer {requestId}), Eq(TableProcessorRejectType::Disconnected)));

    Component.ProcessQueryApiConnected();
}

BOOST_AUTO_TEST_SUITE_END()
}
