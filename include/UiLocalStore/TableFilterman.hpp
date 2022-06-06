#pragma once

#include <NewUiServer/UiLocalStore/ITableFilterman.hpp>
#include <NewUiServer/UiLocalStore/IDataRanges.hpp>
#include <NewUiServer/UiLocalStore/TableUtils.hpp>
#include <NewUiServer/UiLocalStore/LocalStoreUtils.hpp>

#include "TradingSerialization/Table/Columns.hpp"

#include <Common/Tracer.hpp>

#include <Common/Pack.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Фильтрация данных таблицы.
 * \ingroup NewUiServer
 */
template <typename TSetup, typename TDataRanges>
class TableFilterman
{
public:   
    static constexpr int MaxCount = TSetup::MaxCount;

    using TData = typename TSetup::TData;
    using TTableSetup = typename TSetup::TTableSetup;

    using TUiFilters = TradingSerialization::Table::FilterGroup;
    using TDataPack = Basis::Pack<TData>;

    struct TInit
    {
        const TUiFilters& Filters;
        TDataPack& Result;
        TDataRanges& IndexRanges;

        TInit(
            const TUiFilters& aFilters,
            TDataPack& outResult,
            TDataRanges& aIndexRanges)
            : Filters(aFilters)
            , Result(outResult)
            , IndexRanges(aIndexRanges)
        {}
    };

    using IRanges = typename IDataRanges<TData>::template Ranges<TDataRanges, Basis::Bind>;

    IRanges Ranges;

private:
    const TUiFilters* mFilters = nullptr;
    TDataPack* mResult = nullptr;
    Basis::Tracer& mTracer;
    TableFiltermanState mState = TableFiltermanState::Initializing;

public:
    TableFilterman(Basis::Tracer& aTracer)
        : mTracer(aTracer)
    {}

    bool IsInitialized() const
    {
        return mState != TableFiltermanState::Initializing;
    }

    void Init(const TInit& aInit)
    {
        assert(!IsInitialized());

        Ranges.Bind(aInit.IndexRanges);
        mFilters = &aInit.Filters;
        mResult = &aInit.Result;

        mState = TableFiltermanState::Processing;
    }

    void Reset()
    {
        Ranges.Bind(nullptr);
        mFilters = nullptr;
        mResult = nullptr;

        mState = TableFiltermanState::Initializing;
    }

    TableFiltermanState Process()
    {
        if (mState != TableFiltermanState::Processing)
        {
            assert(false);
            mTracer.ErrorSlow("Filterman.Process: state is invalid:", mState);
            mState = TableFiltermanState::Error;
            return mState;
        }

        int counter = 0;
        while (++counter <= MaxCount)
        {
            auto dataPtr = Ranges.GetNext();
            if (!dataPtr.HasValue())
            {
                mState = TableFiltermanState::Completed;
                return mState;
            }

            bool ok = true;
            if (LocalStoreUtils::CheckDataByFilter<TTableSetup>(*dataPtr, *mFilters, ok))
            {
                mResult->push_back(dataPtr);
            }
            if (!ok)
            {
                mTracer.ErrorSlow("Filterman.Process: check filter failed for:", dataPtr, ", filter: ", *mFilters);
                mState = TableFiltermanState::Error;
                return mState;
            }
        }

        return mState;
    }

    TableFiltermanState GetState() const
    {
        return mState;
    }
};

}
