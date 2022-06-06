#pragma once

#include <NewUiServer/UiSession.hpp>
#include <NewUiServer/UiLocalStore/ITableProcessorApi.hpp>

#include "UiLocalStore/VersionedDataContainer.hpp"

#include <Common/Collections.hpp>
#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>
#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

struct ISubscriptionActor
{
    using TDeletedIds = Basis::Vector<int64_t>;

    enum class ResultState
    {
        NoResult,
        FinalResult,
        IntermediateResult,
    };

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Logic : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD_RETURN(bool, Process)
        API_METHOD(IncreaseVersion)
        API_METHOD_RETURN(bool, ProcessGetNext)
    };
};

}
