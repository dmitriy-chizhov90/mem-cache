#include "UiLocalStore/LocalStoreStateMachine.hpp"

namespace NTPro::Ecn::NewUiServer
{

LocalStoreStateMachineLogic::TState LocalStoreStateMachineLogic::GetState() const
{
    return mState;
}

bool LocalStoreStateMachineLogic::ChangeStateInternal(TEvent aEvent)
{
    switch (mState)
    {
    case TState::NotReady:
        return ProcessNotReadyState(aEvent);
    case TState::Idle:
        return ProcessIdleState(aEvent);
    case TState::Processing:
        return ProcessProcessingState(aEvent);
    case TState::Updating:
        return ProcessUpdatingState(aEvent);
    }
    return false;
}

bool LocalStoreStateMachineLogic::ProcessNotReadyState(TEvent aEvent)
{
    assert(mState == TState::NotReady);
    switch (aEvent)
    {
    case TEvent::UpdatesReceived:
        /// NotReady --> Updating
        mState = TState::Updating;
        return true;
    case TEvent::NewRequestReceived:
    case TEvent::ApiDisconnected:
        /// NotReady
        return true;
    case TEvent::AllReadyToProcessing:
    case TEvent::AllProcessed:
        /// Error
        break;
    }
    return false;
}

bool LocalStoreStateMachineLogic::ProcessIdleState(TEvent aEvent)
{
    assert(mState == TState::Idle);
    switch (aEvent)
    {
    case TEvent::UpdatesReceived:
        /// Idle --> Updating
        mState = TState::Updating;
        return true;
    case TEvent::NewRequestReceived:
        /// Idle --> Processing
        mState = TState::Processing;
        return true;
    case TEvent::ApiDisconnected:
        /// Idle --> NotReady
        mState = TState::NotReady;
        return true;
    case TEvent::AllReadyToProcessing:
    case TEvent::AllProcessed:
        break;
    }
    return false;
}

bool LocalStoreStateMachineLogic::ProcessProcessingState(TEvent aEvent)
{
    assert(mState == TState::Processing);
    switch (aEvent)
    {
    case TEvent::UpdatesReceived:
        /// Processing --> Updating
        mState = TState::Updating;
        return true;
    case TEvent::NewRequestReceived:
        return true;
    case TEvent::ApiDisconnected:
        /// Processing --> NotReady
        mState = TState::NotReady;
        return true;
    case TEvent::AllProcessed:
        /// Processing --> Idle
        mState = TState::Idle;
        return true;
    case TEvent::AllReadyToProcessing:
        break;
    }
    return false;
}

bool LocalStoreStateMachineLogic::ProcessUpdatingState(TEvent aEvent)
{
    assert(mState == TState::Updating);
    switch (aEvent)
    {
    case TEvent::UpdatesReceived:
    case TEvent::NewRequestReceived:
        return true;
    case TEvent::ApiDisconnected:
        /// Updating --> NotReady
        mState = TState::NotReady;
        return true;
    case TEvent::AllReadyToProcessing:
        /// Updating --> Processing
        mState = TState::Processing;
        return true;
    case TEvent::AllProcessed:
        break;
    }
    return false;
}

LocalStoreStateMachine::LocalStoreStateMachine(Basis::Tracer& aTracer)
    : mTracer(aTracer)
{}

void LocalStoreStateMachine::ChangeState(LocalStoreStateMachineLogic::TEvent aEvent)
{
    auto oldState = GetState();
    if (!ChangeStateInternal(aEvent))
    {
        mTracer.ErrorSlow("SetState failed: state:", GetState(), ", aEvent: ", aEvent);
        assert(false);
    }
    if (GetState() != oldState)
    {
        mTracer.InfoSlow("State changed: old:", oldState, ", new: ", GetState());
    }
}

}
