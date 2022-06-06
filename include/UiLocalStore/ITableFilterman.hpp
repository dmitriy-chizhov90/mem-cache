#pragma once

#include <NewUiServer/UiLocalStore/IFairOperation.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Табличная фильтрация.
 * \ingroup NewUiServer
 */

enum class TableFiltermanState
{
    Initializing,
    Processing,
    Error,
    Completed
};

using ITableFilterman = IFairOperation<TableFiltermanState>;

inline std::ostream& operator<<(std::ostream& out, TableFiltermanState value)
{
    switch (value)
    {
    case TableFiltermanState::Initializing:
        return out << "Processing";
    case TableFiltermanState::Processing:
        return out << "Processing";
    case TableFiltermanState::Completed:
        return out << "Completed";
    case TableFiltermanState::Error:
        return out << "Error";
    default:
        return out << "???";
    }
}

}
