#pragma once

#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Состояние подписки.
 * \ingroup NewUiServer
 * Правильные переходы:
 * - Initializing -> Ok. Данные сформированы
 * - Ok -> Updating. Получены обновления
 * - Updating -> Ok. Данные сформированы
 */
struct ISubscriptionStateMachine
{
    enum class TableState
    {
        Initializing,       ///< Подписка инициализируется.
        Filtration,         ///< Фильтрация для первого снапшота даннных.
        Sorting,            ///< Сортировка первого снапшота данных.
        Updating,           ///< Подписка обновляется.
        IncrementMaking,    ///< Вычисление инкремента.
        IncrementApplying,  ///< Применение инкремента.
        Ok,                 ///< Подписка не требует никаких операций.
        Error,              ///< Предыдущая операция завершилась ошибкой.
    };

    enum class TableEvent
    {
        Initialized,        ///< Подписка готова к обработке.
        FiltrationCompleted,///< Фильтрация завершена.
        SortingCompleted,   ///< Сортировка завершена.
        UpdatesReceived,    ///< Обновления получены.
        RawDataUpdated,     ///< Подписка готова к обработке после обновления.
        IncrementMade,      ///< Инкремент создан.
        IncrementApplied,   ///< Инкремент применен.
        ErrorOccured,       ///< Произошла ошибка.
    };

    enum class PackState
    {
        Initializing,       ///< Подписка инициализируется.
        Filtration,         ///< Фильтрация для первого снапшота даннных.
        Updating,           ///< Подписка обновляется.
        DeletedFiltration,  ///< Фильтрация инкрементально удаленных данных.
        AddedFiltration,    ///< Фильтрация инкрементально добавленных данных.
        PendingResult,      ///< Подписка ожидает запроса на очередной пакет.
        FinalPendingResult, ///< Подписка ожидает разрешения завершиться.
        Ok,                 ///< Подписка не требует никаких операций.
        Error,              ///< Предыдущая операция завершилась ошибкой.
    };

    enum class PackEvent
    {
        Initialized,                            ///< Подписка готова к обработке.
        FiltrationCompleted,                    ///< Фильтрация завершена.
        UpdatesReceived,                        ///< Обновления получены.
        RawDataUpdated,                         ///< Подписка готова к обработке после обновления.
        DeletedFiltrationCompleted,             ///< Фильтрация удаленных данных завершена.
        DeletedFiltrationCompletedWithResult,   ///< Фильтрация удаленных данных завершена, cформирован результат..
        AddedFiltrationCompleted,               ///< Фильтрация добавленных данных завершена.
        AddedFiltrationCompletedWithResult,     ///< Фильтрация добавленных данных завершена, cформирован результат.
        PendingResultCompleted,                 ///< Фильтрация одной пачки данных заверешена.
        GetNextReceived,                        ///< Запрос на следующую пачку данных.
        ErrorOccured,                           ///< Произошла ошибка.
    };

    /// Порядок элементов важен, так как эти значения используются в упорядоченных индексах.
    /// @see SubscriptionsContainer ByIsProcessing
    enum ProcessingState
    {
        Idle,       ///< Подписка полностью обработана и ожидает обновлений.
        Pending,    ///< Сформирован промежуточный результат. Подписка ожидает запроса следующего результата
        Processing  ///< Подписка обрабатывается.
    };


    template<typename TState, typename TEvent, typename TImpl, typename TOwnership = Basis::Own>
    struct Machine : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        CONST_API_METHOD_RETURN(TState, GetState)
        CONST_API_METHOD_RETURN(ISubscriptionStateMachine::ProcessingState, GetProcessingState)

        API_METHOD(ChangeState, TEvent /* aEvent */)
        API_METHOD(ReportError, const std::string& /* aError */)
    };
};

