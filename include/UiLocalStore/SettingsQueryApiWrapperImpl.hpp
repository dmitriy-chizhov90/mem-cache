#pragma once

#include <Common/Collections.hpp>
#include <Common/Pack.hpp>
#include <Common/StringUtils.hpp>

#include <Basis/EventRegistry.hpp>

#include "UiLocalStore/IQueryApiWrapper.hpp"

#include "UiServerHelpers.hpp"

namespace NTPro::Ecn::NewUiServer
{

template <typename TSetting, typename TSetup>
class SettingsQueryApiWrapperImpl
{
    Basis::Tracer& mTracer;
public:

    using TApiHandler = typename TSetup::TQueryApiWrapperHandler;
    using TQueyApi = typename TSetup::TQueryApi;
    using TQueryApiImpl = typename TSetup::TSettingsQueryApiClient;
    using TQueyApiClient = typename TQueyApi::template Client<TQueryApiImpl, Basis::Own>;

    TQueyApiClient QueryApiClient;

    IQueryApiWrapper::Handler<TApiHandler, Basis::Ref> Handler;
private:

    SubscriptionState mSubscriptionState;

public:
    SettingsQueryApiWrapperImpl(
        const Basis::ComponentId& aComponentId,
        const Basis::ComponentId& aSettingsStoreComponentId,
        Basis::EventRegistry& aEventRegistry,
        TApiHandler* aHandler,
        Basis::Tracer& aTracer)
        : mTracer(aTracer)
        , QueryApiClient((aComponentId.GetName() + "Q").c_str(), aSettingsStoreComponentId, aEventRegistry, this)
        , Handler(*aHandler)
    {
    }

    void StartSession()
    {
        QueryApiClient.StartSession();
    }

    void StopSession()
    {
        if (QueryApiClient.IsSessionStarted())
        {
            QueryApiClient.StopSession();
        }
    }

    bool IsSessionStarted() const
    {
        return QueryApiClient.IsSessionStarted();
    }

    bool IsSessionConnected() const
    {
        return QueryApiClient.IsSessionConnected();
    }

    /// ----------------------------------------------------------------------------------------------------------------
    /// IQueryApi
    /// ----------------------------------------------------------------------------------------------------------------

    void ProcessSettingsUpdate(const typename TQueyApi::SettingPack& aPack)
    {
        mTracer.DebugSlow("Process settings, pack size:", aPack.Data->size());
        Handler.ProcessDataUpdate(aPack.Data);
    }

    void ProcessSettingsSubscriptionReject(const typename TQueyApi::SubscriptionReject&)
    {
        mTracer.Warning("Settings subscription reject!");
        mSubscriptionState.IsSubscribed = false;

        Handler.ProcessDataSubscriptionReject();
    }

    void ProcessSettingsQueryApiSessionConnected(const TSetting& /*aPriceLevelSettings*/)
    {
        mTracer.Debug("Settings query session connected");

        Handler.ProcessQueryApiConnected();

        mSubscriptionState.SessionConnected = true;

        ProcessApiSubscriptions();
    }

    void ProcessSettingsQueryApiSessionDisconnected(const TSetting& /*aPriceLevelSettings*/)
    {
        mTracer.Debug("Settings query session disconnected");

        mSubscriptionState.SessionConnected = false;
        mSubscriptionState.IsSubscribed = false;

        Handler.ProcessQueryApiDisconnected();
    }

private:

    void ProcessApiSubscriptions()
    {
        if (NeedSubscribe(mSubscriptionState))
        {
            mTracer.Info("Subscribe to SettingsQueryApi");

            mSubscriptionState.SubscribeKey = QueryApiClient.SubscribeSettings(NTPro::Ecn::Service::SettingsQuery<TSetting> {});

            mSubscriptionState.IsSubscribed = true;
        }
    }

    void ProcessApiUnsubscriptions()
    {
        if (NeedUnsubscribe(mSubscriptionState))
        {
            mTracer.Info("Unsubscribe from Query API");

            QueryApiClient.UnsubscribeSettings(mSubscriptionState.SubscribeKey);

            mSubscriptionState.IsSubscribed = false;
        }
    }
};

}
