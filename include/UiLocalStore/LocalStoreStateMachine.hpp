#pragma once

#include "UiLocalStore/ILocalStoreStateMachine.hpp"

#include "Common/Tracer.hpp"

namespace NTPro::Ecn::NewUiServer
{

class LocalStoreStateMachineLogic
{
public:
    using TState = ILocalStoreStateMachine::State;
    using TEvent = ILocalStoreStateMachine::Event;

private:
    TState mState = TState::NotReady;

public:

    TState GetState() const;

    bool ChangeStateInternal(TEvent aEvent);

private:
    bool ProcessNotReadyState(TEvent aEvent);
    bool ProcessIdleState(TEvent aEvent);
    bool ProcessProcessingState(TEvent aEvent);
    bool ProcessUpdatingState(TEvent aEvent);
};

class LocalStoreStateMachine : public LocalStoreStateMachineLogic
{
    Basis::Tracer& mTracer;
public:
    LocalStoreStateMachine(Basis::Tracer& aTracer);

    void ChangeState(TEvent aEvent);
};

}
