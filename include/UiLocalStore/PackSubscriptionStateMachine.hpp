#pragma once

#include "UiLocalStore/ISubscriptionStateMachine.hpp"

#include "Common/Tracer.hpp"

namespace NTPro::Ecn::NewUiServer
{

class PackSubscriptionStateMachineLogic
{
public:
    using TState = ISubscriptionStateMachine::PackState;
    using TEvent = ISubscriptionStateMachine::PackEvent;

private:

    TState mState = TState::Initializing;
    std::optional<TState> mPreviousState;

public:
    TState GetState() const;
    /// Подписка в процессе формировани, нельзя вызывать update.
    ISubscriptionStateMachine::ProcessingState GetProcessingState() const;

    bool ChangeStateInternal(TEvent aEvent);

private:
    bool ProcessInitializingState(TEvent aEvent);
    bool ProcessFiltrationState(TEvent aEvent);
    bool ProcessUpdatingState(TEvent aEvent);
    bool ProcessDeletedFiltrationState(TEvent aEvent);
    bool ProcessAddedFiltrationState(TEvent aEvent);
    bool ProcessOkState(TEvent aEvent);
    bool ProcessPendingResult(TEvent aEvent);
    bool ProcessFinalPendingResult(TEvent aEvent);
    bool ProcessErrorState(TEvent aEvent);
};

}
