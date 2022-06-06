#pragma once

#include <NewUiServer/UiLocalStore/IDataRanges.hpp>
#include <NewUiServer/UiLocalStore/ITableIncrementMaker.hpp>
#include <NewUiServer/UiLocalStore/ITableFilterman.hpp>
#include <NewUiServer/UiLocalStore/ITableSorter.hpp>
#include <NewUiServer/UiLocalStore/TableUtils.hpp>
#include <NewUiServer/UiLocalStore/LocalStoreUtils.hpp>
#include <NewUiServer/UiLocalStore/VersionedDataContainer.hpp>

#include "TradingSerialization/Table/Columns.hpp"

#include <Common/Tracer.hpp>

#include <Common/Pack.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Расчет инкремента.
 * \ingroup NewUiServer
 */
template <typename TSetup>
class TableIncrementMaker
{
public:
    using TData = typename TSetup::TData;

    using TDataPack = Basis::Pack<TData>;
    using TIt = typename TDataPack::const_iterator;

    using TSorterImpl = typename TSetup::TTableSorter;
    using TSorterInit = typename TSetup::TTableSorterInit;
    using ISorter = ITableSorter::Performer<TSorterImpl>;
    using TFiltermanImpl = typename TSetup::TTableIndexFilterman;
    using TFiltermanInit = typename TSetup::TTableIndexFiltermanInit;
    using IFilterman = ITableFilterman::Performer<TFiltermanImpl>;

    using TUiFilters = TradingSerialization::Table::FilterGroup;
    using TSortOrder = TradingSerialization::Table::TSortOrder;

    using IRanges = typename IDataRanges<TData>::template Ranges<typename TSetup::TIndexedDataRanges>;
    using IRangesInit = typename TSetup::TIndexedDataRangesInit;

    using TMap = typename TSetup::TMap;

    struct TInit
    {
        const TUiFilters& Filters;
        const TSortOrder& SortOrder;
        TDataPack& Deleted;
        TDataPack& Added;
        const TMap& Map;
        TDataPack& TmpBuffer;
        TDataVersion Version;

        TInit(
            const TUiFilters& aFilters,
            const TSortOrder& aSortOrder,
            TDataPack& outDeleted,
            TDataPack& outAdded,
            const TMap& aMap,
            TDataPack& aTmpBuffer,
            TDataVersion aVersion)
            : Filters(aFilters)
            , SortOrder(aSortOrder)
            , Deleted(outDeleted)
            , Added(outAdded)
            , Map(aMap)
            , TmpBuffer(aTmpBuffer)
            , Version(aVersion)
        {}
    };

private:
    const TUiFilters* mFilters = nullptr;
    const TSortOrder* mSortOrder = nullptr;

    TDataPack* mDeleted = nullptr;
    TDataPack* mAdded = nullptr;
    const TMap* mMap = nullptr;
    TDataPack* mTmpBuffer = nullptr;

    TableIncrementMakerState mState = TableIncrementMakerState::Initializing;

    Basis::Tracer& mTracer;

public:
    IRanges DeletedRange;
    IRanges AddedRange;

    IFilterman Filterman;
    ISorter Sorter;

public:

    TableIncrementMaker(Basis::Tracer& aTracer)
        : mTracer(aTracer)
        , DeletedRange(mTracer)
        , AddedRange(mTracer)
        , Filterman(mTracer)
        , Sorter(mTracer)
    {}

    void Init(const TInit& aInit)
    {
        assert(!IsInitialized());

        mFilters = &aInit.Filters;
        mSortOrder = &aInit.SortOrder;
        mDeleted = &aInit.Deleted;
        mAdded = &aInit.Added;
        mMap = &aInit.Map;
        mTmpBuffer = &aInit.TmpBuffer;

        if (!DeletedRange.Init(IRangesInit { *mMap, *mFilters, Model::ActionType::Delete, aInit.Version }))
        {
            mTracer.Error("TableIncrementMaker: error during initialization deleted increment");
            assert(false);
        }

        if (!AddedRange.Init(IRangesInit { *mMap, *mFilters, Model::ActionType::New, aInit.Version }))
        {
            mTracer.Error("TableIncrementMaker: error during initialization deleted increment");
            assert(false);
        }

        mState = TableIncrementMakerState::DeletedFiltration;
    }

    void Reset()
    {
        mFilters = nullptr;
        mSortOrder = nullptr;
        mDeleted = nullptr;
        mAdded = nullptr;
        mMap = nullptr;
        mTmpBuffer = nullptr;

        DeletedRange.Reset();
        AddedRange.Reset();

        mState = TableIncrementMakerState::Initializing;
    }

    bool IsInitialized() const
    {
        return mState != TableIncrementMakerState::Initializing;
    }

    TableIncrementMakerState GetState() const
    {
        return mState;
    }

    TableIncrementMakerState Process()
    {
        switch(mState)
        {
        case TableIncrementMakerState::DeletedFiltration:
            ProcessDeletedFiltration();
            break;
        case TableIncrementMakerState::AddedFiltration:
            ProcessAddedFiltration();
            break;
        case TableIncrementMakerState::DeletedSorting:
            ProcessDeletedSorting();
            break;
        case TableIncrementMakerState::AddedSorting:
            ProcessAddedSorting();
            break;
        default:
            mTracer.ErrorSlow("IncrementCalculator.Process: state is invalid:", mState);
            mState = TableIncrementMakerState::Error;
            break;
        }

        return mState;
    }

private:
    void ProcessFiltrationInternal(
        TDataPack* outResult,
        IRanges& aRange,
        TableIncrementMakerState aNextState)
    {
        if (!Filterman.IsInitialized())
        {
            Filterman.Init(TFiltermanInit { *mFilters, *outResult, aRange.Get() });
        }

        switch (Filterman.Process())
        {
        case TableFiltermanState::Completed:
            mState = aNextState;
            Filterman.Reset();
            break;
        case TableFiltermanState::Error:
            mState = TableIncrementMakerState::Error;
            break;
        default:
            break;
        }
    }

    void ProcessDeletedFiltration()
    {
        ProcessFiltrationInternal(mDeleted, DeletedRange, TableIncrementMakerState::AddedFiltration);
    }

    void ProcessAddedFiltration()
    {
        ProcessFiltrationInternal(mAdded, AddedRange, TableIncrementMakerState::DeletedSorting);
    }

    void ProcessSortingInternal(TDataPack* outResult, TableIncrementMakerState aTargetState)
    {
        if (!Sorter.IsInitialized())
        {
            Sorter.Init(TSorterInit { *mSortOrder, *outResult, *mTmpBuffer });
            return;
        }

        switch (Sorter.Process())
        {
        case TableSorterState::Completed:
            Sorter.Reset();
            mState = aTargetState;
            break;
        case TableSorterState::Error:
            mState = TableIncrementMakerState::Error;
            return;
        default:
            break;
        }
    }

    void ProcessDeletedSorting()
    {
        ProcessSortingInternal(mDeleted, TableIncrementMakerState::AddedSorting);
    }

    void ProcessAddedSorting()
    {
        ProcessSortingInternal(mAdded, TableIncrementMakerState::Completed);
    }
};

}
