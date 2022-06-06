#pragma once

#include <Common/Collections.hpp>
#include <Common/Pack.hpp>
#include <Common/StringUtils.hpp>

#include <Basis/EventRegistry.hpp>

#include "TradingSerialization/Replies/ServerDisconnectReply.hpp"

#include "TradingSerialization/Table/Columns.hpp"
#include "TradingSerialization/Table/Subscription.hpp"

#include <Basis/ITechnicalControlApi.hpp>
#include <Basis/DbAccess/PqxxReader.hpp>

#include "UiLocalStore/ILocalStoreLogic.hpp"
#include "UiLocalStore/IQueryApiWrapper.hpp"
#include "UiLocalStore/ITableProcessorApi.hpp"
#include "UiLocalStore/ISubscriptionRouter.hpp"

#include "UiSession.hpp"

namespace NTPro::Ecn::NewUiServer
{

/**
 * \class UiCacheComponent
 * \brief Компонент для локального кэширования данных.
 * \ingroup NewUiServer
 * Хранит данные одного типа в удобном для их фильтрации и сортировки виде.
 */
template <typename TSetup>
class UiCacheComponent
{
    using TUiSubscription = TradingSerialization::Table::SubscribeBase;

    using TData = typename TSetup::TData;

    using TLocalStoreLogicInterface = ILocalStoreLogic<TData>;

    static constexpr SubscriptionType StoreType = TSetup::StoreType;

    struct DbSubscriptionInfo
    {
        TUiSubscription Subscription;
        bool WaitNextPacket {true};
        bool FinalPacketReceived {false};

        DbSubscriptionInfo() = default;
        DbSubscriptionInfo(const TUiSubscription& aSubscription)
            : Subscription(aSubscription)
        {}
    };

    Basis::Tracer& mTracer;

    Basis::Map<TUiSubscription::TId, DbSubscriptionInfo> mDbQueries;
public:

    Service::ITechnicalControlApi::Client<typename TSetup::TTechnicalControlApiClient, Basis::Own> TechnicalControlApiClient;

    using TQueryApiWrapperLogicImpl = typename TSetup::TQueryApiWrapper;
    using TQueryApiWrapperLogic = IQueryApiWrapper::Logic<
        TQueryApiWrapperLogicImpl,
        Basis::Own>;
    TQueryApiWrapperLogic QueryApiWrapper;

    typename ITableProcessorApi<Basis::Pack<TData>>::template Store<
        typename TSetup::TTableProcessorApiStore,
        Basis::Own> TableProcessor;

    typename ITableProcessorApi<DbAccess::PqxxReader::TPack>::template Processor<
        typename TSetup::TTableProcessorApiDbReaderClient,
        Basis::Own> DbReader;

    typename TLocalStoreLogicInterface::template Logic<
        typename TSetup::TLocalStoreLogic> Logic;

    typename ISubscriptionRouter<Basis::Pack<TData>>::template Logic<
        typename TSetup::TSubscriptionRouter,
        Basis::Bind> SubscriptionRouter;
    
public:
    UiCacheComponent(
        const Basis::ComponentId& aComponentId,
        const Basis::ComponentId& aSettingsStoreComponentId,
        const std::string& aDbReaderName,
        size_t aDbReadersCount,
        Basis::EventRegistry& aEventRegistry)
        : mTracer(Basis::Tracing::GetTracer(aComponentId, "UiCache"))
        , TechnicalControlApiClient(aComponentId, aEventRegistry, this)
        , QueryApiWrapper(aComponentId, aSettingsStoreComponentId, aEventRegistry, this, mTracer)
        , TableProcessor(aComponentId, aEventRegistry, this)
        , DbReader(StoreType, aComponentId.GetName(), aDbReaderName, aDbReadersCount, aEventRegistry, this)
        , Logic(mTracer)
    {
    }

    void ProcessTechnicalStart()
    {
        mTracer.Info("ProcessTechnicalStart");

        QueryApiWrapper.StartSession();
        DbReader.StartSession();
        TechnicalControlApiClient.SendState(Model::TechnicalStateType::Started);
    }

    void ProcessTechnicalStop()
    {
        mTracer.Info("ProcessTechnicalStop");

        QueryApiWrapper.StopSession();
        DbReader.StopSession();
        Logic.Clear();
        TechnicalControlApiClient.SendState(Model::TechnicalStateType::Stopped);
    }

    /// ----------------------------------------------------------------------------------------------------------
    /// ITableProcessorApi::StoreHandler (from Processor)
    /// ----------------------------------------------------------------------------------------------------------

