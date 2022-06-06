#pragma once

#include <NewUiServer/UiSession.hpp>
#include <NewUiServer/UiLocalStore/ITableProcessorApi.hpp>
#include <NewUiServer/UiLocalStore/ISubscriptionActor.hpp>

#include "UiLocalStore/VersionedDataContainer.hpp"

#include <Common/Collections.hpp>
#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>
#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

struct ISubscriptionsContainer
{
    using TSubscriptionId = TradingSerialization::Table::SubscribeBase::TId;
    template <typename TData>
    struct ProcessingResult
    {
        bool Processed = false;
        ISubscriptionActor::ResultState ResultState = ISubscriptionActor::ResultState::NoResult;
        std::optional<TSubscriptionId> SubscriptionId;
        Basis::SPtrPack<TData> Result;
        Basis::Vector<int64_t> DeletedIds;

        bool IsOk() const
        {
            return SubscriptionId
                && ResultState != ISubscriptionActor::ResultState::NoResult;
        }

        bool HasNext() const
        {
            return ResultState == ISubscriptionActor::ResultState::IntermediateResult;
        }
    };

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Logic : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD_TEMPLATE_TYPE_RETURN(TSubscriptionPtr, bool, EmplaceSubscription,
            const TSubscriptionPtr& /* aSubscription */)

        API_METHOD(EraseSubscription,
            const TSubscriptionId& /* aRequestId */)

        API_METHOD_RETURN(Basis::Vector<TSubscriptionId>, Clear)
        API_METHOD_RETURN(Basis::Vector<TSubscriptionId>, GetRejectedSubscriptions)


        API_METHOD_RETURN(bool, UpdateSubscriptions, TDataVersion /*aCurrentVersion*/)
        API_METHOD_TEMPLATE_SPEC_TYPE_RETURN(
            TData,
            ProcessingResult<TData>,
            ProcessNextSubscription,
            TDataVersion /*aCurrentVersion*/)
        API_METHOD_TEMPLATE_SPEC_TYPE_RETURN(
            TData,
            ProcessingResult<TData>,
            ProcessGetNext,
            const TSubscriptionId& /* aRequestId */,
            TDataVersion /*aCurrentVersion*/)

        CONST_API_METHOD_RETURN(std::optional<TDataVersion>, GetOldestVersion)
    };
};

}
