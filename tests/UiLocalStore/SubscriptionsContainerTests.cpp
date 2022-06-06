#include "UiLocalStore/SubscriptionsContainerTests.hpp"

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_SubscriptionsContainerTests)

using namespace TradingSerialization::Table;

struct TSubscriptionsContainerSetup
{
    using TSubscriptionActor = Actor;
    static constexpr size_t MaxUpdatedSubscriptions = 3;
    static constexpr SubscriptionType StoreType = SubscriptionType::Table;
};

struct SubscriptionsContainerTests : public BaseTestFixture
{
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TUiFilter = TradingSerialization::Table::Filter;
    using TUiFilters = TradingSerialization::Table::FilterGroup;

    using TUiSubscription = TradingSerialization::Table::SubscribeBase;
    using TUiRequestId = TUiSubscription::TId;

    using TDataPack = Basis::Pack<DummyTableItem>;

    using TSubscriptionsContainer = SubscriptionsContainer<TSubscriptionsContainerSetup>;
    using TSubscriptionActor = Actor;

    using TActorPack = Basis::Vector<std::shared_ptr<ISubscriptionActor::Logic<Actor>>>;

    Basis::Tracer& Tracer;

    TDataVersion Version = 50;

    TSubscriptionsContainer Container;

    TUiRequestId Request1 = Actor::MakeRequestId();
    TUiRequestId Request2 = Actor::MakeRequestId();
    TUiRequestId Request3 = Actor::MakeRequestId();
    TUiRequestId Request4 = Actor::MakeRequestId();
    TUiRequestId Request5 = Actor::MakeRequestId();

    SubscriptionsContainerTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart()))
        , Container(Tracer)
    {
        std::srand(unsigned(std::time(nullptr)));
    }

    template <typename ...TArgs>
    auto MakeActor(TArgs&& ...aArgs)
    {
        return Basis::MakeShared<ISubscriptionActor::Logic<Actor>>(std::forward<TArgs>(aArgs)...);
    }

    auto EmplaceSubscriptions()
    {
        TActorPack result
        {{
            MakeActor(Request1, 1),
            MakeActor(Request2, 1),
            MakeActor(Request3, 1),
            MakeActor(Request4, 2)
        }};

        for (const auto& actor : result)
        {
            BOOST_CHECK(Container.EmplaceSubscription(actor));
        }
        return result;
    }

    void CompareUiRequests(
        Basis::Vector<TUiRequestId> aActual,
        Basis::Vector<TUiRequestId> aExpected)
    {
        std::sort(aActual.begin(), aActual.end());
        std::sort(aExpected.begin(), aExpected.end());

        BOOST_CHECK_EQUAL_COLLECTIONS(
            aExpected.cbegin(), aExpected.cend(),
            aActual.cbegin(), aActual.cend());
    }

    void CheckVersion(TDataVersion aVersion)
    {
        auto version = Container.GetOldestVersion();
        BOOST_REQUIRE(version);
        BOOST_CHECK_EQUAL(aVersion, *version);
    }

    void CheckProcess(ISubscriptionActor::Logic<Actor>& aActor)
    {
        EXPECT_CALL(aActor, Process()).WillOnce(Return(true));
        auto result = Container.ProcessNextSubscription<DummyTableItem>(2);

        BOOST_CHECK(result.Processed);
        BOOST_CHECK(!result.SubscriptionId);
        BOOST_CHECK(!result.Result.HasValue());
    }

    auto GenerateResult()
    {
        static constexpr int64_t resultSize = 50;
        Basis::Pack<DummyTableItem> tmpResult;
        auto size = rand() % resultSize;
        for (int64_t i = 0; i < size; ++i)
        {
            tmpResult.push_back(Basis::MakeSPtr<DummyTableItem>(rand() % resultSize));
        }
        return Basis::MakeSPtr(std::move(tmpResult));
    }

    void ProcessTest(
        ISubscriptionActor::Logic<Actor>& aActor,
        TDataVersion aVersion,
        const std::optional<TUiRequestId>& aExpectedProcessedRequest = std::nullopt,
        bool aExpectedProcessed = true,
        Actor::TState aNewState = Actor::TState::Ok,
        bool aIncreaseVersion = false)
    {
        auto expectedResult = GenerateResult();

        EXPECT_CALL(aActor, Process()).WillOnce(Invoke([=, &aActor]()
        {
            aActor.Get().State = aNewState;
            if (aNewState == Actor::TState::Ok)
            {
                aActor.Get().Result = expectedResult;
            }
            return true;
        }));
        if (aIncreaseVersion)
        {
            EXPECT_CALL(aActor, IncreaseVersion()).WillOnce(Invoke([&aActor]()
            {
                ++aActor.Get().DataVersion;
                aActor.Get().State = Actor::TState::Updating;
            }));
        }
        auto result = Container.ProcessNextSubscription<DummyTableItem>(aVersion);

        BOOST_CHECK_EQUAL(aExpectedProcessed, result.Processed);

        BOOST_REQUIRE(result.SubscriptionId);
        BOOST_CHECK_EQUAL(aExpectedProcessedRequest.has_value(), result.SubscriptionId.has_value());
        if (aExpectedProcessedRequest && result.SubscriptionId)
        {
            BOOST_CHECK_EQUAL(*aExpectedProcessedRequest, *result.SubscriptionId);
        }
        BOOST_CHECK_EQUAL(expectedResult, result.Result);
    }

    void ProcessEmpty(TDataVersion aVersion)
    {
        auto result = Container.ProcessNextSubscription<DummyTableItem>(aVersion);
        BOOST_CHECK(!result.Processed);
        BOOST_CHECK(!result.SubscriptionId);
        BOOST_CHECK(!result.Result.HasValue());
    }

    void UpdateSubscription(
        TDataVersion aVersion,
        const TActorPack& aActors,
        bool aExpectedResult = true)
    {
        for (const auto& actor : aActors)
        {
            auto actorPtr = actor.get();
            EXPECT_CALL(*actorPtr, IncreaseVersion()).WillOnce(Invoke([actorPtr]()
            {
                ++(actorPtr->Get().DataVersion);
                actorPtr->Get().State = Actor::TState::Updating;
            }));
        }

        BOOST_CHECK_EQUAL(aExpectedResult, Container.UpdateSubscriptions(aVersion));
    }
};

BOOST_FIXTURE_TEST_CASE(SuccessEmplaceTest, SubscriptionsContainerTests)
{
    EmplaceSubscriptions();
}

BOOST_FIXTURE_TEST_CASE(FailedEmplaceTest, SubscriptionsContainerTests)
{
    EmplaceSubscriptions();

    BOOST_CHECK(!Container.EmplaceSubscription(MakeActor(Request1, 1)));
    BOOST_CHECK(!Container.EmplaceSubscription(MakeActor(Request2, 1)));
    BOOST_CHECK(!Container.EmplaceSubscription(MakeActor(Request3, 1)));
    BOOST_CHECK(!Container.EmplaceSubscription(MakeActor(Request4, 2)));
}

BOOST_FIXTURE_TEST_CASE(ClearTest, SubscriptionsContainerTests)
{
    EmplaceSubscriptions();

    CompareUiRequests(
        Container.Clear(),
        Basis::Vector<TUiRequestId> {{ Request1, Request2, Request3, Request4 }});
}

BOOST_FIXTURE_TEST_CASE(EraseTest, SubscriptionsContainerTests)
{
    EmplaceSubscriptions();

    Container.EraseSubscription(Request1);
    Container.EraseSubscription(Request4);

    CompareUiRequests(
        Container.Clear(),
        Basis::Vector<TUiRequestId> {{ Request2, Request3 }});
}

