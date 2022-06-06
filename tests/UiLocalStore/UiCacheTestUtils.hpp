#pragma once

#include <boost/test/unit_test.hpp>

#include "UiSession.hpp"
#include "DummyTableData.hpp"

#include "UiLocalStore/ILocalStoreLogic.hpp"

namespace NTPro::Ecn::NewUiServer
{

using TResult = typename ILocalStoreLogic<DummyTableItem>::TPendingUpdate;
using TUiSubscription = TradingSerialization::Table::SubscribeBase;
using TUiRequestId = TUiSubscription::TId;

inline auto MakeRequestId()
{
    Basis::UniqueIdGenerator<TUiRequestId> gen;
    return gen.GetNext();
}

inline void CompareValidResults(const TResult& aLhs, const TResult& aRhs)
{
    BOOST_CHECK(aLhs.Processed);
    BOOST_CHECK(aRhs.Processed);
    BOOST_CHECK_EQUAL(static_cast<int>(aLhs.ResultState), static_cast<int>(aRhs.ResultState));
    BOOST_REQUIRE(aLhs.SubscriptionId);
    BOOST_REQUIRE(aRhs.SubscriptionId);
    BOOST_CHECK_EQUAL(*aLhs.SubscriptionId, *aRhs.SubscriptionId);
}

struct UiRequestsComparer
{
    TUiRequestId Expected;
    UiRequestsComparer(const TUiRequestId& aExpected)
        : Expected(aExpected)
    {}
    bool operator() (const TUiRequestId& aActual) const
    {
        return Expected == aActual;
    }
};

}
