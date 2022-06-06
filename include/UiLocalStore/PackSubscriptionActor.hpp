#pragma once

#include "ITableProcessorApi.hpp"

#include "UiLocalStore/IIteratorRanges.hpp"
#include "UiLocalStore/IDataRanges.hpp"
#include "UiLocalStore/ISubscriptionActor.hpp"
#include "UiLocalStore/ISubscriptionStateMachine.hpp"
#include "UiLocalStore/ITableFilterman.hpp"
#include "UiLocalStore/ITableIncrementApplicator.hpp"
#include "UiLocalStore/ITableIncrementMaker.hpp"
#include "UiLocalStore/ITableSorter.hpp"
#include "UiLocalStore/LocalStoreUtils.hpp"
#include "UiLocalStore/VersionedDataContainer.hpp"

#include <Common/Tracer.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Обработчик подписки на табличные данные.
 * \ingroup NewUiServer
 * Подготавливает данные для подписки и содержит ее состояние.
 */
template <typename TSetup>
class PackSubscriptionActor
{
public:
    using TData = typename TSetup::TData;
    using TDataPack = typename ITableProcessorApi<Basis::Pack<TData>>::TDataPack;
    using TMap = typename TSetup::TMap;

    using TUiSubscription = TradingSerialization::Table::SubscribeBase;
    using TProcessingResult = Basis::Pack<TData>;
    using TCompletedResult = Basis::SPtrPack<TData>;

    using TUiFilters = TradingSerialization::Table::FilterGroup;
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TUiFilterValues = TradingSerialization::Table::TFilterValues;
    using TIntFilter = TradingSerialization::Table::TInt;
    using TFilterOperator = TradingSerialization::Table::FilterOperator;

    using TState = ISubscriptionStateMachine::PackState;
    using TEvent = ISubscriptionStateMachine::PackEvent;

    using TFiltermanImpl = typename TSetup::TTableIndexFilterman;
    using TFiltermanInit = typename TSetup::TTableIndexFiltermanInit;
    using IFilterman = ITableFilterman::Performer<TFiltermanImpl>;

    using IRanges = typename IDataRanges<TData>::template Ranges<typename TSetup::TIndexedDataRanges>;
    using IRangesInit = typename TSetup::TIndexedDataRangesInit;
    using IState = ISubscriptionStateMachine::Machine<TState, TEvent, typename TSetup::TSubscriptionStateMachine>;

private:

    Basis::Tracer& mTracer;

    TUiSubscription::TId mRequestId;
    TUiSubscription mSubscription;

    const TMap& mRawData;

    mutable TProcessingResult mProcessedResult;
    mutable TCompletedResult mCompletedResult;
    mutable ISubscriptionActor::TDeletedIds mProcessedDeletedIds;
    mutable ISubscriptionActor::TDeletedIds mCompletedDeletedIds;
    mutable ISubscriptionActor::ResultState mResultState = ISubscriptionActor::ResultState::NoResult;

    TDataVersion mVersion = 0;

    Basis::Set<int64_t> mAddedIds;
    bool mNeedFinalPack = false;

public:
    IState State;

    IRanges Ranges;
    IFilterman Filterman;

public:
    PackSubscriptionActor(
        Basis::Tracer& aTracer,
        const TUiSubscription::TId& aRequestId,
        const TUiSubscription& aSubscription,
        const TMap& aRawData,
        TDataVersion aVersion)
        : mTracer(aTracer)
        , mRequestId(aRequestId)
        , mSubscription(aSubscription)
        , mRawData(aRawData)
        , mVersion(aVersion)
        , State(mTracer)
        , Ranges(mTracer)
        , Filterman(mTracer)

    {
        mTracer.Info("New subscription: snapshot size");
    }

    const TUiSubscription::TId& GetRequestId() const
    {
        return mRequestId;
    }

    TDataVersion GetVersion() const
    {
        return mVersion;
    }

    TState GetState() const
    {
        return State.GetState();
    }

    bool IsOk() const
    {
        return State.GetState() == TState::Ok;
    }

    bool IsPending() const
    {
        return State.GetProcessingState() == ISubscriptionStateMachine::ProcessingState::Pending;
    }

    bool IsError() const
    {
        return State.GetState() == TState::Error;
    }

    auto GetProcessingState() const
    {
        return State.GetProcessingState();
    }

    void IncreaseVersion()
    {
        ++mVersion;
        State.ChangeState(TEvent::UpdatesReceived);
    }

    bool ProcessGetNext()
    {
        State.ChangeState(TEvent::GetNextReceived);
        return true;
    }

    bool Process()
    {
        if (GetProcessingState() != ISubscriptionStateMachine::ProcessingState::Processing)
        {
            assert(false);
            return false;
        }
        switch (State.GetState())
        {
        case TState::Initializing:
            return ProcessInitializingState();
        case TState::Filtration:
            return ProcessFiltrationState();
        case TState::Updating:
            return ProcessUpdatingState();
        case TState::DeletedFiltration:
            return ProcessDeletedFiltrationState();
        case TState::AddedFiltration:
            return ProcessAddedFiltrationState();
        default:
            assert(false);
            return false;
        }
    }

    TCompletedResult GetResult() const
    {
        [[maybe_unused]] auto state = State.GetState();

        assert(state == TState::Ok
            || state == TState::PendingResult
            || state == TState::FinalPendingResult);
        return mCompletedResult;
    }

    ISubscriptionActor::TDeletedIds GetDeletedIds() const
    {
        [[maybe_unused]] auto state = State.GetState();

        assert(state == TState::Ok
               || state == TState::PendingResult
               || state == TState::FinalPendingResult);
        return mCompletedDeletedIds;
    }

