#pragma once

#include "UiLocalStore/IDataItemBuilder.hpp"
#include "UiLocalStore/LocalStoreUtils.hpp"

namespace NTPro::Ecn::NewUiServer
{

using TDataVersion = int64_t;

//struct SharedVersionedObject
//{
//    Basis::Vector<TDataVersion> Versions;
//};

template<typename TDataItem>
struct VersionedItem
{
    Basis::SPtr<TDataItem> Item;
    TDataVersion Version;

    /**
     * 1. Если не заполнен, то это элемент самой последней версии.
     * 2. Если заполнен версией, и в этой версии есть элемент с таким же id,
     * то значит данный элемент был перезаписан версией выше.
     * 3. Если заполнен версия, но в этой версии нет элемента с таким же id,
     * то значит данный элемент был удален.
     */
    std::optional<TDataVersion> IsRewritedBy;

    VersionedItem(
        const Basis::SPtr<TDataItem>& aItem,
        TDataVersion aVersion,
        const std::optional<TDataVersion>& aIsRewritedBy = std::nullopt)
        : Item(aItem)
        , Version(aVersion)
        , IsRewritedBy(aIsRewritedBy)
    {
    }
};

/**
 * \brief Хранилище данных.
 * \ingroup NewUiServer
 * Хранилище версионируется.
 * Старые версии должны удаляться, если они никому больше не нужны.
 * Новые данные применяются небольшими порциями.
 */
template <typename TSetup>
class VersionedDataContainer
{
    using TData = typename TSetup::TData;
    using TMap = typename TSetup::TMap;
    using TIncomingPack = typename TSetup::TQueryApiPack;
    using TIncomingPackIt = typename TIncomingPack::const_iterator;

    static constexpr size_t MaxIncomingChunkSize = 100;
    
private:
    struct IncomingPackCtx
    {
        Basis::SPtr<TIncomingPack> Pack;
        TIncomingPackIt It;
    };
    
    TDataVersion mCurrentVersion;
    TMap mData;
    Basis::Deque<IncomingPackCtx> mIncomingQueue;

    Basis::Tracer& mTracer;
    
public:
    typename IDataItemBuilder<TData>::template Logic<
        typename TSetup::TDataItemBuilder,
        Basis::Bind> ItemBuilder;

    VersionedDataContainer(Basis::Tracer& aTracer)
        : mCurrentVersion(0)
        , mData(aTracer)
        , mTracer(aTracer)
    {
    }

    void UpdateAllData(const Basis::SPtr<TIncomingPack>& aPack)
    {
        mIncomingQueue.push_back(IncomingPackCtx { aPack, aPack->cbegin() });
        mTracer.InfoSlow("UpdateAllData: internal queue size:", mIncomingQueue.size());
    }

    bool HasPendingIncomingData() const
    {
        return !mIncomingQueue.empty();
    }

    bool ProcessIncomingQueue()
    {
        if (mIncomingQueue.empty())
        {
            return false;
        }
        auto nextVersion = mCurrentVersion + 1;

        auto& ctx = mIncomingQueue.front();
        for (
            size_t counter = 0;
            ctx.It != ctx.Pack->cend() && counter < MaxIncomingChunkSize;
            ++counter, ++ctx.It)
        {
            const auto& modelData = *ctx.It;
            auto item = ItemBuilder.CreateItem(modelData);
            if (!item.HasValue())
            {
                continue;
            }
            switch (modelData->Action)
            {
            case Model::ActionType::New:
            case Model::ActionType::Change:
            {
                mData.Emplace(item, nextVersion);
                break;
            }
            case Model::ActionType::Delete:
                mData.Erase(item->GetId(), nextVersion);
                break;
            }
        }

        if (ctx.It == ctx.Pack->cend())
        {
            mCurrentVersion = nextVersion;
            mTracer.InfoSlow(
                "ProcessIncomingQueue: version:", mCurrentVersion,
                ", pack size: ", ctx.Pack->size(),
                ", cache size: ", mData.Size());

            if (mCurrentVersion == 1)
            {
                mData.ProcessInitialPack();
            }
            
            mIncomingQueue.pop_front();
            return true;
        }
        else
        {
            return false;
        }
    }

    TDataVersion GetCurrentVersion() const
    {
        return mCurrentVersion;
    }
    const TMap& GetData() const
    {
        return mData;
    }

    void ClearOldVersions(TDataVersion aVersion)
    {
        mData.ErasePrevious(aVersion);
        mTracer.InfoSlow(
            "ClearOldVersions: version:", aVersion,
            ", current version: ", mCurrentVersion,
            ", size: ", mData.Size());
    }

    void Clear()
    {
        mCurrentVersion = 0;
        mData.Clear();
    }
};
}
