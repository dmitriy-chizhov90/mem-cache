#pragma once

#include <NewUiServer/UiSession.hpp>
#include <TradingSerialization/Table/Subscription.hpp>

#include <Trading/Model/ActionType.hpp>

#include <Common/Collections.hpp>
#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>
#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

enum class TableProcessorRejectType
{
    Disconnected = 1,
    WrongSubscription,
};

inline std::ostream& operator<<(std::ostream& out, TableProcessorRejectType value)
{
    switch (value)
    {
    case TableProcessorRejectType::Disconnected:
        return out << "Disconnected";
    case TableProcessorRejectType::WrongSubscription:
        return out << "Wrong subscription parameters";
    }
    return out << "???";
}

enum class SubscriptionType
{
    Chunk = 1,
    Table,
};

inline std::ostream& operator<<(std::ostream& out, SubscriptionType value)
{
    switch (value)
    {
    case SubscriptionType::Chunk:
        return out << "Chunk";
    case SubscriptionType::Table:
        return out << "Table";
    }
    return out << "???";
}

inline std::string MakeStoreTypeName(
    const std::string& aBaseName,
    SubscriptionType aType,
    ChunkProcessor aProcessor)
{
    using namespace Common::MsgProcessor;

    auto base = aBaseName + GetProcessorPostfix(aProcessor);

    if (aType == SubscriptionType::Chunk)
    {
        return base + "C";
    }
    else
    {
        return base + "T";
    }
}

inline static Basis::ComponentId MakeStoreTypeComponentId(
    const std::string& aBaseName,
    SubscriptionType aType,
    ChunkProcessor aProcessor)
{
    return Basis::ComponentId(MakeStoreTypeName(aBaseName, aType, aProcessor).c_str());
}

struct StoresCount
{
    struct TypeStoresCount
    {
        size_t TableCount = 0;
        size_t ChunkCount = 0;
    };

    TypeStoresCount QuoterSettingsStoresCount;
    TypeStoresCount ExecutorSettingsStoresCount;
    TypeStoresCount SourceInstrumentSettingsStoresCount;
    TypeStoresCount PriceLevelSettingsStoresCount;
    TypeStoresCount AuditStoresCount;
};

/**
 * \brief API для связи табличного процессора с DataStore.
 * \ingroup NewUiServer
 *  При подписке:
 *  - процессор проверяет валидность набора фильтров и порядка сортировки,
 *  - подписывается на Store (с указанием дополнительной информации о сессии, если необходимо),
 *  - Store возвращает набор отфильтрованных и отсортированных данных
 *  (возможно для этого потребуется также преобразование данных для сессии),
 *  - процессор конвертирует эти данные в табличные.
 */
    template <typename TDataPack_>
struct ITableProcessorApi
{
    using TQueryId = TradingSerialization::Table::SubscribeBase::TId;
    using TDataPack = TDataPack_;
    using TDataSPtrPack = Basis::SPtr<TDataPack>;

    struct DataChunk
    {
        TDataSPtrPack Data;
        std::vector<int64_t> DeletedIds;
        bool HasNext{false};

        DataChunk() = default;
        DataChunk(
            const TDataSPtrPack& aData,
            const std::vector<int64_t>& aDeletedIds,
            bool aHasNext)
            : Data(aData)
            , DeletedIds(aDeletedIds)
            , HasNext(aHasNext)
        {}
    };


    template<typename TImpl, typename TOwnership = Basis::Bind>
    struct Store : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD(SendDataSnapshot,
            const TQueryId& /* aRequestId */,
            const TDataSPtrPack& /* aData */)

        API_METHOD(SendChunkSnapshot,
            const TQueryId& /* aRequestId */,
            const TDataSPtrPack& /* aData */,
            const Basis::Vector<int64_t>& /* aDeletedIds */,
            bool /* aHasNext */)

        API_METHOD(RejectSubscription,
            const TQueryId& /* aRequestId */,
            TableProcessorRejectType /* aRejectType */)

        API_METHOD(SetIsRecallNeeded,
            bool /* aIsRecallNeeded */,
            bool /* aForceAsyncCall */)
        CONST_API_METHOD_RETURN(bool, IsRecallNeeded)
    };

    template<typename TImpl, typename TOwnership = Basis::Bind>
    struct StoreHandler : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD(ProcessSubscription,
            const TQueryId& /* aRequestId */,
            const TradingSerialization::Table::SubscribeBase& /* aSubscription */,
            const Basis::SPtr<Model::Login>& /* aSessionLogin */,
            SubscriptionType /* aType */)
        API_METHOD(ProcessUnsubscription, const TQueryId& /* aRequestId */)
        API_METHOD(ProcessGetNext, const TQueryId& /* aRequestId */)

        API_METHOD(ProcessRecall, const Basis::DateTime& /* aNow */)
    };

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Processor : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD(StartSession)
        API_METHOD(StopSession)
        CONST_API_METHOD_RETURN(bool, IsSessionStarted)
        CONST_API_METHOD_RETURN(bool, IsSessionConnected)

        API_METHOD_RETURN(bool, Subscribe,
            const TQueryId& /* aRequestId */,
            const TradingSerialization::Table::SubscribeBase& /* aSubscription */,
            const Basis::SPtr<Model::Login>& /* aSessionLogin */,
            SubscriptionType /* aType */)
        API_METHOD_RETURN(bool, Unsubscribe,
            const TQueryId& /* aRequestId */)

        API_METHOD(GetNext, const TQueryId& /* aRequestId */)
        API_METHOD(RecallSubscription, const TQueryId& /* aRequestId */)
    };

    template<typename TImpl, typename TOwnership = Basis::Bind>
    struct ProcessorHandler : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD(ProcessRecallSubscription,
            const TQueryId& /* aRequestId */)

        API_METHOD(ProcessDataSnapshot,
            const TQueryId& /* aRequestId */,
            const TDataSPtrPack& /* aData */)

        API_METHOD(ProcessChunkSnapshot,
            const TQueryId& /* aRequestId */,
            const TDataSPtrPack& /* aData */,
            const Basis::Vector<int64_t>& /* aDeletedIds */,
            bool /* aHasNext */)

        API_METHOD(ProcessSubscriptionRejected,
            const TQueryId& /* aRequestId */,
            TableProcessorRejectType /* aRejectType */)
    };
};

}
