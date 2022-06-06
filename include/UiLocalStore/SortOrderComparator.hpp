#pragma once

#include <NewUiServer/UiLocalStore/ITableSorter.hpp>
#include <NewUiServer/UiLocalStore/TableUtils.hpp>
#include <NewUiServer/UiLocalStore/LocalStoreUtils.hpp>

#include "TradingSerialization/Table/Columns.hpp"

#include <Common/Tracer.hpp>

#include <Common/Pack.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Функтор сравнения.
 * \ingroup NewUiServer
 * Сравнивает два табличных элемента в соответствии с порядком сортировки.
 * Флаг mOk выставляется в False в случае ошибок при выполнении сравнения.
 */
template <typename TData, typename TTableColumnType>
class SortOrderComparator
{
public:
    using TSortOrder = TradingSerialization::Table::TSortOrder;

private:
    const TSortOrder& mSortOrder;
    bool& mOk;

public:
    SortOrderComparator(const TSortOrder& aSortOrder, bool& outOk)
        : mSortOrder(aSortOrder)
        , mOk(outOk)
    {}

    bool operator ()(const Basis::SPtr<TData>& aLhs, const Basis::SPtr<TData>& aRhs) const
    {
        for (const auto sortColumn : mSortOrder)
        {
            const auto& lhs = aLhs->GetValue(static_cast<TTableColumnType>(sortColumn));
            const auto& rhs = aRhs->GetValue(static_cast<TTableColumnType>(sortColumn));
            if (LocalStoreUtils::CheckEqual(lhs, rhs, mOk))
            {
                continue;
            }
            return LocalStoreUtils::CheckLess(lhs, rhs, mOk);
        }
        return false;
    }
};

}
