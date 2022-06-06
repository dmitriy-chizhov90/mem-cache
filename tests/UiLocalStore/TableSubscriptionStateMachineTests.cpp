#include "UiLocalStore/TableSubscriptionStateMachine.hpp"

#include <Basis/BaseTestFixture.hpp>


namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_TableSubscriptionStateMachineTests)

struct TableSubscriptionStateMachineTests : public BaseTestFixture
{
    using TState = ISubscriptionStateMachine::TableState;
    using TEvent = ISubscriptionStateMachine::TableEvent;

    void TestChangeState(TableSubscriptionStateMachineLogic& outMachine, TEvent aEvent, TState aExpectedState, bool aResult)
    {
        BOOST_CHECK_EQUAL(aResult, outMachine.ChangeStateInternal(aEvent));
        BOOST_CHECK_EQUAL(aExpectedState, outMachine.GetState());
    }

    TableSubscriptionStateMachineLogic TestInitializationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine;
        BOOST_CHECK_EQUAL(TState::Initializing, machine.GetState());
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    TableSubscriptionStateMachineLogic TestFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine = TestInitializationState(TEvent::Initialized, TState::Filtration, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    TableSubscriptionStateMachineLogic TestSortingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine = TestFiltrationState(TEvent::FiltrationCompleted, TState::Sorting, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    TableSubscriptionStateMachineLogic TestOkState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine = TestSortingState(TEvent::SortingCompleted, TState::Ok, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    TableSubscriptionStateMachineLogic TestUpdatingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine = TestOkState(TEvent::UpdatesReceived, TState::Updating, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    TableSubscriptionStateMachineLogic TestIncrementMakingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine = TestUpdatingState(TEvent::RawDataUpdated, TState::IncrementMaking, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    TableSubscriptionStateMachineLogic TestIncrementApplyingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine = TestIncrementMakingState(TEvent::IncrementMade, TState::IncrementApplying, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    TableSubscriptionStateMachineLogic TestErrorState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        TableSubscriptionStateMachineLogic machine = TestInitializationState(TEvent::ErrorOccured, TState::Error, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }
};

BOOST_FIXTURE_TEST_CASE(CheckDefaultState, TableSubscriptionStateMachineTests)
{
    TableSubscriptionStateMachineLogic machine;
    BOOST_CHECK_EQUAL(TState::Initializing, machine.GetState());
}

BOOST_FIXTURE_TEST_CASE(InitializingTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestInitializationState(TEvent::Initialized, TState::Filtration, true);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestInitializationState(TEvent::FiltrationCompleted, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestInitializationState(TEvent::SortingCompleted, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestInitializationState(TEvent::UpdatesReceived, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestInitializationState(TEvent::RawDataUpdated, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestInitializationState(TEvent::IncrementMade, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestInitializationState(TEvent::IncrementApplied, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestInitializationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(FiltrationTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestFiltrationState(TEvent::Initialized, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestFiltrationState(TEvent::FiltrationCompleted, TState::Sorting, true);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestFiltrationState(TEvent::SortingCompleted, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestFiltrationState(TEvent::UpdatesReceived, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestFiltrationState(TEvent::RawDataUpdated, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestFiltrationState(TEvent::IncrementMade, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestFiltrationState(TEvent::IncrementApplied, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(SortingTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestSortingState(TEvent::Initialized, TState::Sorting, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestSortingState(TEvent::FiltrationCompleted, TState::Sorting, false);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestSortingState(TEvent::SortingCompleted, TState::Ok, true);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestSortingState(TEvent::UpdatesReceived, TState::Sorting, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestSortingState(TEvent::RawDataUpdated, TState::Sorting, false);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestSortingState(TEvent::IncrementMade, TState::Sorting, false);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestSortingState(TEvent::IncrementApplied, TState::Sorting, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestSortingState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(OkTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestOkState(TEvent::Initialized, TState::Ok, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestOkState(TEvent::FiltrationCompleted, TState::Ok, false);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestOkState(TEvent::SortingCompleted, TState::Ok, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestOkState(TEvent::UpdatesReceived, TState::Updating, true);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestOkState(TEvent::RawDataUpdated, TState::Ok, false);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestOkState(TEvent::IncrementMade, TState::Ok, false);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestOkState(TEvent::IncrementApplied, TState::Ok, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestOkState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(UpdatingTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestUpdatingState(TEvent::Initialized, TState::Updating, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestUpdatingState(TEvent::FiltrationCompleted, TState::Updating, false);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestUpdatingState(TEvent::SortingCompleted, TState::Updating, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestUpdatingState(TEvent::UpdatesReceived, TState::Updating, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestUpdatingState(TEvent::RawDataUpdated, TState::IncrementMaking, true);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestUpdatingState(TEvent::IncrementMade, TState::Updating, false);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestUpdatingState(TEvent::IncrementApplied, TState::Updating, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestUpdatingState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(IncrementMakingTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestIncrementMakingState(TEvent::Initialized, TState::IncrementMaking, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestIncrementMakingState(TEvent::FiltrationCompleted, TState::IncrementMaking, false);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestIncrementMakingState(TEvent::SortingCompleted, TState::IncrementMaking, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestIncrementMakingState(TEvent::UpdatesReceived, TState::IncrementMaking, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestIncrementMakingState(TEvent::RawDataUpdated, TState::IncrementMaking, false);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestIncrementMakingState(TEvent::IncrementMade, TState::IncrementApplying, true);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestIncrementMakingState(TEvent::IncrementApplied, TState::IncrementMaking, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestIncrementMakingState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(IncrementApplyingTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestIncrementApplyingState(TEvent::Initialized, TState::IncrementApplying, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestIncrementApplyingState(TEvent::FiltrationCompleted, TState::IncrementApplying, false);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestIncrementApplyingState(TEvent::SortingCompleted, TState::IncrementApplying, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestIncrementApplyingState(TEvent::UpdatesReceived, TState::IncrementApplying, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestIncrementApplyingState(TEvent::RawDataUpdated, TState::IncrementApplying, false);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestIncrementApplyingState(TEvent::IncrementMade, TState::IncrementApplying, false);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestIncrementApplyingState(TEvent::IncrementApplied, TState::Ok, true);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestIncrementApplyingState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(ErrorTests, TableSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestErrorState(TEvent::Initialized, TState::Error, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestErrorState(TEvent::FiltrationCompleted, TState::Error, false);}
    BOOST_TEST_CONTEXT("SortingCompleted") {TestErrorState(TEvent::SortingCompleted, TState::Error, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestErrorState(TEvent::UpdatesReceived, TState::Error, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestErrorState(TEvent::RawDataUpdated, TState::Error, false);}
    BOOST_TEST_CONTEXT("IncrementMade") {TestErrorState(TEvent::IncrementMade, TState::Error, false);}
    BOOST_TEST_CONTEXT("IncrementApplied") {TestErrorState(TEvent::IncrementApplied, TState::Error, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestErrorState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_AUTO_TEST_SUITE_END()
}
