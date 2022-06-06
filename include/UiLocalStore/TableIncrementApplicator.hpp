#pragma once

#include <NewUiServer/UiLocalStore/ITableIncrementApplicator.hpp>
#include <NewUiServer/UiLocalStore/TableUtils.hpp>
#include <NewUiServer/UiLocalStore/LocalStoreUtils.hpp>
#include <NewUiServer/UiLocalStore/SortOrderComparator.hpp>

#include "TradingSerialization/Table/Columns.hpp"

#include <Common/Tracer.hpp>

#include <Common/Pack.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Применение инкремента.
 * \ingroup NewUiServer
 */
template <typename TSetup>
class TableIncrementApplicator
{
public:
    static constexpr int MaxCount = TSetup::MaxCount;

    using TData = typename TSetup::TData;
    using TTableSetup = typename TSetup::TTableSetup;

    using TTableColumnType = typename TTableSetup::TTableColumnType;
    using TDataPack = Basis::Pack<TData>;
    using TDataSPtrPack = Basis::SPtr<TDataPack>;
    using TIt = typename TDataPack::const_iterator;

    using TSortOrder = TradingSerialization::Table::TSortOrder;

    using TComparator = SortOrderComparator<TData, TTableColumnType>;

    struct TInit
    {
        const TSortOrder& SortOrder;
        const TDataSPtrPack& OldSnapshot;
        const TDataPack& DeletedIncrement;
        const TDataPack& AddedIncrement;
        TDataPack& NewSnapshot;

        TInit(
            const TSortOrder& aSortOrder,
            const TDataSPtrPack& aOldSnapshot,
            const TDataPack& aDeletedIncrement,
            const TDataPack& aAddedIncrement,
            TDataPack& outNewSnapshot)
            : SortOrder(aSortOrder)
            , OldSnapshot(aOldSnapshot)
            , DeletedIncrement(aDeletedIncrement)
            , AddedIncrement(aAddedIncrement)
            , NewSnapshot(outNewSnapshot)
        {}
    };

private:

    struct MergePosition
    {
        TIt OldIt;
        TIt DeletedIt;
        TIt AddedIt;
    };

    const TSortOrder* mSortOrder = nullptr;
    const TDataSPtrPack* mOldSnapshot = nullptr;
    const TDataPack* mDeletedIncrement = nullptr;
    const TDataPack* mAddedIncrement = nullptr;

    TDataPack* mNewSnapshot = nullptr;

    Basis::Tracer& mTracer;

    TableIncrementApplicatorState mState = TableIncrementApplicatorState::Initializing;
    bool mOk = true;
    std::optional<TComparator> mComparator;
    MergePosition mPosition;

public:
    TableIncrementApplicator(
        Basis::Tracer& aTracer)
        : mTracer(aTracer)
    {}

    void Init(const TInit& aInit)
    {
        assert(!IsInitialized());

        mSortOrder = &aInit.SortOrder;
        mOldSnapshot = &aInit.OldSnapshot;
        mDeletedIncrement = &aInit.DeletedIncrement;
        mAddedIncrement = &aInit.AddedIncrement;
        mNewSnapshot = &aInit.NewSnapshot;

        mPosition.OldIt = (*mOldSnapshot)->cbegin();
        mPosition.DeletedIt = mDeletedIncrement->cbegin();
        mPosition.AddedIt = mAddedIncrement->cbegin();

        mComparator.emplace(*mSortOrder, mOk);

        /// Валидация входных параметров (порядка сортировки)
        auto sortOrderError = TableUtils<TTableSetup>::CheckSortOrder(*mSortOrder);
        if (!sortOrderError.empty())
        {
            mState = TableIncrementApplicatorState::Error;
            mTracer.ErrorSlow("TableIncrementApplicator:", sortOrderError, ", ", *mSortOrder);
            return;
        }

        mState = TableIncrementApplicatorState::Processing;
    }

    void Reset()
    {
        mSortOrder = nullptr;
        mOldSnapshot = nullptr;
        mDeletedIncrement = nullptr;
        mAddedIncrement = nullptr;
        mNewSnapshot = nullptr;
        mComparator = std::nullopt;

        mState = TableIncrementApplicatorState::Initializing;
    }

    bool IsInitialized() const
    {
        return mState != TableIncrementApplicatorState::Initializing;
    }

    TableIncrementApplicatorState GetState() const
    {
        return mState;
    }

    TableIncrementApplicatorState Process()
    {
        switch(mState)
        {
        case TableIncrementApplicatorState::Processing:
            ProcessInternal();
            break;
        default:
            mTracer.ErrorSlow("IncrementApplicator.Process: state is invalid:", mState);
            mState = TableIncrementApplicatorState::Error;
            break;
        }

        return mState;
    }

private:
    bool TrySetCompleted()
    {
        if (!IsOldIt() && !IsAddedIt() && !IsDeletedIt())
        {
            mState = TableIncrementApplicatorState::Completed;
            return true;
        }
        return false;
    }

    bool TrySetError()
    {
        if (!mOk)
        {
            mState = TableIncrementApplicatorState::Error;
            mTracer.Error("IncrementApplicator. Error occured");
            return true;
        }
        return false;
    }

    bool IsOldIt() const
    {
        return mPosition.OldIt != (*mOldSnapshot)->cend();
    }
    bool IsAddedIt() const
    {
        return mPosition.AddedIt != mAddedIncrement->cend();
    }
    bool IsDeletedIt() const
    {
        return mPosition.DeletedIt != mDeletedIncrement->cend();
    }

    void ProcessInternal()
    {
        if (TrySetCompleted())
        {
            return;
        }

        int counter = 0;

        while ((++counter <= MaxCount) && !TrySetCompleted())
        {
            if (IsAddedIt())
            {
                if (!IsOldIt() || (*mComparator)(*mPosition.AddedIt, *mPosition.OldIt))
                {
                    if (TrySetError())
                    {
                        return;
                    }
                    /// Новый элемент
                    mNewSnapshot->push_back(*mPosition.AddedIt);
                    ++mPosition.AddedIt;
                    continue;
                }
                if (!(*mComparator)(*mPosition.OldIt, *mPosition.AddedIt))
                {
                    if (TrySetError())
                    {
                        return;
                    }
                    /// Элементы равны, берем более новый
                    mNewSnapshot->push_back(*mPosition.AddedIt);
                    ++mPosition.AddedIt;
                    ++mPosition.OldIt;
                    continue;
                }
                else
                {
                    /// Старый элемент, здесь ничего не делаем,
                    /// потому что нужно ещё проверить, не удален ли он. Это делается ниже.
                }
            }
            if (IsDeletedIt())
            {
                if (!IsOldIt() || (*mComparator)(*mPosition.DeletedIt, *mPosition.OldIt))
                {
                    if (TrySetError())
                    {
                        return;
                    }
                    /// Такое может быть, если выше вставили новый элемент вместо старого
                    /// или если удаляем то, чего и раньше не было.
                    ++mPosition.DeletedIt;
                    continue;
                }
                if (!(*mComparator)(*mPosition.OldIt, *mPosition.DeletedIt))
                {
                    if (TrySetError())
                    {
                        return;
                    }
                    /// Элемент удален
                    ++mPosition.DeletedIt;
                    ++mPosition.OldIt;
                    continue;
                }
            }
            /// Нет нового и старый не удален
            mNewSnapshot->push_back(*mPosition.OldIt);
            ++mPosition.OldIt;
        }
    }
};

}
