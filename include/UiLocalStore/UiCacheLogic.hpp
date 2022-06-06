#pragma once

#include <boost/multi_index/hashed_index.hpp>

#include "UiSession.hpp"

#include "UiLocalStore/ILocalStoreLogic.hpp"
#include "UiLocalStore/ILocalStoreStateMachine.hpp"
#include "UiLocalStore/ITableProcessorApi.hpp"
#include "UiLocalStore/VersionedDataContainer.hpp"
#include "UiLocalStore/ISubscriptionActor.hpp"
#include "UiLocalStore/ISubscriptionsContainer.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Логика локального кэширования данных.
 * \ingroup NewUiServer
 * Хранит данные одного типа в удобном для их фильтрации и сортировки виде.
 * Обеспечивает справедливое распределение мощности реактора для нескольких подписок.
 *
 * Применение обновлений:
 * - Обработка запросов прерывается в момент прихода обновлений.
 * - Увеличивается версия хранилища данных.
 * - Обновление сохраняется с новой версией.
 * - Обработка прерванных запросов продолжается, при этом они используют данные из предыдущего хранилища.
 *
 * State Mashine:
 * Логика может находиться в одном из следующих состояний:
 * - NotReady. Изначально логика находится в состоянии NotReady.
 *      -- В этом состоянии не должны вызываться реколы,
 *      -- Если логика получает подписку, то она отклоняется.
 *      -- Хранилище данных пусто.
 * - NotReady --> Updating. При получении первого пакета обновлений он записывается в хранилище с новой версией.
 * - Updating. В этом состоянии логика должна сообщить всем подпискам о том, что они должны обновиться.
 *      -- Все подписки в статусе Ok переводятся в статус Updating. При этом у них повышается версия.
 *      -- Подписки в других статусах не обрабатываются.
 *      -- Если логика получает подписку, то она сохраняется со статусом Initializing.
 *      -- Если логика получает новый пакет обновлений, то он записывается в контейнер с текущей версией.
 * - Updating -> Processing. Eсли не осталось подписок в статусе Ok, переходим в Processing.
 * - Processing. В данном состоянии происходит выполнение задачи по подписке.
 *      Для определения задачи используется статус подписки, для определения исходных данных - версия подписки.
 *      -- Обрабатываем подписки в статусе Initializing, Updating, ReadyForSending.
 *      -- Сначала обрабатываем одну подписку в Initializing или Updating.
 *      -- Номер последней обработанной подписки сохраняется в конце каждой итерации.
 *      -- На новой итерации обработка начинается со следующего порядкового номера.
 *      -- После того, как все подписки обработаны, обработка начинается сначала.
 *      -- Если подписка обработана полностью, она переводится в состояние ReadyForSending.
 *      -- Если вконце итерации есть подписки в состоянии ReadyForSending они отправляются клиенту.
 *      -- Отправленная подписка переводится в статус OK, если её версия равна текущей.
 *      -- Подписка переводится в Updating с повышением версии, если её версия меньше максимальной.
 *      -- Если логика получает подписку, то она сохраняется со статусом Initializing.
 * - Processing -> Updating. При получении пакета обновлений он записывается в хранилище с новой версией.
 * - Processing -> Idle. Если все задачи в статусе OK, то переходим в Idle.
 * - Idle.
 *      -- В этом состоянии не должны вызываться реколы.
 *      -- Если логика получает подписку, то она сохраняется со статусом Initializing.
 * - Idle -> Updating. При получении пакета обновлений он записывается в хранилище с новой версией.
 * - Idle -> Processing. При получении подписки переходим в состояние Processing.
 * - Idle, Processing, Updating -> NotReady. При дисконнекте. Очищаем хранилище. Реджектим подписки.
 */
template <typename TSetup>
class UiCacheLogic
{
public:
    using TData = typename TSetup::TData;
    using TMap = typename TSetup::TMap;

    using TLocalStoreLogicInterface = ILocalStoreLogic<TData>;
    using TUiSubscription = typename TLocalStoreLogicInterface::TUiSubscription;
    using TPendingUpdate = typename TLocalStoreLogicInterface::TPendingUpdate;
    using TSubscriptionId = TradingSerialization::Table::SubscribeBase::TId;

    using TState = ILocalStoreStateMachine::State;
    using TEvent = ILocalStoreStateMachine::Event;

