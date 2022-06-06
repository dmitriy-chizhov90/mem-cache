#pragma once

#include <Basis/RemoteApi.hpp>
#include <Basis/EndPointId.hpp>
#include <Common/Tracer.hpp>
#include <Common/ConnectionStateLevels.hpp>

#include "UiServerApiTypes.hpp"

#include "UiLocalStore/ITableProcessorApi.hpp"
#include "UiServerHelpers.hpp"

namespace NTPro::Ecn::NewUiServer
{

constexpr Basis::NetEndPointType TableProcessorApiType{
     UiServerApiTypes::TableProcessor,
     UiServerProject,
     "TableProcessorApiType",
     Basis::EndPointMode::CanBeRemote};

template <ChunkProcessor Type>
struct TableProcessorEvents
{
    using TEventType = Basis::EventTypeFast;
    static constexpr TEventType::TId IdShift = static_cast<TEventType::TId>(Type) * 100;
    
    static constexpr TEventType TableSubscribe = TEventType(1 + IdShift, TableProcessorApiType, "TableSubscribe");
    static constexpr TEventType TableUnsubscribe = TEventType(2 + IdShift, TableProcessorApiType, "TableUnsubscribe");
    static constexpr TEventType TableSnapshot = TEventType(3 + IdShift, TableProcessorApiType, "TableSnapshot");
    static constexpr TEventType TableReject = TEventType(4 + IdShift, TableProcessorApiType, "TableReject");
    static constexpr TEventType FeedbackEvent = TEventType(5 + IdShift, TableProcessorApiType, "FeedbackEvent");
    static constexpr TEventType ChunkSnapshotEvent = TEventType(6 + IdShift, TableProcessorApiType, "ChunkSnapshotEvent");
    static constexpr TEventType GetNextInternalEvent = TEventType(7 + IdShift, TableProcessorApiType, "GetNextInternalEvent");
    static constexpr TEventType RecallInternalEvent = TEventType(8 + IdShift, TableProcessorApiType, "RecallInternalEvent");
};

template <typename TDataPack_>
struct TableProcessorApi
{
    using InterfaceApi= ITableProcessorApi<TDataPack_>;
    using TQueryId = typename InterfaceApi::TQueryId;
    using TQueryIdHasher = Basis::UniqueIdHasher<TQueryId>;
    using TSptrPack = typename InterfaceApi::TDataSPtrPack;

    struct Snapshot : public Basis::Traceable
    {
        TQueryId RequestId;
        TSptrPack Data;

        Snapshot() = default;
        Snapshot(
            const TQueryId& aRequestId,
            const TSptrPack& aData)
            : RequestId(aRequestId)
            , Data(aData)
        {}

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(
                RequestId,
                Data);
        }

        void ToString(std::ostream& stream) const override
        {
            stream << "Snapshot:{";
            FIELD_TO_STREAM(stream, RequestId);
            FIELD_TO_STREAM(stream, Data);
            stream << "}";
        }
    };

    struct ChunkSnapshot : public Basis::Traceable
    {
        TQueryId RequestId;
        TSptrPack Data;
        Basis::Vector<int64_t> DeletedIds;
        bool HasNext{false};
        std::optional<size_t> FeedbackNeeded;

        ChunkSnapshot() = default;
        ChunkSnapshot(
            const TQueryId& aRequestId,
            const TSptrPack& aData,
            const Basis::Vector<int64_t>& aDeletedIds,
            bool aHasNext,
            const std::optional<size_t>& aFeedbackNeeded)
            : RequestId(aRequestId)
            , Data(aData)
            , DeletedIds(aDeletedIds)
            , HasNext(aHasNext)
            , FeedbackNeeded(aFeedbackNeeded)
        {}

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(
                RequestId,
                Data,
                DeletedIds,
                HasNext,
                FeedbackNeeded);
        }

