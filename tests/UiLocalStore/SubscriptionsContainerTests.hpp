#include "DummyTableData.hpp"

#include "UiLocalStore/SubscriptionsContainer.hpp"
#include "UiLocalStore/TableSorter.hpp"
#include "UiLocalStore/TableFilterman.hpp"
#include "UiLocalStore/TableIncrementApplicator.hpp"
#include "UiLocalStore/TableIncrementMaker.hpp"
#include "TradingSerialization/Table/Columns.hpp"

#include <Basis/BaseTestFixture.hpp>
#include <Common/Pack.hpp>

#include <Common/Interface.hpp>

#include <boost/iterator/zip_iterator.hpp>
#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

struct Actor : public Basis::GMock
{
    using TUiSubscription = TradingSerialization::Table::SubscribeBase;
    using TUiRequestId = TUiSubscription::TId;

    using TState = ISubscriptionStateMachine::TableState;
    using TEvent = ISubscriptionStateMachine::TableEvent;

    const auto& GetRequestId() const { return RequestId; }
    TDataVersion GetVersion() const { return DataVersion; }
    Basis::SPtrPack<DummyTableItem> GetResult() const { return Result; }
    bool IsOk() const { return State == TState::Ok; }
    bool IsError() const { return State == TState::Error; }

    ISubscriptionStateMachine::ProcessingState GetProcessingState() const
    {
        if (State != TState::Ok
            && State != TState::Error)
        {
            return ISubscriptionStateMachine::ProcessingState::Processing;
        }
        return ISubscriptionStateMachine::ProcessingState::Idle;
    }

    bool IsPending()
    {
        return false;
    }

    Basis::Vector<int64_t> GetDeletedIds()
    {
        return {};
    }

    ISubscriptionActor::ResultState GetResultState() const
    {
        return ISubscriptionActor::ResultState::FinalResult;
    }

    Actor() = default;
    Actor(
        TUiRequestId aRequestId,
        TDataVersion aDataVersion)
        : RequestId(aRequestId)
        , DataVersion(aDataVersion)
    {}

    static auto MakeRequestId()
    {
        Basis::UniqueIdGenerator<TUiRequestId> gen;
        return gen.GetNext();
    }

    TUiRequestId RequestId;
    TDataVersion DataVersion{0};
    Basis::SPtrPack<DummyTableItem> Result;
    TState State = TState::Initializing;
};

}

namespace NTPro::Ecn::Basis
{

template <> struct TGMock<NewUiServer::Actor>
{
    template <typename T>
    using Type = ::testing::StrictMock<T>;
};

template<>
struct IsGMock<NewUiServer::Actor>
{};

template <>
struct Interface<NewUiServer::Actor, Own>
    : GMockInterface<NewUiServer::Actor>
{
    using GMockInterface<NewUiServer::Actor>::GMockInterface;
};

}
