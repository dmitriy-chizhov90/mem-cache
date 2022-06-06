#include "UiLocalStore/LocalStoreStateMachine.hpp"

#include <Basis/BaseTestFixture.hpp>


namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_LocalStoreStateMachineTests)

struct LocalStoreStateMachineTests : public BaseTestFixture
{
    using TState = ILocalStoreStateMachine::State;
    using TEvent = ILocalStoreStateMachine::Event;

    void TestChangeState(LocalStoreStateMachineLogic& outMachine, TEvent aEvent, TState aExpectedState, bool aResult)
    {
        BOOST_CHECK_EQUAL(aResult, outMachine.ChangeStateInternal(aEvent));
        BOOST_CHECK_EQUAL(aExpectedState, outMachine.GetState());
    }

    LocalStoreStateMachineLogic TestNotReadyState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        LocalStoreStateMachineLogic machine;
        BOOST_CHECK_EQUAL(TState::NotReady, machine.GetState());
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    LocalStoreStateMachineLogic TestUpdatingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        auto machine = TestNotReadyState(TEvent::UpdatesReceived, TState::Updating, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    LocalStoreStateMachineLogic TestProcessingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        auto machine = TestUpdatingState(TEvent::AllReadyToProcessing, TState::Processing, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    LocalStoreStateMachineLogic TestIdleState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        auto machine = TestProcessingState(TEvent::AllProcessed, TState::Idle, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }
};

BOOST_FIXTURE_TEST_CASE(CheckDefaultState, LocalStoreStateMachineTests)
{
    LocalStoreStateMachineLogic machine;
    BOOST_CHECK_EQUAL(TState::NotReady, machine.GetState());
}

BOOST_FIXTURE_TEST_CASE(NotReadyTests, LocalStoreStateMachineTests)
{
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestNotReadyState(TEvent::UpdatesReceived, TState::Updating, true);}
    BOOST_TEST_CONTEXT("AllReadyToProcessing") {TestNotReadyState(TEvent::AllReadyToProcessing, TState::NotReady, false);}
    BOOST_TEST_CONTEXT("AllProcessed") {TestNotReadyState(TEvent::AllProcessed, TState::NotReady, false);}
    BOOST_TEST_CONTEXT("NewRequestReceived") {TestNotReadyState(TEvent::NewRequestReceived, TState::NotReady, true);}
    BOOST_TEST_CONTEXT("ApiDisconnected") {TestNotReadyState(TEvent::ApiDisconnected, TState::NotReady, true);}
}

BOOST_FIXTURE_TEST_CASE(UpdatingTests, LocalStoreStateMachineTests)
{
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestUpdatingState(TEvent::UpdatesReceived, TState::Updating, true);}
    BOOST_TEST_CONTEXT("AllReadyToProcessing") {TestUpdatingState(TEvent::AllReadyToProcessing, TState::Processing, true);}
    BOOST_TEST_CONTEXT("AllProcessed") {TestUpdatingState(TEvent::AllProcessed, TState::Updating, false);}
    BOOST_TEST_CONTEXT("NewRequestReceived") {TestUpdatingState(TEvent::NewRequestReceived, TState::Updating, true);}
    BOOST_TEST_CONTEXT("ApiDisconnected") {TestUpdatingState(TEvent::ApiDisconnected, TState::NotReady, true);}
}

BOOST_FIXTURE_TEST_CASE(ProcessingTests, LocalStoreStateMachineTests)
{
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestProcessingState(TEvent::UpdatesReceived, TState::Updating, true);}
    BOOST_TEST_CONTEXT("AllReadyToProcessing") {TestProcessingState(TEvent::AllReadyToProcessing, TState::Processing, false);}
    BOOST_TEST_CONTEXT("AllProcessed") {TestProcessingState(TEvent::AllProcessed, TState::Idle, true);}
    BOOST_TEST_CONTEXT("NewRequestReceived") {TestProcessingState(TEvent::NewRequestReceived, TState::Processing, true);}
    BOOST_TEST_CONTEXT("ApiDisconnected") {TestProcessingState(TEvent::ApiDisconnected, TState::NotReady, true);}
}

BOOST_FIXTURE_TEST_CASE(IdleTests, LocalStoreStateMachineTests)
{
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestIdleState(TEvent::UpdatesReceived, TState::Updating, true);}
    BOOST_TEST_CONTEXT("AllReadyToProcessing") {TestIdleState(TEvent::AllReadyToProcessing, TState::Idle, false);}
    BOOST_TEST_CONTEXT("AllProcessed") {TestIdleState(TEvent::AllProcessed, TState::Idle, false);}
    BOOST_TEST_CONTEXT("NewRequestReceived") {TestIdleState(TEvent::NewRequestReceived, TState::Processing, true);}
    BOOST_TEST_CONTEXT("ApiDisconnected") {TestIdleState(TEvent::ApiDisconnected, TState::NotReady, true);}
}

BOOST_AUTO_TEST_SUITE_END()
}
