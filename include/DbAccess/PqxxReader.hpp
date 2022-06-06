#pragma once

#include "Common/Collections.hpp"
#include "Common/SPtr.hpp"

#include <pqxx/pqxx>

namespace NTPro::Ecn::DbAccess
{

/// Выполняет запрос в БД в рамках отдельного соединения.
/// Возвращает полученные записи пакетами определенного размера.
class PqxxReader
{
public:
    static constexpr size_t PackCount = 100;

    using TField = std::optional<std::string>;
    using TRow = Basis::Vector<TField>;
    using TPack = Basis::Vector<TRow>;
    
private:
    using TTransaction = pqxx::transaction<
        pqxx::isolation_level::read_committed,
        pqxx::write_policy::read_only>;

    std::unique_ptr<pqxx::connection> mConnection;
    std::unique_ptr<TTransaction> mTransaction;
    std::unique_ptr<pqxx::stream_from> mQuery;

    std::string mError;

public:
    std::string GetError() const;
    bool IsValid() const;
    
    bool PerformQuery(
        const std::string& aConnectionStr,
        const std::string& aSql);

    Basis::SPtr<TPack> GetNextPackage();
};
    
}
