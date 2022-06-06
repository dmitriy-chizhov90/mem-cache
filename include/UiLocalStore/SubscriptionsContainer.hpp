#pragma once

#include <boost/multi_index/hashed_index.hpp>

#include "UiLocalStore/ISubscriptionActor.hpp"
#include "UiLocalStore/ISubscriptionStateMachine.hpp"
#include "UiLocalStore/ISubscriptionsContainer.hpp"
#include "UiLocalStore/VersionedDataContainer.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Контейнер подписок.
 * \ingroup NewUiServer
 * Обеспечивает поочередную обработку всех активных подписок.
 */
template <typename TSetup>
class SubscriptionsContainer
{
public:
    static constexpr size_t MaxUpdatedSubscriptions = TSetup::MaxUpdatedSubscriptions;

    using TSubscriptionActorImpl = typename TSetup::TSubscriptionActor;
    using TSubscriptionActor = ISubscriptionActor::Logic<TSubscriptionActorImpl>;
    using TSubscriptionPtr = std::shared_ptr<TSubscriptionActor>;
    using TSubscriptionId = ISubscriptionsContainer::TSubscriptionId;

private:
    Basis::Tracer& mTracer;

    struct SubscriptionInfo
    {
        SubscriptionInfo() = default;
        SubscriptionInfo(
            const TSubscriptionPtr& aSubscription,
            size_t aSubscriptionNumber)
            : Subscription(aSubscription)
            , SubscriptionNumber(aSubscriptionNumber)
        {}

        TSubscriptionPtr Subscription;
        size_t SubscriptionNumber = 0;
        mutable bool WaitNextPacket = true;
    };

    struct SubscriptionsContainerType
    {
        struct ByRequestId
        {
            typedef TSubscriptionId result_type;
            const result_type& operator()(const SubscriptionInfo& aData) const
            {
                return aData.Subscription->Get().GetRequestId();
            }
        };

        struct BySubscriptionNumber
        {
            typedef size_t result_type;
            result_type operator()(const SubscriptionInfo& aData) const
            {
                return aData.SubscriptionNumber;
            }
        };

        struct ByIsProcessing
        {
            typedef ISubscriptionStateMachine::ProcessingState result_type;
            result_type operator()(const SubscriptionInfo& aData) const
            {
                return aData.Subscription->Get().GetProcessingState();
            }
        };

        struct ByVersion
        {
            typedef TDataVersion result_type;
            result_type operator()(const SubscriptionInfo aData) const
            {
                return aData.Subscription->Get().GetVersion();
            }
        };

        struct ByIsProcessingAndSubscription {};

        using Type = Basis::MultiIndex
            <
                SubscriptionInfo,
                boost::multi_index::indexed_by
                <
                    boost::multi_index::hashed_unique<
                        boost::multi_index::tag<ByRequestId>,
                        ByRequestId,
                        Basis::UniqueIdHasher<TSubscriptionId>
                    >,

                    boost::multi_index::ordered_unique<
                        boost::multi_index::tag<ByIsProcessingAndSubscription>,
                        boost::multi_index::composite_key<
                            SubscriptionInfo,
                            ByIsProcessing,
                            BySubscriptionNumber
                        >
                    >,

                    boost::multi_index::ordered_non_unique<
                        boost::multi_index::tag<ByVersion>,
                        ByVersion
                    >
                >
            >;
    };

    using TSubscriptionsContainer = typename SubscriptionsContainerType::Type;

    TSubscriptionsContainer mSubscriptions;

    Basis::Vector<TSubscriptionId> mRejectedSubscriptions;

    /// Счетчик новых подписок. Каждой новой подписке присваивается порядковый номер.
    size_t mSubscriptionsCounter = 0;
    /// Номер последней обработанной подписки.
    /// Подписки вызываются в порядке создания.
    /// Здесь сохраняется индекс последней обработанной подписки.
    size_t mLastProcessedSubscriptionNumber = 0;

public:
    SubscriptionsContainer(Basis::Tracer& aTracer)
        : mTracer(aTracer)
    {
    }