    ISubscriptionActor::ResultState GetResultState() const
    {
        return mResultState;
    }

private:

    bool ProcessInitializingState()
    {
        return InitInternal(TEvent::Initialized);
    }

    bool ProcessFiltrationState()
    {
        return ProcessFiltrationStateInternal(
            TEvent::FiltrationCompleted,
            TEvent::FiltrationCompleted,
            std::nullopt,
            true);
    }

    bool ProcessUpdatingState()
    {
        mAddedIds.clear();
        mNeedFinalPack = false;
        return InitInternal(TEvent::RawDataUpdated);
    }

    bool ProcessDeletedFiltrationState()
    {
        return ProcessFiltrationStateInternal(
            TEvent::DeletedFiltrationCompleted,
            TEvent::DeletedFiltrationCompletedWithResult,
            Model::ActionType::Delete,
            true);
    }

    bool ProcessAddedFiltrationState()
    {
        return ProcessFiltrationStateInternal(
            TEvent::AddedFiltrationCompleted,
            TEvent::AddedFiltrationCompletedWithResult,
            Model::ActionType::New,
            false);
    }

    bool InitRanges(std::optional<Model::ActionType> aAction)
    {
        if (Ranges.Init(IRangesInit { mRawData, mSubscription.FilterExpression, aAction, mVersion }))
        {
            return true;
        }
        State.ReportError("Cannot init ranges");
        return false;
    }

    bool CheckRanges(std::optional<Model::ActionType> aAction)
    {
        if (!Ranges.IsInitialized())
        {
            if (!InitRanges(aAction))
            {
                return false;
            }
        }
        return true;
    }

    void InitFilterman()
    {
        if (!Filterman.IsInitialized())
        {
            Filterman.Init(TFiltermanInit
            {
                mSubscription.FilterExpression,
                mProcessedResult,
                Ranges.Get()
            });
        }
    }

    void FillCompletedAddedIds()
    {
        if (mCompletedResult.HasValue())
        {
            for (const auto& item : *mCompletedResult)
            {
                mAddedIds.insert(item->GetId());
            }
        }
    }

    void TryFillCompletedIntermediate()
    {
        if (mProcessedResult.size())
        {
            auto tmpResult = Basis::MakeShared<TProcessingResult>();
            tmpResult->swap(mProcessedResult);
            mCompletedResult = std::move(tmpResult);
        }
    }

    void TryFillCompletedDeletedIdsIntermediate()
    {
        if (mProcessedDeletedIds.size())
        {
            mCompletedDeletedIds.clear();
            mCompletedDeletedIds.swap(mProcessedDeletedIds);
        }
    }

    void FillProcessedDeletedIds()
    {
        for (const auto& item : mProcessedResult)
        {
            const auto id = item->GetId();
            if (!mAddedIds.count(id))
            {
                mProcessedDeletedIds.push_back(id);
            }
        }
        mProcessedResult.clear();
    }

    void TryFillIntermediateResult(std::optional<Model::ActionType> aAction)
    {
        if (aAction == Model::ActionType::Delete)
        {
            FillProcessedDeletedIds();
            TryFillCompletedDeletedIdsIntermediate();
        }
        else
        {
            TryFillCompletedIntermediate();
            if (aAction == Model::ActionType::New)
            {
                FillCompletedAddedIds();
            }
        }

        if (mCompletedResult.HasValue() || mCompletedDeletedIds.size())
        {
            mResultState = ISubscriptionActor::ResultState::IntermediateResult;
            mNeedFinalPack = true;
        }
    }

    void TryFillFinalResult(std::optional<Model::ActionType> aAction)
    {
        if (!aAction
            || mNeedFinalPack
            || (mResultState == ISubscriptionActor::ResultState::IntermediateResult))
        {
            mResultState = ISubscriptionActor::ResultState::FinalResult;
        }
    }

    bool ProcessFiltrationStateInternal(
        TEvent aCompletedEvent,
        TEvent aCompletedEventWithResult,
        std::optional<Model::ActionType> aAction,
        bool aMayCompleteResult)
    {
        mResultState = ISubscriptionActor::ResultState::NoResult;
        mCompletedResult = nullptr;
        mCompletedDeletedIds.clear();

        if (!CheckRanges(aAction))
        {
            return false;
        }

        InitFilterman();

        auto state = Filterman.Process();
        switch (state)
        {
        case TableFiltermanState::Completed:
            TryFillIntermediateResult(aAction);
            if (aMayCompleteResult)
            {
                TryFillFinalResult(aAction);
                mTracer.Trace("Final result");
            }

            Filterman.Reset();
            Ranges.Reset();

            State.ChangeState((mResultState == ISubscriptionActor::ResultState::NoResult)
                ? aCompletedEvent
                : aCompletedEventWithResult);

            mTracer.Trace("Filtration completed");

            break;
        case TableFiltermanState::Processing:
            TryFillIntermediateResult(aAction);
            if (mResultState != ISubscriptionActor::ResultState::NoResult)
            {
                mTracer.Trace("Pending");
                State.ChangeState(TEvent::PendingResultCompleted);
            }
            break;
        case TableFiltermanState::Error:
            State.ReportError("Filtration failed");
            return false;
        default:
            break;
        }

        mTracer.TraceSlow("Filtration: added: ", mCompletedResult.HasValue() ? mCompletedResult->size() : 0,
            ", deleted: ", mCompletedDeletedIds.size());

        return true;
    }

    bool InitInternal(TEvent aEvent)
    {
        mProcessedResult.clear();
        mCompletedResult = TCompletedResult {};

        State.ChangeState(aEvent);
        mTracer.Info("InitInternal: initialized");
        return true;
    }

};

}
