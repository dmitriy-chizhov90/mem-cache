#include "UiLocalStore/PackRanges.hpp"

#include <Basis/BaseTestFixture.hpp>
#include <Basis/Tracing.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

using TPackRangesTests_Iterator = Basis::Pack<int>::const_iterator;

BOOST_AUTO_TEST_SUITE(UiServer_PackRangesTests)

struct PackRangesTests : public BaseTestFixture
{
    using TData = int;
    using TDataPack = Basis::Pack<int>;
    using TPackIterator = TPackRangesTests_Iterator;
    using TPackRanges = Basis::GMock;

    Basis::Pack<int> Buffer;

    Basis::Tracer& Tracer;
    PackRanges<PackRangesTests> PackRange;

    PackRangesTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart("TestPart")))
        , PackRange(Tracer)
    {
        std::srand(unsigned(std::time(nullptr)));
    }

    void FillRandomData(int aCount)
    {
        for (int i = 0; i < aCount; ++i)
        {
            Buffer.push_back(Basis::MakeSPtr<int>(rand() % aCount));
        }
    }

    void TestInit()
    {
        EXPECT_CALL(PackRange.Ranges, Add(Eq(std::make_pair(Buffer.cbegin(), Buffer.cend()))));
        PackRange.Init(Buffer);
    }

    void TestGetElement(size_t aShift)
    {
        EXPECT_CALL(PackRange.Ranges, Next())
            .WillOnce(Return(Buffer.cbegin() + static_cast<int>(aShift)));

        auto result = PackRange.GetNext();
        BOOST_CHECK_EQUAL(result, Buffer[aShift]);
    }

    void TestNullElement()
    {
        EXPECT_CALL(PackRange.Ranges, Next())
            .WillOnce(Return(std::nullopt));

        auto result = PackRange.GetNext();
        BOOST_CHECK(!result.HasValue());
    }

};

BOOST_FIXTURE_TEST_CASE(TestInitRange, PackRangesTests)
{
    FillRandomData(5);
    TestInit();
}

BOOST_FIXTURE_TEST_CASE(TestGetNext, PackRangesTests)
{
    FillRandomData(5);
    TestGetElement(0);
    TestGetElement(4);
}

BOOST_FIXTURE_TEST_CASE(TestGetNull, PackRangesTests)
{
    TestNullElement();
}

BOOST_FIXTURE_TEST_CASE(TestReset, PackRangesTests)
{
    FillRandomData(5);
    TestInit();

    EXPECT_CALL(PackRange.Ranges, Reset());
    PackRange.Reset();

    TestInit();
}

BOOST_AUTO_TEST_SUITE_END()
}

namespace std
{

std::ostream& operator << (std::ostream& stream, const std::optional<NTPro::Ecn::NewUiServer::TPackRangesTests_Iterator>&)
{
    return stream;
}
}

