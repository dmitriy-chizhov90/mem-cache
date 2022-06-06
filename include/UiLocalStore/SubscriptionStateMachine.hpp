#pragma once

#include "UiLocalStore/ISubscriptionStateMachine.hpp"

#include "Common/Tracer.hpp"

namespace NTPro::Ecn::NewUiServer
{

template <typename TLogic>
class SubscriptionStateMachine : public TLogic
{
    Basis::Tracer& mTracer;
public:
    using TState = typename TLogic::TState;
    using TEvent = typename TLogic::TEvent;

    SubscriptionStateMachine(Basis::Tracer& aTracer)
        : mTracer(aTracer)
    {}

    void ChangeState(TEvent aEvent)
    {
        assert (aEvent != TEvent::ErrorOccured);

        auto oldState = this->GetState();
        if (!this->ChangeStateInternal(aEvent))
        {
            assert(false);
            ReportError("SetState failed: state:", this->GetState(), ", aEvent: ", aEvent);
        }
        mTracer.TraceSlow("Subscription state changed: old:", oldState, ", new: ", this->GetState(), ", aEvent: ", aEvent);
    }

    template<typename... TArgs>
    void ReportError(TArgs&& ... aArgs)
    {
        mTracer.ErrorSlow(std::forward<TArgs>(aArgs)...);
        this->ChangeStateInternal(TEvent::ErrorOccured);
    }
};

}
