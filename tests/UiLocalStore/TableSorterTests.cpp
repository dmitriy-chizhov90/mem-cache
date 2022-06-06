#include "DummyTableData.hpp"

#include "UiLocalStore/TableSorter.hpp"
#include "TradingSerialization/Table/Columns.hpp"


#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_TableSorterTests)

using namespace TradingSerialization::Table;

template <int _MaxCount>
struct TSorterSetup
{
    static constexpr int MaxCount = _MaxCount;
    using TData = DummyTableItem;
    using TTableSetup = DummyTableSetup;
};

struct TableSorterTests : public BaseTestFixture
{
    using TSetup = DummyTableSetup;
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TDataPack = Basis::Pack<DummyTableItem>;

    TSortOrder SortOrder;
    bool Ok = true;

    TDataPack Data;
    TDataPack ExpectedData;
    TDataPack TmpBuf;

    Basis::Tracer& Tracer;

    TableSorterTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart()))
    {
        SortOrder.push_back(static_cast<TColumnType>(DummyColumnType::Id));
        std::srand(unsigned(std::time(nullptr)));
    }

    void FillRandomData(int aCount)
    {
        Data.clear();
        for (int i = 0; i < aCount; ++i)
        {
            Data.push_back(Basis::MakeSPtr<DummyTableItem>(rand() % aCount));
        }
    }

    void CopyExpectedData()
    {
        ExpectedData.clear();
        std::copy(Data.cbegin(), Data.cend(), std::back_inserter(ExpectedData));
    }

    void SortExpectedData()
    {
        std::sort(ExpectedData.begin(), ExpectedData.end(), [](const auto& aLhs, const auto& aRhs)
        {
            return aLhs->GetValue(DummyColumnType::Id) < aRhs->GetValue(DummyColumnType::Id);
        });
    }

    void PrepareExpectedData()
    {
        CopyExpectedData();
        SortExpectedData();
    }

    void FillRandomDataAndExpectedResult(int aCount)
    {
        FillRandomData(aCount);
        PrepareExpectedData();
    }

    template <typename TSorter>
    void Init(TSorter& aSorter, TableSorterState aState)
    {
        BOOST_CHECK(!aSorter.IsInitialized());
        aSorter.Init(typename TSorter::TInit { SortOrder, Data, TmpBuf });
        BOOST_CHECK(aSorter.IsInitialized());
        BOOST_CHECK_EQUAL(aState, aSorter.GetState());
    }

    template <int _MaxCount>
    auto MakeSorter(TableSorterState aState = TableSorterState::PartSort)
    {
        using TSorter = TableSorter<TSorterSetup<_MaxCount>>;
        TSorter sorter(Tracer);
        Init(sorter, aState);
        return sorter;
    }

    void CheckResult()
    {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            ExpectedData.cbegin(), ExpectedData.cend(),
            Data.cbegin(), Data.cend());
    }

    int GetMergeCallsCount(int aPartsCount)
    {
        /// Количество частей после каждого мержа уменьшается вдвое.
        /// Количество мержей - логарифм по основанию 2.
        auto mergesCount = static_cast<int>(std::ceil(std::log2l(aPartsCount)));
        /// На каждом вызове мы можем пройти не больше одного парта.
        /// Поэтому каждая итерация приводит к aPartsCount вызовам.
        return aPartsCount * mergesCount;
    }

    template <typename TSorter>
    void TestSortInternal(int aPartsCount, TSorter& aSorter)
    {
        for (int i = 0; i < aPartsCount - 1; ++i)
        {
            BOOST_CHECK_EQUAL(TableSorterState::PartSort, aSorter.Process());
        }
        const auto mergesCount = GetMergeCallsCount(aPartsCount);
        for (int i = 0; i < mergesCount; ++i)
        {
            BOOST_CHECK_EQUAL(TableSorterState::MergeSort, aSorter.Process());
        }
        BOOST_CHECK_EQUAL(TableSorterState::Completed, aSorter.Process());
        CheckResult();
    }

    template <int PartSize>
    auto TestSort(int aPartsCount, int aLastPartSize = PartSize)
    {
        FillRandomDataAndExpectedResult((aPartsCount - 1) * PartSize + aLastPartSize);
        auto sorter = MakeSorter<PartSize>();
        TestSortInternal(aPartsCount, sorter);
        return sorter;
    }
};

BOOST_FIXTURE_TEST_CASE(SuccessCreationTest, TableSorterTests)
{
    MakeSorter<10>();
}

