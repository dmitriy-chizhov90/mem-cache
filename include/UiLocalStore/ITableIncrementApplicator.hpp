#pragma once

#include <NewUiServer/UiLocalStore/IFairOperation.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Применение инкремента.
 * \ingroup NewUiServer
 */

enum class TableIncrementApplicatorState
{
    Initializing,
    Processing,
    Completed,
    Error
};

using ITableIncrementApplicator = IFairOperation<TableIncrementApplicatorState>;

inline std::ostream& operator<<(std::ostream& out, TableIncrementApplicatorState value)
{
    switch (value)
    {
    case TableIncrementApplicatorState::Initializing:
        return out << "Initializing";
    case TableIncrementApplicatorState::Processing:
        return out << "Processing";
    case TableIncrementApplicatorState::Completed:
        return out << "Completed";
    case TableIncrementApplicatorState::Error:
        return out << "Error";
    default:
        return out << "???";
    }
}

}