        void ToString(std::ostream& stream) const override
        {
            stream << "ChunkSnapshot:{";
            FIELD_TO_STREAM(stream, RequestId);
            FIELD_TO_STREAM(stream, Data);
            FIELD_TO_STREAM(stream, DeletedIds);
            FIELD_TO_STREAM(stream, HasNext);
            FIELD_TO_STREAM(stream, FeedbackNeeded);
            stream << "}";
        }
    };

    struct Reject : public Basis::Traceable
    {
        TQueryId RequestId;
        TableProcessorRejectType RejectType{TableProcessorRejectType::Disconnected};

        Reject() = default;
        Reject(
            const TQueryId& aRequestId,
            TableProcessorRejectType aRejectType)
            : RequestId(aRequestId)
            , RejectType(aRejectType)
        {}

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(
                RequestId,
                RejectType);
        }

        void ToString(std::ostream& stream) const override
        {
            stream << "Reject:{";
            FIELD_TO_STREAM(stream, RequestId);
            FIELD_TO_STREAM(stream, RejectType);
            stream << "}";
        }
    };

    struct Subscription : public Basis::Traceable
    {
        TQueryId RequestId;
        TradingSerialization::Table::SubscribeBase UiSubscription;
        Basis::SPtr<Model::Login> SessionLogin;
        SubscriptionType Type{SubscriptionType::Chunk};

        Subscription() = default;
        Subscription(
            const TQueryId& aRequestId,
            const TradingSerialization::Table::SubscribeBase& aUiSubscription,
            const Basis::SPtr<Model::Login>& aSessionLogin,
            SubscriptionType aType)
            : RequestId(aRequestId)
            , UiSubscription(aUiSubscription)
            , SessionLogin(aSessionLogin)
            , Type(aType)
        {}

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(
                RequestId,
                UiSubscription,
                SessionLogin,
                Type);
        }

        void ToString(std::ostream& stream) const override
        {
            stream << "Snapshot:{";
            FIELD_TO_STREAM(stream, RequestId);
            FIELD_TO_STREAM(stream, UiSubscription);
            FIELD_TO_STREAM(stream, SessionLogin);
            FIELD_TO_STREAM(stream, Type);
            stream << "}";
        }
    };

    struct Feedback : public Basis::Traceable
    {
        TQueryId RequestId;
        size_t ConfirmedPacksCount{};
        size_t WarnLevel{};

        Feedback() = default;
        Feedback(
            const TQueryId& aRequestId,
            size_t aConfirmedPacksCount,
            size_t aWarnLevel)
            : RequestId(aRequestId)
            , ConfirmedPacksCount(aConfirmedPacksCount)
            , WarnLevel(aWarnLevel)
        {}

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(
                RequestId,
                ConfirmedPacksCount,
                WarnLevel);
        }

        void ToString(std::ostream& stream) const override
        {
            stream << "Feedback:{";
            FIELD_TO_STREAM(stream, RequestId);
            FIELD_TO_STREAM(stream, ConfirmedPacksCount);
            FIELD_TO_STREAM(stream, WarnLevel);
            stream << "}";
        }
    };

    template<typename TSetup>
    class Store : public Basis::RemoteApi::ServerBase<TSetup, Store<TSetup>>
    {
        struct QueryStoreInfo : public Basis::SeriesCounter
        {
            QueryStoreInfo() = default;
            QueryStoreInfo(const Basis::EndPointId& aIdentity)
                : Identity(aIdentity)
            {}
            
            Basis::EndPointId Identity;
        };

    public:

        using TEvents = TableProcessorEvents<TSetup::StoreChunkProcessorType>;

        typename InterfaceApi::template StoreHandler<typename TSetup::TTableProcessorApiStoreHandler> Handler;

    public:

        Store(
            const Basis::ComponentId& aComponentId,
            Basis::EventRegistry& aEventRegistry,
            typename TSetup::TTableProcessorApiStoreHandler* aHandler = nullptr)
            : Basis::RemoteApi::ServerBase<TSetup, Store>(aComponentId, TableProcessorApiType, aEventRegistry)
            , mTracer(Basis::Tracing::GetTracer(aComponentId, "StoreApi"))
        {
            this->template RegisterOutEvent<Snapshot>(TEvents::TableSnapshot);
            this->template RegisterOutEvent<ChunkSnapshot>(TEvents::ChunkSnapshotEvent);
            this->template RegisterOutEvent<Reject>(TEvents::TableReject);

            this->template RegisterHandler(TEvents::TableSubscribe, &Store<TSetup>::ProcessSubscription);
            this->template RegisterHandler(TEvents::TableUnsubscribe, &Store<TSetup>::ProcessUnsubscription);

            this->template RegisterHandler(TEvents::FeedbackEvent, &Store<TSetup>::ProcessFeedback);
            this->template RegisterHandler(TEvents::GetNextInternalEvent, &Store<TSetup>::ProcessGetNextInternal);

            this->RegisterHandler(Basis::RecallEvent, &Store<TSetup>::ProcessRecall);

            if (aHandler)
            {
                Handler.Bind(aHandler);
            }
        }

        void SendDataSnapshot(
            const TQueryId& aRequestId,
            const TSptrPack& aData)
        {
            assert(aData.HasValue());

            auto client = mQueries.find(aRequestId);
            if(client == mQueries.end())
            {
                assert(false);
                return;
            }

            this->SendToTarget(
                client->second.Identity,
                TEvents::TableSnapshot.Id,
                Basis::MakeSPtr<Snapshot>(aRequestId, aData));
        }

        void SendChunkSnapshot(
            const TQueryId& aRequestId,
            const TSptrPack& aData,
            const Basis::Vector<int64_t>& aDeletedIds,
            bool aHasNext)
        {
            auto it = mQueries.find(aRequestId);
            if(it == mQueries.end())
            {
                assert(false);
                return;
            }

            auto& client = it->second;
            auto feedbackNeeded = client.ProcessPack();
            if (!feedbackNeeded)
            {
                SendGetNextInternal(aRequestId);
            }

            mTracer.InfoSlow(
                "SendChunkSnapshot:", aRequestId,
                (feedbackNeeded
                    ? std::string { ", request feedback" }
                    : std::string {}),
                ", pack: ", client.SubscriptionPackCounter,
                " (", client.SeriesPackCounter,
                " of ", client.MaxPacksCount, ")");

            this->SendToTarget(
                client.Identity,
                TEvents::ChunkSnapshotEvent.Id,
                Basis::MakeSPtr<ChunkSnapshot>(
                    aRequestId,
                    aData,
                    aDeletedIds,
                    aHasNext,
                    feedbackNeeded));
        }

        void RejectSubscription(
            const TQueryId& aRequestId,
            TableProcessorRejectType aRejectType)
        {
            auto client = mQueries.find(aRequestId);
            if(client == mQueries.end())
            {
                assert(false);
                return;
            }

            this->SendToTarget(
                client->second.Identity,
                TEvents::TableReject.Id,
                Basis::MakeSPtr<Reject>(aRequestId, aRejectType));
            mQueries.erase(client);
        }

        void SetIsRecallNeeded(bool aIsRecallNeeded, bool aForceAsyncCall)
        {
            Basis::RemoteApi::ServerBase<TSetup, Store>::SetIsRecallNeeded(aIsRecallNeeded, aForceAsyncCall);
        }

        bool IsRecallNeeded() const
        {
            return Basis::RemoteApi::ServerBase<TSetup, Store>::IsRecallNeeded();
        }

    private:

        void ProcessRecall(const Basis::SenderInfo& aSenderIdentity)
        {
            Handler.ProcessRecall(aSenderIdentity.ReactorTime);
        }

        void ProcessSubscription(const Basis::SenderInfo& aSenderIdentity, const Subscription& aSubscription)
        {
            mTracer.InfoSlow("ProcessSubscription:", aSubscription.RequestId);
            
            if (mQueries.emplace(aSubscription.RequestId, QueryStoreInfo { aSenderIdentity.BusinessId }).second)
            {
                Handler.ProcessSubscription(
                    aSubscription.RequestId,
                    aSubscription.UiSubscription,
                    aSubscription.SessionLogin,
                    aSubscription.Type);
            }
        }

        void ProcessUnsubscription(
            const Basis::SenderInfo& /*aSenderIdentity*/,
            const TQueryId& aRequestId)
        {
            auto client = mQueries.find(aRequestId);
            if (client != mQueries.end())
            {
                Handler.ProcessUnsubscription(aRequestId);
                mQueries.erase(client);
            }
        }

        void ProcessFeedback(const Basis::SenderInfo& /*aSenderIdentity*/, const Feedback& aFeedback)
        {
            auto it = mQueries.find(aFeedback.RequestId);
            if (it == mQueries.end())
            {
                return;
            }

            auto& client = it->second;

            mTracer.InfoSlow(
                "ProcessFeedback:", aFeedback.RequestId,
                ", wl: ", aFeedback.WarnLevel);

            if (client.ProcessFeedback(
                aFeedback.ConfirmedPacksCount,
                aFeedback.WarnLevel))
            {
                ProcessGetNextInternal(Basis::SenderInfo {}, aFeedback.RequestId);
            }
        }

        void ProcessGetNextInternal(const Basis::SenderInfo& /*aSenderIdentity*/, const TQueryId& aRequestId)
        {
            Handler.ProcessGetNext(aRequestId);
        }

        void ProcessSessionState(const Basis::EndPointId& aClientIdentity, bool aIsConnected)
        {
            if (aIsConnected)
            {
                return;
            }

            RemoveSubscriptionsByClient(aClientIdentity);
        }

        bool IsSessionConnected(const Basis::EndPointId& aClientIdentity)
        {
            return this->IsSessionConnectedInternal(aClientIdentity);
        }

        void RemoveSubscriptionsByClient(const Basis::EndPointId& aClientIdentity)
        {
            auto it = mQueries.begin();
            auto end = mQueries.end();

            while (it != end)
            {
                if (it->second.Identity == aClientIdentity)
                {
                    Handler.ProcessUnsubscription(it->first);
                    it = mQueries.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void SendGetNextInternal(const TQueryId& aRequestId)
        {
            Basis::NetEvent event(
                this->GetIdentity(),
                this->GetIdentity(),
                TEvents::GetNextInternalEvent.Id,
                Basis::MakeSPtr<TQueryId>(aRequestId));

            this->Send(std::move(event));
        }

    private:
        Basis::Tracer& mTracer;
        
        Basis::UnorderedMap<TQueryId, QueryStoreInfo, TQueryIdHasher> mQueries;

        friend Basis::RemoteApi::ServerBase<TSetup, Store<TSetup>>;
    };

    template<typename TSetup>
    class Processor : public Basis::RemoteApi::ClientBase<TSetup, Processor<TSetup>>
    {

        struct QueryProcessorInfo
        {
            size_t StoreIndex {};
            Basis::QueueNotTbb<Basis::SPtr<ChunkSnapshot>> PackQueue {};
            bool PackRequested {true};
            size_t PeakQueueSize {};
            std::optional<TableProcessorRejectType> PendingReject;

            QueryProcessorInfo() = default;
            QueryProcessorInfo(size_t aStoreIndex)
                : StoreIndex { aStoreIndex }
            {}
        };

    public:

        using TEvents = TableProcessorEvents<TSetup::ProcessorChunkProcessorType>;
        using TPreProcessFunc = std::function<void(QueryProcessorInfo&)>;

        typename InterfaceApi::template ProcessorHandler<typename TSetup::TTableProcessorApiProcessorHandler> Handler;

    public:

        Processor(
            SubscriptionType aType,
            const std::string& aParentName,
            const std::string& aServerNameBase,
            size_t aServersCount,
            Basis::EventRegistry& aEventRegistry,
            typename TSetup::TTableProcessorApiProcessorHandler* aHandler = nullptr)
            : Basis::RemoteApi::ClientBase<TSetup, Processor>(
                MakeStoreTypeComponentId(aParentName, aType, TSetup::ProcessorChunkProcessorType),
                TableProcessorApiType,
                aEventRegistry)
            , mTracer(Basis::Tracing::GetTracer(
                aParentName.c_str(),
                MakeStoreTypeName("StoreApi", aType, TSetup::ProcessorChunkProcessorType)))
        {
            this->template RegisterOutEvent<Subscription>(TEvents::TableSubscribe);
            this->template RegisterOutEvent<TQueryId>(TEvents::TableUnsubscribe);
            this->template RegisterOutEvent<Feedback>(TEvents::FeedbackEvent);

            this->template RegisterHandler(TEvents::TableSnapshot, &Processor<TSetup>::ProcessDataSnapshot);
            this->template RegisterHandler(TEvents::ChunkSnapshotEvent, &Processor<TSetup>::ProcessChunkSnapshot);
            this->template RegisterHandler(TEvents::TableReject, &Processor<TSetup>::ProcessReject);
            this->template RegisterHandler(TEvents::RecallInternalEvent, &Processor<TSetup>::ProcessRecallSubscriptionInternal);

            if (aHandler)
            {
                Handler.Bind(aHandler);
            }

            for (size_t i = 0; i < aServersCount; ++i)
            {
                mServerIdentities.emplace_back(
                    MakeIndexedContext(
                        MakeStoreTypeName(aServerNameBase, aType, TSetup::ProcessorChunkProcessorType),
                        i).c_str(),
                    Basis::EndPointSide::Server,
                    TableProcessorApiType);
            }
        }

        bool Subscribe(
            const TQueryId& aRequestId,
            const TradingSerialization::Table::SubscribeBase& aUiSubscription,
            const Basis::SPtr<Model::Login>& aSessionLogin,
            SubscriptionType aType)
        {
            if (!IsSessionConnected())
            {
                mTracer.Info("Subscribe: is disconnected");
                return false;
            }

            if (mServerIdentities.empty())
            {
                mTracer.Error("Subscribe: has no server identities");
                assert(false);
                return false;
            }

            auto routeIndex = GetNextSubscriptionRouteIndex();

            auto [it, emplaced] = mActiveQueries.emplace(aRequestId, QueryProcessorInfo { routeIndex });
            if (!emplaced)
            {
                mTracer.Error("Subscribe: query duplicated");
                assert(false);
                return false;
            }

            mTracer.InfoSlow("Subscribe:", aRequestId, ", queue size: ", it->second.PackQueue.size(), ". Send from ", this->GetIdentity(), " to ", mServerIdentities[routeIndex]);

            this->SendToTarget(
                mServerIdentities[routeIndex],
                TEvents::TableSubscribe.Id,
                Basis::MakeSPtr<Subscription>(aRequestId, aUiSubscription, aSessionLogin, aType));

            return true;
        }

        bool Unsubscribe(const TQueryId& aRequestId)
        {
            auto it = mActiveQueries.find(aRequestId);
            if (it == mActiveQueries.cend())
            {
                return false;
            }
            auto routeIndex = it->second.StoreIndex;
            mActiveQueries.erase(it);

            if (IsSessionConnected())
            {
                this->SendToTarget(
                    mServerIdentities[routeIndex],
                    TEvents::TableUnsubscribe.Id,
                    Basis::MakeSPtr(aRequestId));
            }

            return true;
        }

        void StartSession()
        {
            for (const auto& identity : mServerIdentities)
            {
                this->StartSessionInternal(identity);
            }
        }

        void StopSession()
        {
            for (const auto& identity : mServerIdentities)
            {
                this->StopSessionInternal(identity);
            }
        }

        [[nodiscard]] bool IsSessionStarted() const
        {
            if (mServerIdentities.empty())
            {
                return false;
            }
            return this->IsSessionStartedInternal(mServerIdentities[0]);
        }

        [[nodiscard]] bool IsSessionConnected(const std::optional<size_t>& aStoreIndex = std::nullopt) const
        {
            if (aStoreIndex)
            {
                return this->IsSessionConnectedInternal(mServerIdentities[*aStoreIndex]);
            }
            
            for (const auto& identity : mServerIdentities)
            {
                if (!this->IsSessionConnectedInternal(identity))
                {
                    return false;
                }
            }
            return true;
        }

        void GetNext(const TQueryId& aRequestId)
        {
            ProcessQuery(
                aRequestId,
                [](auto& info) { info.PackRequested = true; });
        }

        void RecallSubscription(const TQueryId& aRequestId)
        {
            Basis::NetEvent event(
                this->GetIdentity(),
                this->GetIdentity(),
                TEvents::RecallInternalEvent.Id,
                Basis::MakeSPtr<TQueryId>(aRequestId));

            this->Send(std::move(event));
        }

    private:

        void ProcessSessionState(const Basis::EndPointId& aIdentity, bool aIsConnected)
        {
            if (!aIsConnected)
            {
                for (auto it = mActiveQueries.begin(); it != mActiveQueries.end();)
                {
                    const auto& [request, info] = *it;
                    if (mServerIdentities[info.StoreIndex] == aIdentity)
                    {
                        Handler.ProcessSubscriptionRejected(request, TableProcessorRejectType::Disconnected);
                        it = mActiveQueries.erase(it);
                    }
                    else
                    {
                         ++it;
                    }
                }
            }
        }

        void ProcessDataSnapshot(
            const Basis::SenderInfo& aIdentity,
            const Snapshot& aSnapshot)
        {
            auto it = mActiveQueries.find(aSnapshot.RequestId);
            if (it == mActiveQueries.cend())
            {
                return;
            }

            const auto& info = it->second;
            if (aIdentity.BusinessId != mServerIdentities[info.StoreIndex])
            {
                /// Пока нет динамического роутинга, Identities должны совпадать.
                assert(false);
                return;
            }

            Handler.ProcessDataSnapshot(aSnapshot.RequestId, aSnapshot.Data);
        }

        void ProcessChunkSnapshot(
            [[maybe_unused]] const Basis::SenderInfo& aIdentity,
            const Basis::SPtr<ChunkSnapshot>& aSnapshot)
        {
            ProcessQuery(
                aSnapshot->RequestId,
                [&](auto& info)
            {
                /// Пока нет динамического роутинга, Identities должны совпадать.
                assert(aIdentity.BusinessId == mServerIdentities[info.StoreIndex]);
                info.PackQueue.push(aSnapshot);
            });
        }

        void ProcessReject(
            [[maybe_unused]] const Basis::SenderInfo& aIdentity,
            const Reject& aReject)
        {
            ProcessQuery(
                aReject.RequestId,
                [&](auto& info)
            {
                /// Пока нет динамического роутинга, Identities должны совпадать.
                assert(aIdentity.BusinessId == mServerIdentities[info.StoreIndex]);
                info.PendingReject = aReject.RejectType;
            });
        }

        void ProcessRecallSubscriptionInternal(
            const Basis::SenderInfo& /*aSenderIdentity*/,
            const TQueryId& aRequestId)
        {
            Handler.ProcessRecallSubscription(aRequestId);
        }

        void ProcessQuery(const TQueryId& aRequestId, TPreProcessFunc aPreProcess)
        {
            auto it = mActiveQueries.find(aRequestId);
            if (it == mActiveQueries.cend())
            {
                return;
            }
            else
            {
                auto& info = it->second;
                aPreProcess(info);

                mTracer.InfoSlow(
                    "ProcessQuery:", aRequestId, ", size: ", info.PackQueue.size());

                ProcessPackQueue(aRequestId, info);

                if (info.PackQueue.empty() && info.PendingReject)
                {
                    Handler.ProcessSubscriptionRejected(aRequestId, *info.PendingReject);
                    mActiveQueries.erase(it);
                }
            }
            mTracer.InfoSlow("ProcessQuery:", aRequestId, " processed");
        }

        void ProcessPackQueue(const TQueryId& aRequestId, QueryProcessorInfo& outInfo)
        {
            outInfo.PeakQueueSize = (std::max)(outInfo.PeakQueueSize, outInfo.PackQueue.size());
            
            if (!outInfo.PackRequested || outInfo.PackQueue.empty())
            {
                return;
            }

            auto snapshot = outInfo.PackQueue.front();
            outInfo.PackRequested = false;
            outInfo.PackQueue.pop();

            if (outInfo.PackQueue.empty() && snapshot->FeedbackNeeded)
            {
                auto warnLevel = (outInfo.PeakQueueSize >= QueueSizeUpperBound) ? Basis::MaxWarnLevel : 0;
                if (IsSessionConnected(outInfo.StoreIndex))
                {
                    this->SendToTarget(
                        mServerIdentities[outInfo.StoreIndex],
                        TEvents::FeedbackEvent.Id,
                        Basis::MakeSPtr<Feedback>(aRequestId, *snapshot->FeedbackNeeded, warnLevel));
                }
                outInfo.PeakQueueSize = 0;
            }

            Handler.ProcessChunkSnapshot(
                snapshot->RequestId,
                snapshot->Data,
                snapshot->DeletedIds,
                snapshot->HasNext);
        }

        size_t GetNextSubscriptionRouteIndex()
        {
            if ((++mSubscriptionRouteIndex) >= mServerIdentities.size())
            {
                mSubscriptionRouteIndex = 0;
            }
            assert(mSubscriptionRouteIndex < mServerIdentities.size());
            return mSubscriptionRouteIndex;
        }

    private:

        static constexpr size_t QueueSizeUpperBound = 100;
        
        Basis::Vector<Basis::EndPointId> mServerIdentities;
        size_t mSubscriptionRouteIndex = 0;

        Basis::Tracer& mTracer;

        /// subscription id -> route index
        Basis::UnorderedMap<TQueryId, QueryProcessorInfo, TQueryIdHasher> mActiveQueries;

        friend Basis::RemoteApi::ClientBase<TSetup, Processor<TSetup>>;
    };
};

}
