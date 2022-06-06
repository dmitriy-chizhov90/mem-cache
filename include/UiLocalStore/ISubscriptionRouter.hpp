#pragma once

#include <NewUiServer/UiSession.hpp>
#include <NewUiServer/UiLocalStore/ISubscriptionsContainer.hpp>
#include <NewUiServer/UiLocalStore/ITableProcessorApi.hpp>

#include <Basis/DbAccess/PqxxReader.hpp>

#include <Common/Collections.hpp>
#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>
#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

template <typename TDataPack_>
struct ISubscriptionRouter
{
    using TUiSubscription = TradingSerialization::Table::SubscribeBase;
    using TDataPack = TDataPack_;
    using TDataSPtrPack = Basis::SPtr<TDataPack>;

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Logic : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD_RETURN(bool, IsDbQuery,
            const TUiSubscription& /* aSubscription */)

        CONST_API_METHOD_RETURN(std::optional<TUiSubscription>, MakeDbQuery,
            const TUiSubscription& /* aSubscription */)

        CONST_API_METHOD_RETURN(TDataSPtrPack, MakePack,
            const Basis::SPtr<DbAccess::PqxxReader::TPack>& /* aPack */)
    };
};

}
