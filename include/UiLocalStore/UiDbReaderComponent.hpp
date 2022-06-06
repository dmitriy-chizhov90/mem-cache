#pragma once

#include <Common/Collections.hpp>
#include <Common/Pack.hpp>
#include <Common/StringUtils.hpp>

#include <Basis/EventRegistry.hpp>

#include "TradingSerialization/Table/Columns.hpp"
#include "TradingSerialization/Table/Subscription.hpp"

#include <Basis/ITechnicalControlApi.hpp>
#include <Basis/DbAccess/PqxxReader.hpp>

#include "UiLocalStore/ITableProcessorApi.hpp"

namespace NTPro::Ecn::NewUiServer
{

/**
 * \class UiDbReaderComponent
 * \ingroup NewUiServer
 * \brief Компонент для выполнения запросов на чтение к БД.
 * Результаты отправляются пакетами по запросу с клиента через ITableProcessorApi.
 */
template <typename TSetup>
class UiDbReaderComponent
{
    using TUiSubscription = TradingSerialization::Table::SubscribeBase;

    using TData = DbAccess::PqxxReader::TRow;

    Basis::Tracer& mTracer;
public:

    Service::ITechnicalControlApi::Client<typename TSetup::TTechnicalControlApiClient, Basis::Own> TechnicalControlApiClient;

    DbAccess::PqxxReader DbReader;

    typename ITableProcessorApi<DbAccess::PqxxReader::TPack>::template Store<
        typename TSetup::TTableProcessorApiDbReader,
        Basis::Own> ClientApi;

private:
    static constexpr size_t MaxDbConnectionsCount = 50;

    struct SubscriptionInfo
    {
        DbAccess::PqxxReader DbReader;
        Basis::SPtr<DbAccess::PqxxReader::TPack> NextPackage;
    };

    Basis::Map<TUiSubscription::TId, SubscriptionInfo> mSubscriptions;
    
public:
    UiDbReaderComponent(
        const Basis::ComponentId& aComponentId,
        Basis::EventRegistry& aEventRegistry)
        : mTracer(Basis::Tracing::GetTracer(aComponentId, "DbReader"))
        , TechnicalControlApiClient(aComponentId, aEventRegistry, this)
        , ClientApi(aComponentId, aEventRegistry, this)
    {
    }

    void ProcessTechnicalStart()
    {
        mTracer.Info("ProcessTechnicalStart");
        TechnicalControlApiClient.SendState(Model::TechnicalStateType::Started);
    }

    void ProcessTechnicalStop()
    {
        mTracer.Info("ProcessTechnicalStop");
        TechnicalControlApiClient.SendState(Model::TechnicalStateType::Stopped);
    }

    /// ----------------------------------------------------------------------------------------------------------------
    /// ITableProcessorApi::StoreHandler
    /// ----------------------------------------------------------------------------------------------------------------

    void ProcessSubscription(
        const TUiSubscription::TId& aRequestId,
        const TUiSubscription& aSubscriptionInfo,
        const Basis::SPtr<Model::Login>& /* aSessionLogin */,
        SubscriptionType /*aType*/)
    {
        mTracer.InfoSlow("ProcessSubscription: aRequestId:", aRequestId, ", aSubscriptionInfo: ", aSubscriptionInfo);

        if (mSubscriptions.size() >= MaxDbConnectionsCount)
        {
            mTracer.ErrorSlow("Resource is busy.", aRequestId);
            ClientApi.RejectSubscription(aRequestId, TableProcessorRejectType::Disconnected);
            return;
        }

        const auto& filters = aSubscriptionInfo.FilterExpression.Filters;
        if (filters.size() != 1)
        {
            mTracer.ErrorSlow("Wrong subscription: need only one filter.", aRequestId);
            ClientApi.RejectSubscription(aRequestId, TableProcessorRejectType::WrongSubscription);
            return;
        }

        if (filters.cbegin()->Values.Size() != 1
            || filters.cbegin()->Values.StringValues.size() != 1)
        {
            mTracer.ErrorSlow("Wrong subscription: need only one filter parameter.", aRequestId);
            ClientApi.RejectSubscription(aRequestId, TableProcessorRejectType::WrongSubscription);
            return;
        }

        const auto& queryStr = filters.cbegin()->Values.StringValues[0];
        if (!queryStr)
        {
            mTracer.ErrorSlow("Wrong subscription: query is null.", aRequestId);
            ClientApi.RejectSubscription(aRequestId, TableProcessorRejectType::WrongSubscription);
            return;
        }

        auto [it, isEmplaced] = mSubscriptions.emplace(aRequestId, SubscriptionInfo {});
        if (!isEmplaced)
        {
            mTracer.ErrorSlow("Request already exists.", aRequestId);
            ClientApi.RejectSubscription(aRequestId, TableProcessorRejectType::WrongSubscription);
            return;
        }

        auto& reader = it->second.DbReader;
        reader.PerformQuery(
            DbAccess::DatabaseConnectionPool::Get().GetConfig().ConnectionString,
            *queryStr);

        if (!reader.IsValid())
        {
            mSubscriptions.erase(it);
            mTracer.ErrorSlow("Db is not available.", aRequestId, ", ", reader.GetError());
            ClientApi.RejectSubscription(aRequestId, TableProcessorRejectType::Disconnected);
            return;
        }

        it->second.NextPackage = reader.GetNextPackage();

        SendDataToSubscription(it->first, it->second, true);
    }

    void ProcessUnsubscription(const TUiSubscription::TId& aRequestId)
    {
        mTracer.InfoSlow("ProcessUnsubscription:", aRequestId);
        mSubscriptions.erase(aRequestId);
    }

    void ProcessGetNext(const TUiSubscription::TId& aRequestId)
    {
        mTracer.TraceSlow("ProcessGetNext:", aRequestId);
        auto it = mSubscriptions.find(aRequestId);
        if (it == mSubscriptions.end())
        {
            return;
        }
        SendDataToSubscription(it->first, it->second, false);
    }

    void ProcessRecall(const Basis::DateTime&)
    {
        assert(false && "Not implemented");
    }

private:

    void SendDataToSubscription(
        const TUiSubscription::TId& aRequestId,
        SubscriptionInfo& aCtx,
        bool aFirstPacket)
    {
        if (!aCtx.NextPackage.HasValue())
        {
            mTracer.InfoSlow("Subscription finished:", aRequestId);

            if (aFirstPacket)
            {
                // Нужно отправить хотя бы один пакет, прежде чем реджектить
                ClientApi.SendChunkSnapshot(
                    aRequestId,
                    Basis::MakeSPtr<DbAccess::PqxxReader::TPack>(),
                    {},
                    false);
            }
            
            mSubscriptions.erase(aRequestId);
            ClientApi.RejectSubscription(aRequestId, TableProcessorRejectType::Disconnected);
            return;
        }
        auto pack = aCtx.NextPackage;
        aCtx.NextPackage = aCtx.DbReader.GetNextPackage();

        ClientApi.SendChunkSnapshot(aRequestId, pack, {}, aCtx.NextPackage.HasValue());
    }
};
}
