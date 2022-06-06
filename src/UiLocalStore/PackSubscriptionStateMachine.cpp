#include "UiLocalStore/PackSubscriptionStateMachine.hpp"

namespace NTPro::Ecn::NewUiServer
{

PackSubscriptionStateMachineLogic::TState PackSubscriptionStateMachineLogic::GetState() const
{
    return mState;
}

ISubscriptionStateMachine::ProcessingState PackSubscriptionStateMachineLogic::GetProcessingState() const
{
    switch (mState)
    {
    case TState::Initializing:
    case TState::Filtration:
    case TState::Updating:
    case TState::DeletedFiltration:
    case TState::AddedFiltration:
        return ISubscriptionStateMachine::ProcessingState::Processing;
    case TState::PendingResult:
    case TState::FinalPendingResult:
        return ISubscriptionStateMachine::ProcessingState::Pending;
    default:
        break;
    }
    return ISubscriptionStateMachine::ProcessingState::Idle;
}

bool PackSubscriptionStateMachineLogic::ChangeStateInternal(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    switch (mState)
    {
    case TState::Initializing:
        return ProcessInitializingState(aEvent);
    case TState::Filtration:
        return ProcessFiltrationState(aEvent);
    case TState::Updating:
        return ProcessUpdatingState(aEvent);
    case TState::DeletedFiltration:
        return ProcessDeletedFiltrationState(aEvent);
    case TState::AddedFiltration:
        return ProcessAddedFiltrationState(aEvent);
    case TState::Ok:
        return ProcessOkState(aEvent);
    case TState::PendingResult:
        return ProcessPendingResult(aEvent);
    case TState::FinalPendingResult:
        return ProcessFinalPendingResult(aEvent);
    case TState::Error:
        return ProcessErrorState(aEvent);
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessInitializingState(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Initializing);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::Initialized:
        mState = TState::Filtration;
        return true;
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessFiltrationState(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Filtration);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::FiltrationCompleted:
        mPreviousState = mState;
        mState = TState::FinalPendingResult;
        return true;
    case TEvent::PendingResultCompleted:
        mPreviousState = mState;
        mState = TState::PendingResult;
        return true;
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessUpdatingState(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Updating);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::RawDataUpdated:
        mState = TState::AddedFiltration;
        return true;
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessDeletedFiltrationState(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::DeletedFiltration);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::DeletedFiltrationCompleted:
        mState = TState::Ok;
        return true;
    case TEvent::DeletedFiltrationCompletedWithResult:
        mPreviousState = mState;
        mState = TState::FinalPendingResult;
        return true;
    case TEvent::PendingResultCompleted:
        mPreviousState = mState;
        mState = TState::PendingResult;
        return true;
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessAddedFiltrationState(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::AddedFiltration);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::AddedFiltrationCompleted:
        mState = TState::DeletedFiltration;
        return true;
    case TEvent::AddedFiltrationCompletedWithResult:
        mPreviousState = mState;
        mState = TState::FinalPendingResult;
        return true;
    case TEvent::PendingResultCompleted:
        mPreviousState = mState;
        mState = TState::PendingResult;
        return true;
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessOkState(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Ok);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::UpdatesReceived:
        mState = TState::Updating;
        return true;
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessPendingResult(TEvent aEvent)
{
    assert(mState == TState::PendingResult);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::GetNextReceived:
        if (!mPreviousState.has_value())
        {
            return false;
        }
        mState = *mPreviousState;
        mPreviousState.reset();
        return true;
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessFinalPendingResult(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::FinalPendingResult);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::GetNextReceived:
    {
        if (!mPreviousState.has_value())
        {
            return false;
        }
        auto previous = *mPreviousState;
        mPreviousState.reset();
        switch (previous)
        {
        case TState::Filtration:
        case TState::DeletedFiltration:
            mState = TState::Ok;
            return true;
        case TState::AddedFiltration:
            mState = TState::DeletedFiltration;
            return true;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

bool PackSubscriptionStateMachineLogic::ProcessErrorState(PackSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Error);
    if (aEvent == TEvent::ErrorOccured)
    {
        mState = TState::Error;
        return true;
    }
    return false;
}

}