BOOST_FIXTURE_TEST_CASE(EmptySortOrderTest, TableSorterTests)
{
    SortOrder.clear();
    MakeSorter<10>(TableSorterState::Error);
}

BOOST_FIXTURE_TEST_CASE(InvalidColumnTest, TableSorterTests)
{
    SortOrder.push_back(100);
    MakeSorter<10>(TableSorterState::Error);
}


BOOST_AUTO_TEST_CASE(EqualRandomPartsTests)
{
    for (int i = 1; i <= 8; ++i)
    {
        BOOST_TEST_CONTEXT("Parts count: " + Basis::ToString(i))
        {
            TableSorterTests fixture;
            fixture.TestSort<500>(i);
        }
    }
}

BOOST_AUTO_TEST_CASE(EqualRandomOddPartsTests)
{
    for (int i = 1; i <= 8; ++i)
    {
        BOOST_TEST_CONTEXT("Parts count: " + Basis::ToString(i))
        {
            TableSorterTests fixture;
            fixture.TestSort<501>(i);
        }
    }
}

BOOST_AUTO_TEST_CASE(AlmostCompleteLastPartTests)
{
    for (int i = 1; i <= 8; ++i)
    {
        BOOST_TEST_CONTEXT("Parts count: " + Basis::ToString(i))
        {
            TableSorterTests fixture;
            fixture.TestSort<500>(i, 499);
        }
    }
}

BOOST_AUTO_TEST_CASE(AlmostEmptyLastPartTests)
{
    for (int i = 1; i <= 8; ++i)
    {
        BOOST_TEST_CONTEXT("Parts count: " + Basis::ToString(i))
        {
            TableSorterTests fixture;
            fixture.TestSort<500>(i, 1);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(EmptyDataTest, TableSorterTests)
{
    auto sorter = MakeSorter<500>();
    BOOST_CHECK_EQUAL(TableSorterState::Completed, sorter.Process());
}

BOOST_FIXTURE_TEST_CASE(ProcessAfterCompletedFailureTest, TableSorterTests)
{
    FillRandomDataAndExpectedResult(250);
    auto sorter = MakeSorter<500>();
    BOOST_CHECK_EQUAL(TableSorterState::Completed, sorter.Process());

    BOOST_CHECK_EQUAL(TableSorterState::Error, sorter.Process());
    BOOST_CHECK_EQUAL(TableSorterState::Error, sorter.Process());
}

BOOST_FIXTURE_TEST_CASE(TestReset, TableSorterTests)
{
    auto sorter = TestSort<500>(8);
    sorter.Reset();

    FillRandomDataAndExpectedResult((3) * 500 + 99);
    Init(sorter, TableSorterState::PartSort);
    TestSortInternal(4, sorter);
}

BOOST_FIXTURE_TEST_CASE(LoadTest, TableSorterTests)
{
    static constexpr int  PartSize = 500;
    static constexpr bool DoTest = false;
    static constexpr int  ItemsCount = DoTest ? 1'000'000 : 1'000;

    [[maybe_unused]] auto d1 = Basis::DateTime::Now();
    FillRandomData(ItemsCount);
    [[maybe_unused]] auto d2 = Basis::DateTime::Now();
    CopyExpectedData();
    [[maybe_unused]] auto d3 = Basis::DateTime::Now();
    auto sorter = MakeSorter<PartSize>();
    [[maybe_unused]] auto d4 = Basis::DateTime::Now();
    while (TableSorterState::Completed != sorter.Process()) {}
    [[maybe_unused]] auto d5 = Basis::DateTime::Now();
    SortExpectedData();
    [[maybe_unused]] auto d6 = Basis::DateTime::Now();

    if constexpr (DoTest)
    {
        std::cout << "Allocation: " << d2.DiffMicroseconds(d1) << " mcs; " << d2.DiffMilliseconds(d1) << " ms;" << std::endl;
        std::cout << "Copy      : " << d3.DiffMicroseconds(d2) << " mcs; " << d3.DiffMilliseconds(d2) << " ms;" << std::endl;
        std::cout << "Init      : " << d4.DiffMicroseconds(d3) << " mcs; " << d4.DiffMilliseconds(d3) << " ms;" << std::endl;
        std::cout << "SortCustom: " << d5.DiffMicroseconds(d4) << " mcs; " << d5.DiffMilliseconds(d4) << " ms;" << std::endl;
        std::cout << "std::sort : " << d6.DiffMicroseconds(d5) << " mcs; " << d6.DiffMilliseconds(d5) << " ms;" << std::endl;
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