    bool EmplaceSubscription(const TSubscriptionPtr& aSubscription)
    {
        if (!mSubscriptions.emplace(aSubscription, ++mSubscriptionsCounter).second)
        {
            mTracer.Error("Request id duplicated, reject");
            return false;
        }

        mTracer.InfoSlow("EmplaceSubscription: size:", mSubscriptions.size());
        return true;
    }

    void EraseSubscription(const TSubscriptionId& aRequestId)
    {
        mSubscriptions.erase(aRequestId);
        mTracer.InfoSlow("EraseSubscription: size:", mSubscriptions.size());
    }

    Basis::Vector<TSubscriptionId> Clear()
    {
        Basis::Vector<TSubscriptionId> result;
        result.reserve(mSubscriptions.size());
        std::transform(
            mSubscriptions.cbegin(),
            mSubscriptions.cend(),
            std::back_inserter(result),
            [](const SubscriptionInfo& aInfo) { return aInfo.Subscription->Get().GetRequestId(); });

        mSubscriptions.clear();

        return result;
    }

    Basis::Vector<TSubscriptionId> GetRejectedSubscriptions()
    {
        Basis::Vector<TSubscriptionId> result;
        result.swap(mRejectedSubscriptions);
        return result;
    }

    /// Возвращает true, если все что нужно обновлено.
    bool UpdateSubscriptions(TDataVersion aCurrentVersion)
    {
        auto& index = mSubscriptions.template get<
            typename SubscriptionsContainerType::ByIsProcessingAndSubscription>();
        /// Получаем первую подписку, которая не обрабатывается.
        auto it = index.lower_bound(std::make_tuple(ISubscriptionStateMachine::ProcessingState::Idle));
        Basis::Vector<decltype(it)> iterators;
        while (it != index.end()
            && it->Subscription->Get().IsOk()
            && iterators.size() < MaxUpdatedSubscriptions)
        {
            iterators.push_back(it);
            ++it;
        }
        for (auto mutableIt : iterators)
        {
            index.modify(
                mutableIt,
                [=](SubscriptionInfo& outInfo) { UpdateSubscriptionData(outInfo, aCurrentVersion); });
            if (mutableIt->Subscription->Get().IsError())
            {
                assert(false);
                mTracer.ErrorSlow("Updating error:", mutableIt->Subscription->Get().GetRequestId());
                mRejectedSubscriptions.push_back(mutableIt->Subscription->Get().GetRequestId());
                index.erase(mutableIt);
            }
        }

        return iterators.size() < MaxUpdatedSubscriptions;
    }

    /// Возвращает false, если нет подписок, требующих обработки.
    template <typename TData>
    ISubscriptionsContainer::ProcessingResult<TData> ProcessNextSubscription(TDataVersion aCurrentVersion)
    {
        ISubscriptionsContainer::ProcessingResult<TData> result;

        auto& index = mSubscriptions.template get<
            typename SubscriptionsContainerType::ByIsProcessingAndSubscription>();
        /// Получаем следующую подписку, которая обрабатывается.
        auto it = index.lower_bound(std::make_tuple(
            ISubscriptionStateMachine::ProcessingState::Processing,
            ++mLastProcessedSubscriptionNumber));
        if (it == index.end())
        {
            mLastProcessedSubscriptionNumber = 0;
            it = index.lower_bound(std::make_tuple(
                ISubscriptionStateMachine::ProcessingState::Processing,
                mLastProcessedSubscriptionNumber));
            if (it == index.end())
            {
                return result;
            }
        }

        assert(it->Subscription->Get().GetProcessingState() == ISubscriptionStateMachine::ProcessingState::Processing);
        mLastProcessedSubscriptionNumber = it->SubscriptionNumber;
        result.Processed = true;

        index.modify(it, [&](SubscriptionInfo& outInfo)
        {
            TSubscriptionActor& subscription = *outInfo.Subscription;
            subscription.Process();
            if (subscription.Get().IsOk())
            {
                mTracer.InfoSlow(
                    "Subscription completed:", subscription.Get().GetRequestId());
                TryFillResult(outInfo, true, result, aCurrentVersion);
                UpdateSubscriptionData(outInfo, aCurrentVersion);
            }
            else if (subscription.Get().IsPending())
            {
                mTracer.TraceSlow(
                    "Subscription pending:", subscription.Get().GetRequestId());
                TryFillResult(outInfo, false, result, aCurrentVersion);
            }

        });
        if (it->Subscription->Get().IsError())
        {
            mTracer.ErrorSlow("Processing error:", it->Subscription->Get().GetRequestId());
            mRejectedSubscriptions.push_back(it->Subscription->Get().GetRequestId());
            index.erase(it);
        }

        return result;
    }