    void ProcessSubscription(
        const TUiSubscription::TId& aRequestId,
        const typename TLocalStoreLogicInterface::TUiSubscription& aSubscriptionInfo,
        const Basis::SPtr<Model::Login>& aSessionLogin,
        SubscriptionType aType)
    {
        mTracer.InfoSlow("ProcessSubscription: aRequestId:", aRequestId, ", aSubscriptionInfo: ", aSubscriptionInfo);

        if (StoreType != aType)
        {
            mTracer.InfoSlow("Wrong subscription: store type:", StoreType, ", aType: ", aType);
            TableProcessor.RejectSubscription(aRequestId, TableProcessorRejectType::WrongSubscription);
            return;
        }

        if (!Logic.IsReady())
        {
            // Если подписка будет отправлена в DbReader, то запрос должен быть составлен так,
            // чтобы наборы данных из БД и кэша не пересекались и не было данных, не попадающих ни в один набор.
            // Для нахождения границы используется минимальное время записи в кэше.
            // Поэтому для обработки подписки кэш должен быть инициализирован.
            mTracer.Info("Cache is not ready");
            TableProcessor.RejectSubscription(aRequestId, TableProcessorRejectType::Disconnected);
            return;
        }

        if (SubscriptionRouter.IsDbQuery(aSubscriptionInfo))
        {
            mTracer.InfoSlow("Is db query:", aRequestId);

            if (!mDbQueries.emplace(aRequestId, DbSubscriptionInfo { aSubscriptionInfo }).second)
            {
                mTracer.WarningSlow("Subscription duplicated:", aRequestId);
                TableProcessor.RejectSubscription(aRequestId, TableProcessorRejectType::WrongSubscription);
                return;
            }

            auto sqlSubscription = SubscriptionRouter.MakeDbQuery(aSubscriptionInfo);
            if (!sqlSubscription)
            {
                mTracer.WarningSlow("Cannot create sql query:", aRequestId);
                mDbQueries.erase(aRequestId);
                TableProcessor.RejectSubscription(aRequestId, TableProcessorRejectType::WrongSubscription);
                return;
            }
            
            if (!DbReader.Subscribe(aRequestId, *sqlSubscription, aSessionLogin, aType))
            {
                mTracer.WarningSlow("Db reader subscription failed:", aRequestId);
                mDbQueries.erase(aRequestId);
                TableProcessor.RejectSubscription(aRequestId, TableProcessorRejectType::Disconnected);
                return;
            }
        }
        else
        {
            ProcessSubscriptionInternal(aRequestId, aSubscriptionInfo, false);
        }
    }

    void ProcessUnsubscription(const TUiSubscription::TId& aRequestId)
    {
        auto it = mDbQueries.find(aRequestId);
        if (it != mDbQueries.cend())
        {
            mDbQueries.erase(it);
            DbReader.Unsubscribe(aRequestId);
        }
        else
        {
            Logic.ProcessUnsubscription(aRequestId);
        }
    }

    void ProcessGetNext(const TUiSubscription::TId& aRequestId)
    {
        mTracer.TraceSlow("ProcessGetNext", aRequestId);
        auto it = mDbQueries.find(aRequestId);
        if (it != mDbQueries.cend())
        {
            assert(!it->second.WaitNextPacket);
            it->second.WaitNextPacket = true;

            if (!TrySwitchSubscription(it->first, it->second, false))
            {
                DbReader.GetNext(aRequestId);
            }
        }
        else
        {
            SendDataToSubscription(Logic.ProcessGetNext(aRequestId));
            ManageRecalls();
        }
    }

    void ProcessRecall(const Basis::DateTime& /*aReactorTime*/)
    {
        mTracer.Trace("ProcessRecall");
        TableProcessor.SetIsRecallNeeded(false, false);

        SendDataToSubscription(Logic.ProcessDefferedTasks());

        ManageRecalls();
    }

    /// ----------------------------------------------------------------------------------------------------------
    /// ITableProcessorApi::ProcessorHandler (from DbReader)
    /// ----------------------------------------------------------------------------------------------------------

    void ProcessRecallSubscription(const TUiSubscription::TId&)
    {
        assert(false);
    }
    
    void ProcessDataSnapshot(const TUiSubscription::TId&, const Basis::SPtr<DbAccess::PqxxReader::TPack>&)
    {
        assert(false && "Not implemented");
    }