inline std::ostream& operator<<(std::ostream& out, ISubscriptionStateMachine::TableState value)
{
    switch (value)
    {
    case ISubscriptionStateMachine::TableState::Initializing:
        return out << "Initializing";
    case ISubscriptionStateMachine::TableState::Filtration:
        return out << "Filtration";
    case ISubscriptionStateMachine::TableState::Sorting:
        return out << "Sorting";
    case ISubscriptionStateMachine::TableState::Updating:
        return out << "Updating";
    case ISubscriptionStateMachine::TableState::IncrementMaking:
        return out << "IncrementMaking";
    case ISubscriptionStateMachine::TableState::IncrementApplying:
        return out << "IncrementApplying";
    case ISubscriptionStateMachine::TableState::Ok:
        return out << "Ok";
    case ISubscriptionStateMachine::TableState::Error:
        return out << "Error";
    default:
        return out << "???";
    }
}

inline std::ostream& operator<<(std::ostream& out, ISubscriptionStateMachine::PackState value)
{
    switch (value)
    {
    case ISubscriptionStateMachine::PackState::Initializing:
        return out << "Initializing";
    case ISubscriptionStateMachine::PackState::Filtration:
        return out << "Filtration";
    case ISubscriptionStateMachine::PackState::Updating:
        return out << "Updating";
    case ISubscriptionStateMachine::PackState::DeletedFiltration:
        return out << "DeletedFiltration";
    case ISubscriptionStateMachine::PackState::AddedFiltration:
        return out << "AddedFiltration";
    case ISubscriptionStateMachine::PackState::Ok:
        return out << "Ok";
    case ISubscriptionStateMachine::PackState::PendingResult:
        return out << "PendingResult";
    case ISubscriptionStateMachine::PackState::FinalPendingResult:
        return out << "FinalPendingResult";
    case ISubscriptionStateMachine::PackState::Error:
        return out << "Error";
    default:
        return out << "???";
    }
}

inline std::ostream& operator<<(std::ostream& out, ISubscriptionStateMachine::TableEvent value)
{
    switch (value)
    {
    case ISubscriptionStateMachine::TableEvent::Initialized:
        return out << "Initialized";
    case ISubscriptionStateMachine::TableEvent::FiltrationCompleted:
        return out << "FiltrationCompleted";
    case ISubscriptionStateMachine::TableEvent::SortingCompleted:
        return out << "SortingCompleted";
    case ISubscriptionStateMachine::TableEvent::UpdatesReceived:
        return out << "UpdatesReceived";
    case ISubscriptionStateMachine::TableEvent::RawDataUpdated:
        return out << "RawDataUpdated";
    case ISubscriptionStateMachine::TableEvent::IncrementMade:
        return out << "IncrementMade";
    case ISubscriptionStateMachine::TableEvent::IncrementApplied:
        return out << "IncrementApplied";
    case ISubscriptionStateMachine::TableEvent::ErrorOccured:
        return out << "ErrorOccured";
    default:
        return out << "???";
    }
}

inline std::ostream& operator<<(std::ostream& out, ISubscriptionStateMachine::PackEvent value)
{
    switch (value)
    {
    case ISubscriptionStateMachine::PackEvent::Initialized:
        return out << "Initialized";
    case ISubscriptionStateMachine::PackEvent::FiltrationCompleted:
        return out << "FiltrationCompleted";
    case ISubscriptionStateMachine::PackEvent::UpdatesReceived:
        return out << "UpdatesReceived";
    case ISubscriptionStateMachine::PackEvent::RawDataUpdated:
        return out << "RawDataUpdated";
    case ISubscriptionStateMachine::PackEvent::DeletedFiltrationCompleted:
        return out << "DeletedFiltrationCompleted";
    case ISubscriptionStateMachine::PackEvent::DeletedFiltrationCompletedWithResult:
        return out << "DeletedFiltrationCompletedWithResult";
    case ISubscriptionStateMachine::PackEvent::AddedFiltrationCompleted:
        return out << "AddedFiltrationCompleted";
    case ISubscriptionStateMachine::PackEvent::AddedFiltrationCompletedWithResult:
        return out << "AddedFiltrationCompletedWithResult";
    case ISubscriptionStateMachine::PackEvent::ErrorOccured:
        return out << "ErrorOccured";
    case ISubscriptionStateMachine::PackEvent::PendingResultCompleted:
        return out << "PendingResultCompleted";
    case ISubscriptionStateMachine::PackEvent::GetNextReceived:
        return out << "GetNextReceived";
    default:
        return out << "???";
    }
}


}
