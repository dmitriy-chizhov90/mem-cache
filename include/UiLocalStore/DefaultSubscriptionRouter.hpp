#pragma once

#include "NewUiServer/UiLocalStore/ISubscriptionRouter.hpp"

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Дефолтный роутер для in-memory данных.
 * \ingroup NewUiServer
 */
template <typename TDataPack>
class DefaultSubscriptionRouter
{
    using TUiSubscription = typename ISubscriptionRouter<TDataPack>::TUiSubscription;
    
public:
    
    template <typename ...TArgs>
    DefaultSubscriptionRouter(TArgs&& ...)
    {}
    
    bool IsDbQuery(const TUiSubscription&)
    {
        return false;
    }

    std::optional<TUiSubscription> MakeDbQuery(const TUiSubscription&) const
    {
        return std::nullopt;
    }

    Basis::SPtr<TDataPack> MakePack(const Basis::SPtr<DbAccess::PqxxReader::TPack>&) const
    {
        return nullptr;
    }
};
}
