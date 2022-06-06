#pragma once

#include <Common/Collections.hpp>
#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>
#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Обертка для вызовов Query Api.
 * \ingroup NewUiServer
 * Абстрагирует работу с Query Api разных типов.
 */
struct IQueryApiWrapper
{

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Logic : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD(StartSession)
        CONST_API_METHOD_RETURN(bool, IsSessionStarted)
        CONST_API_METHOD_RETURN(bool, IsSessionConnected)
        API_METHOD(StopSession)

        API_METHOD(Subscribe)
        API_METHOD(Unsubscribe)
    };

    template<typename TImpl, typename TOwnership = Basis::Bind>
    struct Handler : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD_TEMPLATE_TYPE(
            TPack,
            ProcessDataUpdate,
            const Basis::SPtr<TPack>& /*aPack*/)

        API_METHOD(ProcessDataSubscriptionReject)

        API_METHOD(ProcessQueryApiConnected)
        API_METHOD(ProcessQueryApiDisconnected)
    };
};

}
