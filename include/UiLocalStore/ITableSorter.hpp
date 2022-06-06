#pragma once

#include <NewUiServer/UiLocalStore/IFairOperation.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Табличная сортировка.
 * \ingroup NewUiServer
 * Сортировка осуществляется в два этапа:
 * - на первом этапе весь массив данных разбивается на части фиксированного размера.
 * Каждая часть сортируется отдельно.
 * - на втором этапе выполняется последовательная сортировка слиянием всех частей массива.
 */

enum class TableSorterState
{
    Initializing,
    PartSort,
    MergeSort,
    Completed,
    Error
};

using ITableSorter = IFairOperation<TableSorterState>;

inline std::ostream& operator<<(std::ostream& out, TableSorterState value)
{
    switch (value)
    {
    case TableSorterState::Initializing:
        return out << "Initializing";
    case TableSorterState::PartSort:
        return out << "PartSort";
    case TableSorterState::MergeSort:
        return out << "MergeSort";
    case TableSorterState::Completed:
        return out << "Completed";
    case TableSorterState::Error:
        return out << "Error";
    default:
        return out << "???";
    }
}

}