    template <typename TData>
    ISubscriptionsContainer::ProcessingResult<TData> ProcessGetNext(
        const TSubscriptionId& aRequestId,
        TDataVersion aCurrentVersion)
    {
        ISubscriptionsContainer::ProcessingResult<TData> result;

        auto& index = mSubscriptions.template get<typename SubscriptionsContainerType::ByRequestId>();
        auto it = index.find(aRequestId);
        if (it == index.cend())
        {
            mTracer.InfoSlow("ProcessGetNext: subscription not found", aRequestId);
            return result;
        }

        assert(!it->WaitNextPacket);
        it->WaitNextPacket = true;

        auto state = it->Subscription->Get().GetProcessingState();
        if (state != ISubscriptionStateMachine::ProcessingState::Processing
            && state != ISubscriptionStateMachine::ProcessingState::Pending)
        {
            mTracer.DebugSlow("ProcessGetNext: subscription is not pending", aRequestId);
            return result;
        }
        result.Processed = true;

        if (state == ISubscriptionStateMachine::ProcessingState::Pending)
        {
            index.modify(it, [&](SubscriptionInfo& outInfo)
            {
                TryFillResult(outInfo, false, result, aCurrentVersion);
            });
        }

        return result;
    }

    std::optional<TDataVersion> GetOldestVersion() const
    {
        if (mSubscriptions.empty())
        {
            return std::nullopt;
        }
        else
        {
            auto& index = mSubscriptions.template get<typename SubscriptionsContainerType::ByVersion>();
            return index.begin()->Subscription->Get().GetVersion();
        }
    }

private:
    void UpdateSubscriptionData(SubscriptionInfo& outSubscription, TDataVersion aCurrentVersion)
    {
        TSubscriptionActor& subscription = *outSubscription.Subscription;
        const auto version = subscription.Get().GetVersion();
        assert(version <= aCurrentVersion);

        const auto nextVersion = version + 1;
        if (nextVersion <= aCurrentVersion)
        {
            if constexpr (TSetup::StoreType == SubscriptionType::Table)
            {
                outSubscription.WaitNextPacket = true;
            }
            subscription.IncreaseVersion();
        }
    }

    void CallGetNext(
        SubscriptionInfo& outInfo,
        TDataVersion aCurrentVersion)
    {
        TSubscriptionActor& subscription = *outInfo.Subscription;
        subscription.ProcessGetNext();
        if (subscription.Get().IsOk())
        {
            mTracer.InfoSlow(
                "Subscription completed after get next:", subscription.Get().GetRequestId());
            UpdateSubscriptionData(outInfo, aCurrentVersion);
        }
    }

    template <typename TData>
    void TryFillResult(
        SubscriptionInfo& outInfo,
        bool aSubscriptionProcessed,
        ISubscriptionsContainer::ProcessingResult<TData>& outResult,
        TDataVersion aCurrentVersion)
    {
        TSubscriptionActor& subscription = *outInfo.Subscription;
        outResult.ResultState = subscription.Get().GetResultState();
        if (outInfo.WaitNextPacket
            && outResult.ResultState != ISubscriptionActor::ResultState::NoResult)
        {
            outResult.SubscriptionId = subscription.Get().GetRequestId();
            outResult.Result = subscription.Get().GetResult();
            outResult.DeletedIds = subscription.Get().GetDeletedIds();
            outInfo.WaitNextPacket = false;

            if (!aSubscriptionProcessed)
            {
                CallGetNext(outInfo, aCurrentVersion);
            }
        }
    }

};
}
