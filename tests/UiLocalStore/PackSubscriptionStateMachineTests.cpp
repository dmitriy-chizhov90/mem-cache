#include "UiLocalStore/PackSubscriptionStateMachine.hpp"

#include <Basis/BaseTestFixture.hpp>


namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_PackSubscriptionStateMachineTests)

struct PackSubscriptionStateMachineTests : public BaseTestFixture
{
    using TState = ISubscriptionStateMachine::PackState;
    using TEvent = ISubscriptionStateMachine::PackEvent;

    void TestChangeState(PackSubscriptionStateMachineLogic& outMachine, TEvent aEvent, TState aExpectedState, bool aResult)
    {
        BOOST_CHECK_EQUAL(aResult, outMachine.ChangeStateInternal(aEvent));
        BOOST_CHECK_EQUAL(aExpectedState, outMachine.GetState());
    }

    PackSubscriptionStateMachineLogic TestInitializationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine;
        BOOST_CHECK_EQUAL(TState::Initializing, machine.GetState());
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestInitializationState(TEvent::Initialized, TState::Filtration, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestPendingFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestFinalFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestFiltrationState(TEvent::FiltrationCompleted, TState::FinalPendingResult, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestOkState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestFinalFiltrationState(TEvent::GetNextReceived, TState::Ok, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestUpdatingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestOkState(TEvent::UpdatesReceived, TState::Updating, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestAddedFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestUpdatingState(TEvent::RawDataUpdated, TState::AddedFiltration, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestAddedFinalPendingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestAddedFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::FinalPendingResult, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestPendingDeletedFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestDeletedFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestDeletedFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestAddedFiltrationState(TEvent::AddedFiltrationCompleted, TState::DeletedFiltration, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestDeletedFinalPendingState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestDeletedFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::FinalPendingResult, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestPendingAddedFiltrationState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestAddedFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }

    PackSubscriptionStateMachineLogic TestErrorState(TEvent aEvent, TState aExpectedState, bool aResult)
    {
        PackSubscriptionStateMachineLogic machine = TestInitializationState(TEvent::ErrorOccured, TState::Error, true);
        TestChangeState(machine, aEvent, aExpectedState, aResult);
        return machine;
    }
};

BOOST_FIXTURE_TEST_CASE(CheckDefaultState, PackSubscriptionStateMachineTests)
{
    PackSubscriptionStateMachineLogic machine;
    BOOST_CHECK_EQUAL(TState::Initializing, machine.GetState());
}

BOOST_FIXTURE_TEST_CASE(InitializingTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestInitializationState(TEvent::Initialized, TState::Filtration, true);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestInitializationState(TEvent::FiltrationCompleted, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestInitializationState(TEvent::UpdatesReceived, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestInitializationState(TEvent::RawDataUpdated, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestInitializationState(TEvent::DeletedFiltrationCompleted, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestInitializationState(TEvent::DeletedFiltrationCompletedWithResult, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestInitializationState(TEvent::AddedFiltrationCompleted, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestInitializationState(TEvent::AddedFiltrationCompletedWithResult, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestInitializationState(TEvent::PendingResultCompleted, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestInitializationState(TEvent::GetNextReceived, TState::Initializing, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestInitializationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(FiltrationTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestFiltrationState(TEvent::Initialized, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestFiltrationState(TEvent::FiltrationCompleted, TState::FinalPendingResult, true);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestFiltrationState(TEvent::UpdatesReceived, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestFiltrationState(TEvent::RawDataUpdated, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestFiltrationState(TEvent::DeletedFiltrationCompleted, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestFiltrationState(TEvent::AddedFiltrationCompleted, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, true);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestFiltrationState(TEvent::GetNextReceived, TState::Filtration, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(FinalFiltrationPendingResultTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestFinalFiltrationState(TEvent::Initialized, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestFinalFiltrationState(TEvent::FiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestFinalFiltrationState(TEvent::UpdatesReceived, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestFinalFiltrationState(TEvent::RawDataUpdated, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestFinalFiltrationState(TEvent::DeletedFiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestFinalFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestFinalFiltrationState(TEvent::AddedFiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestFinalFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestFinalFiltrationState(TEvent::PendingResultCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestFinalFiltrationState(TEvent::GetNextReceived, TState::Ok, true);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestFinalFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(PendingFiltrationTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestPendingFiltrationState(TEvent::Initialized, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestPendingFiltrationState(TEvent::FiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestPendingFiltrationState(TEvent::UpdatesReceived, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestPendingFiltrationState(TEvent::RawDataUpdated, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestPendingFiltrationState(TEvent::DeletedFiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestPendingFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestPendingFiltrationState(TEvent::AddedFiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestPendingFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestPendingFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestPendingFiltrationState(TEvent::GetNextReceived, TState::Filtration, true);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestPendingFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(OkTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestOkState(TEvent::Initialized, TState::Ok, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestOkState(TEvent::FiltrationCompleted, TState::Ok, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestOkState(TEvent::UpdatesReceived, TState::Updating, true);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestOkState(TEvent::RawDataUpdated, TState::Ok, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestOkState(TEvent::DeletedFiltrationCompleted, TState::Ok, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestOkState(TEvent::DeletedFiltrationCompletedWithResult, TState::Ok, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestOkState(TEvent::AddedFiltrationCompletedWithResult, TState::Ok, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestOkState(TEvent::PendingResultCompleted, TState::Ok, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestOkState(TEvent::GetNextReceived, TState::Ok, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestOkState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(UpdatingTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestUpdatingState(TEvent::Initialized, TState::Updating, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestUpdatingState(TEvent::FiltrationCompleted, TState::Updating, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestUpdatingState(TEvent::UpdatesReceived, TState::Updating, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestUpdatingState(TEvent::RawDataUpdated, TState::AddedFiltration, true);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestUpdatingState(TEvent::DeletedFiltrationCompleted, TState::Updating, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestUpdatingState(TEvent::DeletedFiltrationCompletedWithResult, TState::Updating, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestUpdatingState(TEvent::AddedFiltrationCompleted, TState::Updating, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestUpdatingState(TEvent::AddedFiltrationCompletedWithResult, TState::Updating, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestUpdatingState(TEvent::PendingResultCompleted, TState::Updating, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestUpdatingState(TEvent::GetNextReceived, TState::Updating, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestUpdatingState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(DeletedFiltrationTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestDeletedFiltrationState(TEvent::Initialized, TState::DeletedFiltration, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestDeletedFiltrationState(TEvent::FiltrationCompleted, TState::DeletedFiltration, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestDeletedFiltrationState(TEvent::UpdatesReceived, TState::DeletedFiltration, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestDeletedFiltrationState(TEvent::RawDataUpdated, TState::DeletedFiltration, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestDeletedFiltrationState(TEvent::DeletedFiltrationCompleted, TState::Ok, true);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestDeletedFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::FinalPendingResult, true);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestDeletedFiltrationState(TEvent::AddedFiltrationCompleted, TState::DeletedFiltration, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestDeletedFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::DeletedFiltration, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestDeletedFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, true);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestDeletedFiltrationState(TEvent::GetNextReceived, TState::DeletedFiltration, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestDeletedFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(DeletedFiltrationFinalPendingTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestDeletedFinalPendingState(TEvent::Initialized, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestDeletedFinalPendingState(TEvent::FiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestDeletedFinalPendingState(TEvent::UpdatesReceived, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestDeletedFinalPendingState(TEvent::RawDataUpdated, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestDeletedFinalPendingState(TEvent::DeletedFiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestDeletedFinalPendingState(TEvent::DeletedFiltrationCompletedWithResult, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestDeletedFinalPendingState(TEvent::AddedFiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestDeletedFinalPendingState(TEvent::AddedFiltrationCompletedWithResult, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestDeletedFinalPendingState(TEvent::PendingResultCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestDeletedFinalPendingState(TEvent::GetNextReceived, TState::Ok, true);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestDeletedFinalPendingState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(PendingDeletedFiltrationTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestPendingDeletedFiltrationState(TEvent::Initialized, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestPendingDeletedFiltrationState(TEvent::FiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestPendingDeletedFiltrationState(TEvent::UpdatesReceived, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestPendingDeletedFiltrationState(TEvent::RawDataUpdated, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestPendingDeletedFiltrationState(TEvent::DeletedFiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestPendingDeletedFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestPendingDeletedFiltrationState(TEvent::AddedFiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestPendingDeletedFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestPendingDeletedFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestPendingDeletedFiltrationState(TEvent::GetNextReceived, TState::DeletedFiltration, true);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestPendingDeletedFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(AddedFiltrationTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestAddedFiltrationState(TEvent::Initialized, TState::AddedFiltration, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestAddedFiltrationState(TEvent::FiltrationCompleted, TState::AddedFiltration, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestAddedFiltrationState(TEvent::UpdatesReceived, TState::AddedFiltration, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestAddedFiltrationState(TEvent::RawDataUpdated, TState::AddedFiltration, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestAddedFiltrationState(TEvent::DeletedFiltrationCompleted, TState::AddedFiltration, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestAddedFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::AddedFiltration, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestAddedFiltrationState(TEvent::AddedFiltrationCompleted, TState::DeletedFiltration, true);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestAddedFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::FinalPendingResult, true);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestAddedFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, true);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestAddedFiltrationState(TEvent::GetNextReceived, TState::AddedFiltration, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestAddedFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(AddedFiltrationFinalPendingTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestAddedFinalPendingState(TEvent::Initialized, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestAddedFinalPendingState(TEvent::FiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestAddedFinalPendingState(TEvent::UpdatesReceived, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestAddedFinalPendingState(TEvent::RawDataUpdated, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestAddedFinalPendingState(TEvent::DeletedFiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestAddedFinalPendingState(TEvent::DeletedFiltrationCompletedWithResult, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestAddedFinalPendingState(TEvent::AddedFiltrationCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestAddedFinalPendingState(TEvent::AddedFiltrationCompletedWithResult, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestAddedFinalPendingState(TEvent::PendingResultCompleted, TState::FinalPendingResult, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestAddedFinalPendingState(TEvent::GetNextReceived, TState::DeletedFiltration, true);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestAddedFinalPendingState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(PendingAddedFiltrationTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestPendingAddedFiltrationState(TEvent::Initialized, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestPendingAddedFiltrationState(TEvent::FiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestPendingAddedFiltrationState(TEvent::UpdatesReceived, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestPendingAddedFiltrationState(TEvent::RawDataUpdated, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestPendingAddedFiltrationState(TEvent::DeletedFiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestPendingAddedFiltrationState(TEvent::DeletedFiltrationCompletedWithResult, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestPendingAddedFiltrationState(TEvent::AddedFiltrationCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestPendingAddedFiltrationState(TEvent::AddedFiltrationCompletedWithResult, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestPendingAddedFiltrationState(TEvent::PendingResultCompleted, TState::PendingResult, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestPendingAddedFiltrationState(TEvent::GetNextReceived, TState::AddedFiltration, true);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestPendingAddedFiltrationState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_FIXTURE_TEST_CASE(ErrorTests, PackSubscriptionStateMachineTests)
{
    BOOST_TEST_CONTEXT("Initialized") {TestErrorState(TEvent::Initialized, TState::Error, false);}
    BOOST_TEST_CONTEXT("FiltrationCompleted") {TestErrorState(TEvent::FiltrationCompleted, TState::Error, false);}
    BOOST_TEST_CONTEXT("UpdatesReceived") {TestErrorState(TEvent::UpdatesReceived, TState::Error, false);}
    BOOST_TEST_CONTEXT("RawDataUpdated") {TestErrorState(TEvent::RawDataUpdated, TState::Error, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompleted") {TestErrorState(TEvent::DeletedFiltrationCompleted, TState::Error, false);}
    BOOST_TEST_CONTEXT("DeletedFiltrationCompletedWithResult") {TestErrorState(TEvent::DeletedFiltrationCompletedWithResult, TState::Error, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompleted") {TestErrorState(TEvent::AddedFiltrationCompleted, TState::Error, false);}
    BOOST_TEST_CONTEXT("AddedFiltrationCompletedWithResult") {TestErrorState(TEvent::AddedFiltrationCompletedWithResult, TState::Error, false);}
    BOOST_TEST_CONTEXT("PendingResultCompleted") {TestErrorState(TEvent::PendingResultCompleted, TState::Error, false);}
    BOOST_TEST_CONTEXT("GetNextReceived") {TestErrorState(TEvent::GetNextReceived, TState::Error, false);}
    BOOST_TEST_CONTEXT("ErrorOccured") {TestErrorState(TEvent::ErrorOccured, TState::Error, true);}
}

BOOST_AUTO_TEST_SUITE_END()
}