BOOST_FIXTURE_TEST_CASE(GetRejectedRequestsTest, SubscriptionsContainerTests)
{
    EmplaceSubscriptions();

    CompareUiRequests(
        Container.GetRejectedSubscriptions(),
        Basis::Vector<TUiRequestId> {});
}

BOOST_FIXTURE_TEST_CASE(GetOldestVersionTest, SubscriptionsContainerTests)
{
    BOOST_TEST_CONTEXT("Initial none")
    {
        BOOST_CHECK(!Container.GetOldestVersion());
    }

    BOOST_TEST_CONTEXT("Version 1")
    {
        EmplaceSubscriptions();
        CheckVersion(1);
    }

    BOOST_TEST_CONTEXT("Version 1 after erase")
    {
        Container.EraseSubscription(Request1);
        Container.EraseSubscription(Request2);

        CheckVersion(1);
    }

    BOOST_TEST_CONTEXT("Version 2")
    {
        Container.EraseSubscription(Request3);
        CheckVersion(2);
    }

    BOOST_TEST_CONTEXT("Version 2 after emplace")
    {
        BOOST_CHECK(
            Container.EmplaceSubscription(MakeActor(Request5, 100)));
        CheckVersion(2);
    }

    BOOST_TEST_CONTEXT("Version 100")
    {
        Container.EraseSubscription(Request4);
        CheckVersion(100);
    }

    BOOST_TEST_CONTEXT("Final none")
    {
        Container.EraseSubscription(Request5);
        BOOST_CHECK(!Container.GetOldestVersion());
    }
}

BOOST_FIXTURE_TEST_CASE(ProcessEmptyTest, SubscriptionsContainerTests)
{
    ProcessEmpty(1);
}

BOOST_FIXTURE_TEST_CASE(SequentialProcessTest, SubscriptionsContainerTests)
{
    auto subscriptions = EmplaceSubscriptions();

    BOOST_TEST_CONTEXT("First process") { CheckProcess(*subscriptions[0]); }
    BOOST_TEST_CONTEXT("Second process") { CheckProcess(*subscriptions[1]); }
    BOOST_TEST_CONTEXT("Third process") { CheckProcess(*subscriptions[2]); }
    BOOST_TEST_CONTEXT("Fourth process") { CheckProcess(*subscriptions[3]); }
    BOOST_TEST_CONTEXT("Fifth process") { CheckProcess(*subscriptions[0]); }
}

BOOST_FIXTURE_TEST_CASE(SequentialSingleProcessTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));

    BOOST_TEST_CONTEXT("First process") { CheckProcess(*actor); }
    BOOST_TEST_CONTEXT("Second process") { CheckProcess(*actor); }
    BOOST_TEST_CONTEXT("Third process") { CheckProcess(*actor); }
}

BOOST_FIXTURE_TEST_CASE(SingleFinalProcessTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));
    ProcessTest(*actor, 1, Request1, true);
}

BOOST_FIXTURE_TEST_CASE(SingleFinalProcessAndUpdateVersionTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));
    ProcessTest(*actor, 2, Request1, true, Actor::TState::Ok, true);
}

BOOST_FIXTURE_TEST_CASE(SingleFinalProcessAndUpdateForwardVersionTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));
    ProcessTest(*actor, 10, Request1, true, Actor::TState::Ok, true);
}

BOOST_FIXTURE_TEST_CASE(SequentialProcessWithFinalOneTest, SubscriptionsContainerTests)
{
    auto subscriptions = EmplaceSubscriptions();

    BOOST_TEST_CONTEXT("First process")
    {
        ProcessTest(*subscriptions[0], 1, Request1, true, Actor::TState::Ok);
    }
    BOOST_TEST_CONTEXT("Second process") { CheckProcess(*subscriptions[1]); }
    BOOST_TEST_CONTEXT("Third process") { CheckProcess(*subscriptions[2]); }
    BOOST_TEST_CONTEXT("Fourth process") { CheckProcess(*subscriptions[3]); }
    BOOST_TEST_CONTEXT("Fifth process") { CheckProcess(*subscriptions[1]); }
}

