#pragma once

#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief State Mashine для логики локального кэширования данных.
 * \ingroup NewUiServer
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

struct ILocalStoreStateMachine
{
    enum class State
    {
        NotReady,       ///< Нет обновлений, нет задач для обработки.
        Idle,           ///< Нет обновлений, нет задач для обработки.
        Processing,     ///< Обработка запросов на основе кэша. Запрос требует обработки, если он не находится в состоянии Ok
        Updating        ///< Обновление кэша. Запросы не обрабатываются.
    };

    enum class Event
    {
        UpdatesReceived,        ///< Получены обновления.
        AllReadyToProcessing,   ///< Все подписки готовы к обработке.
        AllProcessed,           ///< Все подписки обработаны.
        NewRequestReceived,     ///< Получена новая подписка.
        ApiDisconnected         ///< Api disconnected.
    };

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Machine : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        CONST_API_METHOD_RETURN(State, GetState)

        API_METHOD(ChangeState, Event /* aEvent */)
    };
};

inline std::ostream& operator<<(std::ostream& out, ILocalStoreStateMachine::State value)
{
    switch (value)
    {
    case ILocalStoreStateMachine::State::NotReady:
        return out << "NotReady";
    case ILocalStoreStateMachine::State::Idle:
        return out << "Idle";
    case ILocalStoreStateMachine::State::Processing:
        return out << "Processing";
    case ILocalStoreStateMachine::State::Updating:
        return out << "Updating";
    default :
        return out << "???";
    }
}

inline std::ostream& operator<<(std::ostream& out, ILocalStoreStateMachine::Event value)
{
    switch (value)
    {
    case ILocalStoreStateMachine::Event::UpdatesReceived:
        return out << "UpdatesReceived";
    case ILocalStoreStateMachine::Event::AllReadyToProcessing:
        return out << "AllReadyToProcessing";
    case ILocalStoreStateMachine::Event::AllProcessed:
        return out << "AllProcessed";
    case ILocalStoreStateMachine::Event::NewRequestReceived:
        return out << "NewRequestReceived";
    case ILocalStoreStateMachine::Event::ApiDisconnected:
        return out << "ApiDisconnected";
    default :
        return out << "???";
    }
}


}
