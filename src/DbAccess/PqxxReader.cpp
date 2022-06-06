#include "Basis/DbAccess/PqxxReader.hpp"

namespace NTPro::Ecn::DbAccess
{
std::string PqxxReader::GetError() const
{
    return mError;
}

bool PqxxReader::IsValid() const
{
    return mError.empty();
}
    
bool PqxxReader::PerformQuery(
    const std::string& aConnectionStr,
    const std::string& aSql)
{
    try
    {
        mConnection = std::make_unique<pqxx::connection>(aConnectionStr);
        
        mTransaction = std::make_unique<TTransaction>(*mConnection);

        mQuery = std::make_unique<pqxx::stream_from>(
            *mTransaction,
            pqxx::from_query,
            aSql);
    }
    catch (std::exception const &e)
    {
        mError = e.what();
        return false;
    }
    return true;
}

Basis::SPtr<PqxxReader::TPack> PqxxReader::GetNextPackage()
{
    if (!mQuery)
    {
        return nullptr;
    }
        
    try
    {
        auto result = Basis::MakeShared<TPack>();
        result->reserve(PackCount);

        size_t i = 0;
        while (*mQuery && (++i <= PackCount))
        {
            auto view = mQuery->read_row();
            if (!view)
            {
                break;
            }
            TRow row;
            row.reserve(view->size());
            
            for (const auto& field : *view)
            {
                if (field.c_str())
                {
                    row.push_back(std::string { field.c_str() });
                }
                else
                {
                    row.push_back(std::nullopt);
                }
            }
            result->push_back(row);
        }
        
        if (result->empty())
        {
            mQuery->complete();
            mTransaction->commit();
            return nullptr;
        }
            
        return result;
    }
    catch (std::exception const &e)
    {
        mError = e.what();
        return nullptr;
    }
}
}