    void ProcessChunkSnapshot(
        const TUiSubscription::TId& aRequestId,
        const Basis::SPtr<DbAccess::PqxxReader::TPack>& aData,
        const Basis::Vector<int64_t>& aDeletedIds,
        bool aHasNext)
    {
        mTracer.DebugSlow(
            "Process chunk pack, size:", (aData.HasValue() ? aData->size() : 0),
            ", deleted size: ", aDeletedIds.size(),
            ", has next: ", aHasNext,
            ", ", aRequestId);

        auto it = mDbQueries.find(aRequestId);
        if (it == mDbQueries.cend())
        {
            mTracer.WarningSlow("Subscription not found:", aRequestId);
            return;
        }

        // Ожидаем, что пустой пакет может прийти только последним.
        // Иначе нарушится последовательность -GetNext-> <-Pack-.
        assert(!aData->empty() || !aHasNext);
        
        if (!aData->empty())
        {
            assert(it->second.WaitNextPacket);
            it->second.WaitNextPacket = false;
            
            TableProcessor.SendChunkSnapshot(
                aRequestId,
                SubscriptionRouter.MakePack(aData),
                aDeletedIds,
                true);
        }

        it->second.FinalPacketReceived = !aHasNext;
        /// true - включить реколы для TableProcessorAPI из TableProcessorApi::ProcessorHandler.
        TrySwitchSubscription(it->first, it->second, true);
    }

    void ProcessSubscriptionRejected(
        const TUiSubscription::TId& aRequestId,
        TableProcessorRejectType aRejectType)
    {
        auto it = mDbQueries.find(aRequestId);
        if (it == mDbQueries.cend() || it->second.FinalPacketReceived)
        {
            mTracer.InfoSlow("DbReader subscription finished:", aRequestId);
            return;
        }

        mTracer.WarningSlow("DbReader subscription rejected:", aRequestId, aRejectType);
        mDbQueries.erase(it);
        TableProcessor.RejectSubscription(aRequestId, aRejectType);
    }
    
    /// ----------------------------------------------------------------------------------------------------------
    /// IQueryApiWrapper
    /// ----------------------------------------------------------------------------------------------------------

    template <typename TPack>
    void ProcessDataUpdate(const Basis::SPtr<TPack>& aPack)
    {
        Logic.ProcessDataUpdate(aPack);
        /// true - включить реколы для TableProcessorAPI из SettingsQueryApiClient::Handler.
        ManageRecalls(true);
    }

    void ProcessDataSubscriptionReject()
    {}

    void ProcessQueryApiConnected()
    {
        RejectSubscriptions(Logic.Clear(), TableProcessorRejectType::Disconnected);
    }

    void ProcessQueryApiDisconnected()
    {}

private:

    void RejectSubscriptions(const Basis::Vector<TUiSubscription::TId>& aRequstIds, TableProcessorRejectType aReason)
    {
        for (const auto& requestId : aRequstIds)
        {
            TableProcessor.RejectSubscription(requestId, aReason);
        }
    }

    /// aForceAsyncCall нужно выставлять в true, если вызываем не из методов ITableProcessorApi::StoreHandler.
    void ManageRecalls(bool aForceAsyncCall = false)
    {
        RejectSubscriptions(
            Logic.GetRejectedSubscriptions(),
            TableProcessorRejectType::WrongSubscription);

        if (Logic.IsRecallNeeded() && !TableProcessor.IsRecallNeeded())
        {
            mTracer.Trace("Set recall needed");
            TableProcessor.SetIsRecallNeeded(true, aForceAsyncCall);
        }
    }

    void SendDataToSubscription(const typename TLocalStoreLogicInterface::TPendingUpdate& aUpdate)
    {
        if (aUpdate.IsOk())
        {
            assert(aUpdate.SubscriptionId);
            if constexpr (StoreType == SubscriptionType::Table)
            {
                assert(aUpdate.Result.HasValue());
                mTracer.InfoSlow(
                    "Send data snapshot: RequestId:", *aUpdate.SubscriptionId,
                    ", size: ", aUpdate.Result->size());
                TableProcessor.SendDataSnapshot(
                    *aUpdate.SubscriptionId,
                    aUpdate.Result);
            }
            else
            {
                TableProcessor.SendChunkSnapshot(
                    *aUpdate.SubscriptionId,
                    aUpdate.Result,
                    aUpdate.DeletedIds,
                    aUpdate.HasNext());
            }

        }
    }

    void ProcessSubscriptionInternal(
        const TUiSubscription::TId& aRequestId,
        const typename TLocalStoreLogicInterface::TUiSubscription& aSubscriptionInfo,
        bool aForceAsyncCall)
    {
        auto error = Logic.ProcessSubscription(aRequestId, aSubscriptionInfo);
        if (error.has_value())
        {
            mTracer.Info("Wrong subscription: processing failed");
            TableProcessor.RejectSubscription(aRequestId, *error);
            return;
        }
        ManageRecalls(aForceAsyncCall);
    }

    bool TrySwitchSubscription(
        const TUiSubscription::TId& aRequestId,
        const DbSubscriptionInfo& aInfo,
        bool aForceAsyncCall)
    {
        if (aInfo.WaitNextPacket && aInfo.FinalPacketReceived)
        {
            ProcessSubscriptionInternal(aRequestId, aInfo.Subscription, aForceAsyncCall);
            mDbQueries.erase(aRequestId);
            
            return true;
        }
        else
        {
            return false;
        }
    }
};
}
