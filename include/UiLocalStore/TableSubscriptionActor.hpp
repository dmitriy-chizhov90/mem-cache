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
class TableSubscriptionActor
{
public:
    using TData = typename TSetup::TData;
    using TDataPack = typename ITableProcessorApi<Basis::Pack<TData>>::TDataPack;
    using TMap = typename TSetup::TMap;

    using TUiSubscription = TradingSerialization::Table::SubscribeBase;
    using TProcessingResult = std::shared_ptr<Basis::Pack<TData>>;
    using TCompletedResult = Basis::SPtrPack<TData>;

    using TUiFilters = TradingSerialization::Table::FilterGroup;
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TUiFilterValues = TradingSerialization::Table::TFilterValues;
    using TIntFilter = TradingSerialization::Table::TInt;
    using TFilterOperator = TradingSerialization::Table::FilterOperator;

    using TState = ISubscriptionStateMachine::TableState;
    using TEvent = ISubscriptionStateMachine::TableEvent;

    using TFiltermanImpl = typename TSetup::TTableIndexFilterman;
    using TFiltermanInit = typename TSetup::TTableIndexFiltermanInit;
    using IFilterman = ITableFilterman::Performer<TFiltermanImpl>;
    using TSorterImpl = typename TSetup::TTableSorter;
    using TSorterInit = typename TSetup::TTableSorterInit;
    using ISorter = ITableSorter::Performer<TSorterImpl>;
    using TTableIncrementMakerImpl = typename TSetup::TTableIncrementMaker;
    using TTableIncrementMakerInit = typename TSetup::TTableIncrementMakerInit;
    using IIncrementMaker = ITableIncrementMaker::Performer<TTableIncrementMakerImpl>;
    using TTableIncrementApplicatorImpl = typename TSetup::TTableIncrementApplicator;
    using TTableIncrementApplicatorInit = typename TSetup::TTableIncrementApplicatorInit;
    using IIncrementApplicator = ITableIncrementApplicator::Performer<TTableIncrementApplicatorImpl>;

    using IRanges = typename IDataRanges<TData>::template Ranges<typename TSetup::TIndexedDataRanges>;
    using IRangesInit = typename TSetup::TIndexedDataRangesInit;
    using IState = ISubscriptionStateMachine::Machine<TState, TEvent, typename TSetup::TSubscriptionStateMachine>;

private:

    Basis::Tracer& mTracer;

    TUiSubscription::TId mRequestId;
    TUiSubscription mSubscription;

    const TMap& mRawData;

    mutable TProcessingResult mProcessedResult;
    /// For sorting
    TDataPack mTmpBuffer;

    mutable TCompletedResult mCompletedResult;

    TDataVersion mVersion = 0;

    TDataPack mDeletedIncrement;
    TDataPack mAddedIncrement;

public:
    IState State;

    IRanges Ranges;
    IFilterman Filterman;
    ISorter Sorter;
    IIncrementMaker IncrementMaker;
    IIncrementApplicator IncrementApplicator;

public:
    TableSubscriptionActor(
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
        , Sorter(mTracer)
        , IncrementMaker(mTracer)
        , IncrementApplicator(mTracer)

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
        assert(false);
        return false;
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
        case TState::Sorting:
            return ProcessSortingState();
        case TState::Updating:
            return ProcessUpdatingState();
        case TState::IncrementMaking:
            return ProcessIncrementMakingState();
        case TState::IncrementApplying:
            return ProcessIncrementApplyingState();
        default:
            assert(false);
            return false;
        }
    }

    TCompletedResult GetResult() const
    {
        [[maybe_unused]] auto state = State.GetState();
        assert(state == TState::Ok);
        return mCompletedResult;
    }

    ISubscriptionActor::TDeletedIds GetDeletedIds() const
    {
        return ISubscriptionActor::TDeletedIds {};
    }

    ISubscriptionActor::ResultState GetResultState() const
    {
        return ISubscriptionActor::ResultState::FinalResult;
    }

