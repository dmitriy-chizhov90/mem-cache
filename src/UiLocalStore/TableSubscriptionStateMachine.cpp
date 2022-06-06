#include "UiLocalStore/TableSubscriptionStateMachine.hpp"

namespace NTPro::Ecn::NewUiServer
{

TableSubscriptionStateMachineLogic::TState TableSubscriptionStateMachineLogic::GetState() const
{
    return mState;
}

ISubscriptionStateMachine::ProcessingState TableSubscriptionStateMachineLogic::GetProcessingState() const
{
    switch (mState)
    {
    case TState::Initializing:
    case TState::Filtration:
    case TState::Sorting:
    case TState::Updating:
    case TState::IncrementMaking:
    case TState::IncrementApplying:
        return ISubscriptionStateMachine::ProcessingState::Processing;
    default:
        break;
    }
    return ISubscriptionStateMachine::ProcessingState::Idle;
}

bool TableSubscriptionStateMachineLogic::ChangeStateInternal(TableSubscriptionStateMachineLogic::TEvent aEvent)
{
    switch (mState)
    {
    case TState::Initializing:
        return ProcessInitializingState(aEvent);
    case TState::Filtration:
        return ProcessFiltrationState(aEvent);
    case TState::Sorting:
        return ProcessSortingState(aEvent);
    case TState::Updating:
        return ProcessUpdatingState(aEvent);
    case TState::IncrementMaking:
        return ProcessIncrementMakingState(aEvent);
    case TState::IncrementApplying:
        return ProcessIncrementApplyingState(aEvent);
    case TState::Ok:
        return ProcessOkState(aEvent);
    case TState::Error:
        return ProcessErrorState(aEvent);
    }
    return false;
}

bool TableSubscriptionStateMachineLogic::ProcessInitializingState(TableSubscriptionStateMachineLogic::TEvent aEvent)
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

bool TableSubscriptionStateMachineLogic::ProcessFiltrationState(TableSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Filtration);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::FiltrationCompleted:
        mState = TState::Sorting;
        return true;
    default:
        break;
    }
    return false;
}

bool TableSubscriptionStateMachineLogic::ProcessSortingState(TableSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Sorting);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::SortingCompleted:
        mState = TState::Ok;
        return true;
    default:
        break;
    }
    return false;
}

bool TableSubscriptionStateMachineLogic::ProcessUpdatingState(TableSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::Updating);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::RawDataUpdated:
        mState = TState::IncrementMaking;
        return true;
    default:
        break;
    }
    return false;
}

bool TableSubscriptionStateMachineLogic::ProcessIncrementMakingState(TableSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::IncrementMaking);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::IncrementMade:
        mState = TState::IncrementApplying;
        return true;
    default:
        break;
    }
    return false;
}

bool TableSubscriptionStateMachineLogic::ProcessIncrementApplyingState(TableSubscriptionStateMachineLogic::TEvent aEvent)
{
    assert(mState == TState::IncrementApplying);
    switch (aEvent)
    {
    case TEvent::ErrorOccured:
        mState = TState::Error;
        return true;
    case TEvent::IncrementApplied:
        mState = TState::Ok;
        return true;
    default:
        break;
    }
    return false;
}

bool TableSubscriptionStateMachineLogic::ProcessOkState(TableSubscriptionStateMachineLogic::TEvent aEvent)
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

bool TableSubscriptionStateMachineLogic::ProcessErrorState(TableSubscriptionStateMachineLogic::TEvent aEvent)
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