BOOST_FIXTURE_TEST_CASE(SequentialProcessWithFinalTwoTest, SubscriptionsContainerTests)
{
    auto subscriptions = EmplaceSubscriptions();

    BOOST_TEST_CONTEXT("First process") { CheckProcess(*subscriptions[0]); }
    BOOST_TEST_CONTEXT("Second process")
    {
        ProcessTest(*subscriptions[1], 1, Request2, true, Actor::TState::Ok);
    }
    BOOST_TEST_CONTEXT("Third process")
    {
        ProcessTest(*subscriptions[2], 1, Request3, true, Actor::TState::Ok);
    }
    BOOST_TEST_CONTEXT("Fourth process") { CheckProcess(*subscriptions[3]); }
    BOOST_TEST_CONTEXT("Fifth process") { CheckProcess(*subscriptions[0]); }
    BOOST_TEST_CONTEXT("Sixth process") { CheckProcess(*subscriptions[3]); }
}

BOOST_FIXTURE_TEST_CASE(ProcessAfterAllProcessedTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));
    ProcessTest(*actor, 1, Request1, true);
    ProcessEmpty(1);
}

BOOST_FIXTURE_TEST_CASE(ProcessWithUpdateTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));
    ProcessTest(*actor, 2, Request1, true, Actor::TState::Ok, true);
    ProcessTest(*actor, 2, Request1, true, Actor::TState::Ok, false);
    ProcessEmpty(1);
}

BOOST_FIXTURE_TEST_CASE(ProcessWithForwardUpdateTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));
    ProcessTest(*actor, 3, Request1, true, Actor::TState::Ok, true);
    ProcessTest(*actor, 3, Request1, true, Actor::TState::Ok, true);
    ProcessTest(*actor, 3, Request1, true, Actor::TState::Ok, false);
    ProcessEmpty(1);
}

BOOST_FIXTURE_TEST_CASE(UpdateEmptyTest, SubscriptionsContainerTests)
{
    UpdateSubscription(2, TActorPack {});
}

BOOST_FIXTURE_TEST_CASE(UpdateSubscriptionTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));
    ProcessTest(*actor, 1, Request1, true);
    ProcessEmpty(1);

    UpdateSubscription(2, TActorPack { actor });
}

BOOST_FIXTURE_TEST_CASE(DontUpdateProcessingSubscriptionTest, SubscriptionsContainerTests)
{
    auto actor = MakeActor(Request1, 1);
    BOOST_CHECK(Container.EmplaceSubscription(actor));

    UpdateSubscription(2, TActorPack {});
}

BOOST_FIXTURE_TEST_CASE(UpdateManySubscriptionsTest, SubscriptionsContainerTests)
{
    auto subscriptions = EmplaceSubscriptions();

    BOOST_TEST_CONTEXT("First process")
    {
        ProcessTest(*subscriptions[0], 1, Request1, true, Actor::TState::Ok);
    }
    BOOST_TEST_CONTEXT("Second process")
    {
        ProcessTest(*subscriptions[1], 1, Request2, true, Actor::TState::Ok);
    }
    BOOST_TEST_CONTEXT("Third process")
    {
        ProcessTest(*subscriptions[2], 1, Request3, true, Actor::TState::Ok);
    }
    BOOST_TEST_CONTEXT("Fourth process")
    {
        ProcessTest(*subscriptions[3], 2, Request4, true, Actor::TState::Ok);
    }

    UpdateSubscription(3, TActorPack { subscriptions[0], subscriptions[1], subscriptions[2] }, false);
    UpdateSubscription(3, TActorPack { subscriptions[3] }, true);
}

BOOST_AUTO_TEST_SUITE_END()
}