private:

    bool ProcessInitializingState()
    {
        mProcessedResult = Basis::MakeShared<TDataPack>();
        /// TODO: нельзя отдавать управление в реактор после инициализации, т.к. в первый момент времени
        /// итераторы могут указывать на данные предыдущих версий, которые могут быть удалены.
        /// При фильтрации мы отдаем управление, остановившись на данных своей версии, которые не будут удаляться.
        ///
        if (Ranges.Init(IRangesInit { mRawData, mSubscription.FilterExpression, std::nullopt, mVersion }))
        {
            State.ChangeState(TEvent::Initialized);
            mTracer.Info("ProcessInitializingState: initialized");
            return true;
        }
        State.ReportError("Cannot init ranges");
        return false;
    }

    bool ProcessFiltrationState()
    {
        if (!Filterman.IsInitialized())
        {
            Filterman.Init(TFiltermanInit
            {
                mSubscription.FilterExpression,
                *mProcessedResult,
                Ranges.Get()
            });
        }

        switch (Filterman.Process())
        {
        case TableFiltermanState::Completed:
            State.ChangeState(TEvent::FiltrationCompleted);
            mTracer.InfoSlow("ProcessFiltrationState: filtration completed:", mProcessedResult->size());
            Filterman.Reset();
            break;
        case TableFiltermanState::Error:
            State.ReportError("Filtration failed");
            return false;
        default:
            break;
        }

        return true;
    }

    bool ProcessSortingState()
    {
        if (!Sorter.IsInitialized())
        {
            Sorter.Init(TSorterInit
            {
                mSubscription.SortOrder,
                *mProcessedResult,
                mTmpBuffer
            });
            return true;
        }

        switch (Sorter.Process())
        {
        case TableSorterState::Completed:
            Sorter.Reset();
            /// TODO: clear sort buffer
            State.ChangeState(TEvent::SortingCompleted);
            mCompletedResult = mProcessedResult;
            Ranges.Reset();
            mTracer.InfoSlow("ProcessSortingState: completed. mCompletedResult.size:", mCompletedResult->size());
            break;
        case TableSorterState::Error:
            State.ReportError("ProcessSortingState: cannot process sorting");
            return false;
        default:
            break;
        }

        return true;
    }

    bool ProcessUpdatingState()
    {
        mProcessedResult = Basis::MakeShared<TDataPack>();
        State.ChangeState(TEvent::RawDataUpdated);
        return true;
    }

    bool ProcessIncrementMakingState()
    {
        if (!IncrementMaker.IsInitialized())
        {
            /// TODO: сделать плавную очистку
            mDeletedIncrement.clear();
            mAddedIncrement.clear();

            IncrementMaker.Init(TTableIncrementMakerInit
            {
                mSubscription.FilterExpression,
                mSubscription.SortOrder,
                mDeletedIncrement,
                mAddedIncrement,
                mRawData,
                mTmpBuffer,
                mVersion
            });
        }

        switch (IncrementMaker.Process())
        {
        case TableIncrementMakerState::Completed:
            IncrementMaker.Reset();
            State.ChangeState(TEvent::IncrementMade);
            break;
        case TableIncrementMakerState::Error:
            State.ReportError("Increment making failed");
            return false;
        default:
            break;

        }
        return true;
    }

    bool ProcessIncrementApplyingState()
    {
        if (!IncrementApplicator.IsInitialized())
        {
            IncrementApplicator.Init(TTableIncrementApplicatorInit
            {
                mSubscription.SortOrder,
                mCompletedResult,
                mDeletedIncrement,
                mAddedIncrement,
                *mProcessedResult
            });
        }

        switch (IncrementApplicator.Process())
        {
        case TableIncrementApplicatorState::Completed:
            IncrementApplicator.Reset();
            State.ChangeState(TEvent::IncrementApplied);
            break;
        case TableIncrementApplicatorState::Error:
            State.ReportError("Increment applying failed");
            return false;
        default:
            break;
        }
        return true;
    }
};

}
