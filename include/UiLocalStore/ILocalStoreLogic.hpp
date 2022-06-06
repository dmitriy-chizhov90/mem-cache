#pragma once

#include <NewUiServer/UiSession.hpp>
#include <NewUiServer/UiLocalStore/ISubscriptionsContainer.hpp>
#include <NewUiServer/UiLocalStore/ITableProcessorApi.hpp>

#include <Common/Collections.hpp>
#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>
#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

template <typename TDataItem>
struct ILocalStoreLogic
{
    using TUiSubscription = TradingSerialization::Table::SubscribeBase;

    using TPendingUpdate = ISubscriptionsContainer::ProcessingResult<TDataItem>;

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Logic : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD_RETURN(std::optional<TableProcessorRejectType>, ProcessSubscription,
            const TUiSubscription::TId& /* aRequestId */,
            const TUiSubscription& /* aSubscription */)

        API_METHOD(ProcessUnsubscription,
            const TUiSubscription::TId& /* aRequestId */)

        API_METHOD_RETURN(Basis::Vector<TUiSubscription::TId>, Clear)
        API_METHOD_RETURN(Basis::Vector<TUiSubscription::TId>, GetRejectedSubscriptions)

        API_METHOD_TEMPLATE_TYPE(
            TPack,
            ProcessDataUpdate,
            const Basis::SPtr<TPack>& /* aPack */)

        CONST_API_METHOD_RETURN(bool, IsRecallNeeded)
        CONST_API_METHOD_RETURN(bool, IsReady)
        
        API_METHOD_RETURN(TPendingUpdate, ProcessDefferedTasks)
        API_METHOD_RETURN(TPendingUpdate, ProcessGetNext, const TUiSubscription::TId& /* aRequestId */)
    };
};

}