    ILocalStoreStateMachine::Machine<typename TSetup::TLocalStoreStateMachine> StateMachine;
    ISubscriptionsContainer::Logic<typename TSetup::TSubscriptionsContainer> Subscriptions;
    VersionedDataContainer<TSetup> Data;

private:
    Basis::Tracer& mTracer;

public:
    UiCacheLogic(Basis::Tracer& aTracer)
        : StateMachine(aTracer)
        , Subscriptions(aTracer)
        , Data(aTracer)
        , mTracer(aTracer)
    {
    }

    bool IsReady() const
    {
        return (StateMachine.GetState() != TState::NotReady);
    }

    std::optional<TableProcessorRejectType> ProcessSubscription(
        const TSubscriptionId& aRequestId,
        const TUiSubscription& aSubscription)
    {
        if (!IsReady())
        {
            mTracer.Info("Logic not ready");
            return TableProcessorRejectType::Disconnected;
        }

        if (!Subscriptions.EmplaceSubscription(
            Basis::MakeShared<ISubscriptionActor::Logic<typename TSetup::TSubscriptionActor>>(
                mTracer,
                aRequestId,
                aSubscription,
                Data.GetData(),
                Data.GetCurrentVersion())))
        {
            mTracer.Error("Request id duplicated, reject");
            return TableProcessorRejectType::WrongSubscription;
        }
        StateMachine.ChangeState(TEvent::NewRequestReceived);
        return std::nullopt;
    }

    void ProcessUnsubscription(const TSubscriptionId& aRequestId)
    {
        Subscriptions.EraseSubscription(aRequestId);
    }

    Basis::Vector<TSubscriptionId> Clear()
    {
        StateMachine.ChangeState(TEvent::ApiDisconnected);
        Data.Clear();
        return Subscriptions.Clear();
    }

    Basis::Vector<TSubscriptionId> GetRejectedSubscriptions()
    {
        return Subscriptions.GetRejectedSubscriptions();
    }

    template <typename TPack>
    void ProcessDataUpdate(const Basis::SPtr<TPack>& aPack)
    {
        Data.UpdateAllData(aPack);
        ProcessIncomingUpdates();
    }

    bool IsRecallNeeded() const
    {
        const auto state = StateMachine.GetState();
        return state == TState::Processing
            || state == TState::Updating
            || Data.HasPendingIncomingData();
    }

    TPendingUpdate ProcessDefferedTasks()
    {
        mTracer.Trace("ProcessDefferedTasks");

        // Сначала применяем очередную пачку входящих обновлений
        if (ProcessIncomingUpdates())
        {
            return TPendingUpdate {};
        }

        // Затем обрабатываем подписки
        auto version = Data.GetCurrentVersion();
        switch(StateMachine.GetState())
        {
        case TState::Updating:
            if (Subscriptions.UpdateSubscriptions(version))
            {
                ClearOldVersions();
                StateMachine.ChangeState(TEvent::AllReadyToProcessing);
            }
            break;
        case TState::Processing:
        {
            auto result = Subscriptions.template ProcessNextSubscription<TData>(version);
            if (result.Processed)
            {
                if (result.IsOk())
                {
                    return result;
                }
            }
            else
            {
                StateMachine.ChangeState(TEvent::AllProcessed);
            }
            break;
        }
        default:
            break;
        }
        return TPendingUpdate {};
    }

    TPendingUpdate ProcessGetNext(const TSubscriptionId& aRequestId)
    {
        if (StateMachine.GetState() == TState::NotReady)
        {
            mTracer.Error("Logic not ready");
            return TPendingUpdate {};
        }

        auto version = Data.GetCurrentVersion();
        auto result = Subscriptions.template ProcessGetNext<TData>(aRequestId, version);
        if (result.Processed)
        {
            StateMachine.ChangeState(TEvent::NewRequestReceived);
        }
        if (result.IsOk())
        {
            return result;
        }
        return TPendingUpdate {};
    }

private:
    void ClearOldVersions()
    {
        if (auto version = Subscriptions.GetOldestVersion())
        {
            Data.ClearOldVersions(*version);
        }
        else
        {
            Data.ClearOldVersions(Data.GetCurrentVersion());
        }
    }

    bool ProcessIncomingUpdates()
    {
        if (Data.ProcessIncomingQueue())
        {
            StateMachine.ChangeState(TEvent::UpdatesReceived);
            return true;
        }
        else
        {
            return false;
        }
    }
};
}
