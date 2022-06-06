#pragma once

#include "UiLocalStore/ISubscriptionStateMachine.hpp"

#include "Common/Tracer.hpp"

namespace NTPro::Ecn::NewUiServer
{

class TableSubscriptionStateMachineLogic
{
public:
    using TState = ISubscriptionStateMachine::TableState;
    using TEvent = ISubscriptionStateMachine::TableEvent;

private:

    TState mState = TState::Initializing;

public:
    TState GetState() const;
    ISubscriptionStateMachine::ProcessingState GetProcessingState() const;

    bool ChangeStateInternal(TEvent aEvent);

private:
    bool ProcessInitializingState(TEvent aEvent);
    bool ProcessFiltrationState(TEvent aEvent);
    bool ProcessSortingState(TEvent aEvent);
    bool ProcessUpdatingState(TEvent aEvent);
    bool ProcessIncrementMakingState(TEvent aEvent);
    bool ProcessIncrementApplyingState(TEvent aEvent);
    bool ProcessOkState(TEvent aEvent);
    bool ProcessErrorState(TEvent aEvent);
};

}
