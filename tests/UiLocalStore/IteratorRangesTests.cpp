#include "UiLocalStore/IteratorRanges.hpp"

#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_IteratorRangesTests)

struct IteratorRangesTests : public BaseTestFixture
{
    Basis::Vector<int> Buffer;

    using TIterator = Basis::Vector<int>::const_iterator;

    Basis::GMock IndexedRanges;

    IteratorRangesTests()
    {
        std::srand(unsigned(std::time(nullptr)));
    }

    void FillRandomData(int aCount)
    {
        for (int i = 0; i < aCount; ++i)
        {
            Buffer.push_back(rand() % aCount);
        }
    }

    void TestForEach(IteratorRanges<TIterator>& aRanges, size_t aExpectedSize)
    {
        size_t i = 0;
        while (auto it = aRanges.Next())
        {
            /// Сравниваем адреса элементов.
            BOOST_CHECK_EQUAL(&Buffer[i % Buffer.size()], &**it);
            ++i;
        }
        BOOST_CHECK_EQUAL(aExpectedSize, i);
    }

};

BOOST_FIXTURE_TEST_CASE(TestDefaultRange, IteratorRangesTests)
{
    IteratorRanges<TIterator> ranges;
    BOOST_CHECK(!ranges.Next());
}

BOOST_FIXTURE_TEST_CASE(TestOneRange, IteratorRangesTests)
{
    IteratorRanges<TIterator> ranges;
    FillRandomData(5);
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));
    TestForEach(ranges, Buffer.size());
}

BOOST_FIXTURE_TEST_CASE(TestNextAfterNull, IteratorRangesTests)
{
    IteratorRanges<TIterator> ranges;
    FillRandomData(5);
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));
    TestForEach(ranges, Buffer.size());

    BOOST_CHECK(!ranges.Next());
}

BOOST_FIXTURE_TEST_CASE(TestOneEmptyRange, IteratorRangesTests)
{
    IteratorRanges<TIterator> ranges;
    FillRandomData(5);
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cbegin()));

    BOOST_CHECK(!ranges.Next());
}

BOOST_FIXTURE_TEST_CASE(TestMultipleRanges, IteratorRangesTests)
{
    IteratorRanges<TIterator> ranges;
    FillRandomData(5);
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));

    TestForEach(ranges, 2 * Buffer.size());
}

BOOST_FIXTURE_TEST_CASE(TestAddAfterNull, IteratorRangesTests)
{
    IteratorRanges<TIterator> ranges;
    FillRandomData(5);
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));
    TestForEach(ranges, Buffer.size());

    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));
    TestForEach(ranges, Buffer.size());
}

BOOST_FIXTURE_TEST_CASE(TestResetRanges, IteratorRangesTests)
{
    IteratorRanges<TIterator> ranges;
    FillRandomData(5);
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));
    ranges.Next();
    ranges.Reset();
    ranges.Add(std::make_pair<TIterator, TIterator>(Buffer.cbegin(), Buffer.cend()));
    TestForEach(ranges, Buffer.size());
}

BOOST_AUTO_TEST_SUITE_END()
}
