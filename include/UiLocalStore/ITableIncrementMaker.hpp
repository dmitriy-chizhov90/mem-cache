#pragma once

#include <NewUiServer/UiLocalStore/IFairOperation.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Расчет инкремента.
 * \ingroup NewUiServer
 */

enum class TableIncrementMakerState
{
    Initializing,
    DeletedFiltration,
    AddedFiltration,
    DeletedSorting,
    AddedSorting,
    Error,
    Completed
};

using ITableIncrementMaker = IFairOperation<TableIncrementMakerState>;

inline std::ostream& operator<<(std::ostream& out, TableIncrementMakerState value)
{
    switch (value)
    {
    case TableIncrementMakerState::Initializing:
        return out << "Initializing";
    case TableIncrementMakerState::DeletedFiltration:
        return out << "DeletedFiltration";
    case TableIncrementMakerState::AddedFiltration:
        return out << "AddedFiltration";
    case TableIncrementMakerState::DeletedSorting:
        return out << "DeletedSorting";
    case TableIncrementMakerState::AddedSorting:
        return out << "AddedSorting";
    case TableIncrementMakerState::Completed:
        return out << "Completed";
    case TableIncrementMakerState::Error:
        return out << "Error";
    default:
        return out << "???";
    }
}

}
