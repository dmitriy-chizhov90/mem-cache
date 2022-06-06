#include "DummyTableData.hpp"

#include "UiLocalStore/TableIncrementApplicator.hpp"
#include "TradingSerialization/Table/Columns.hpp"


#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include <boost/iterator/zip_iterator.hpp>
#include <cmath>

namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_TableIncrementApplicatorTests)

using namespace TradingSerialization::Table;

template <int _MaxCount>
struct TApplicatorSetup
{
    static constexpr int MaxCount = _MaxCount;
    using TData = DummyTableItem;
    using TTableSetup = DummyTableSetup;
};

struct TableIncrementApplicatorTests : public BaseTestFixture
{
    using TSetup = DummyTableSetup;
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TDataPack = Basis::Pack<DummyTableItem>;
    using TSPtrDataPack = Basis::SPtr<TDataPack>;

    TSortOrder SortOrder;
    bool Ok = true;

    TSPtrDataPack OldSnapshot;
    TDataPack NewSnapshot;
    TDataPack DeletedIncrement;
    TDataPack AddedIncrement;
    TDataPack ExpectedNewSnapshot;

    Basis::Tracer& Tracer;

    TableIncrementApplicatorTests()
        : Tracer(Basis::Tracing::GetTracer(CreateTestPart()))
    {
        SortOrder.push_back(static_cast<TColumnType>(DummyColumnType::Id));
        std::srand(unsigned(std::time(nullptr)));
    }

    void FillPack(TDataPack& outPack, const Basis::Vector<int>& aData, const std::string& aValue)
    {
        outPack.clear();
        std::transform(aData.cbegin(), aData.cend(), std::back_inserter(outPack), [&](auto aId)
        {
            return Basis::MakeSPtr<DummyTableItem>(aId, aValue);
        });
    }

    void FillPack(TDataPack& outPack, const Basis::Vector<int>& aData, const Basis::Vector<std::string>& aValues)
    {
        outPack.clear();
        auto iterBegin = boost::make_zip_iterator(boost::make_tuple(aData.begin(), aValues.begin()));
        auto iterEnd = boost::make_zip_iterator(boost::make_tuple(aData.end(), aValues.end()));
        std::transform(iterBegin, iterEnd, std::back_inserter(outPack), [&](const auto& aPair)
        {
            const auto& [id, value] = aPair;
            return Basis::MakeSPtr<DummyTableItem>(id, value);
        });
    }

    void FillOldSnapshot(const Basis::Vector<int>& aData, const std::string& aValue)
    {
        auto snapshot = Basis::MakeShared<TDataPack>();
        FillPack(*snapshot, aData, aValue);
        OldSnapshot = snapshot;
    }
    void FillDeletedIncrement(const Basis::Vector<int>& aData, const std::string& aValue)
    {
        FillPack(DeletedIncrement, aData, aValue);
    }
    void FillAddedIncrement(const Basis::Vector<int>& aData, const std::string& aValue)
    {
        FillPack(AddedIncrement, aData, aValue);
    }

    void FillExpectedResult(const Basis::Vector<int>& aData, const Basis::Vector<std::string>& aExpectedValues)
    {
        FillPack(ExpectedNewSnapshot, aData, aExpectedValues);
    }


    template <typename TIncrementApplicator>
    void Init(
        TIncrementApplicator& aIncrementApplicator,
        TableIncrementApplicatorState aState = TableIncrementApplicatorState::Processing)
    {
        BOOST_CHECK(!aIncrementApplicator.IsInitialized());
        aIncrementApplicator.Init(typename TIncrementApplicator::TInit
        {
            SortOrder,
            OldSnapshot,
            DeletedIncrement,
            AddedIncrement,
            NewSnapshot
        });
        BOOST_CHECK(aIncrementApplicator.IsInitialized());
        BOOST_CHECK_EQUAL(aState, aIncrementApplicator.GetState());
    }

    template <int _MaxCount>
    auto MakeIncrementApplicator(TableIncrementApplicatorState aState = TableIncrementApplicatorState::Processing)
    {
        using TIncrementApplicator = TableIncrementApplicator<TApplicatorSetup<_MaxCount>>;
        TIncrementApplicator incrementApplicator(Tracer);
        Init(incrementApplicator, aState);
        return incrementApplicator;
    }

    void CheckResult()
    {
        BOOST_CHECK_EQUAL_COLLECTIONS(
            ExpectedNewSnapshot.cbegin(), ExpectedNewSnapshot.cend(),
            NewSnapshot.cbegin(), NewSnapshot.cend());
    }

    template <typename TIncrementApplicator>
    void TestMerge(int aPartsCount, TIncrementApplicator& aIncrementApplicator)
    {
        for (int i = 0; i < aPartsCount - 1; ++i)
        {
            BOOST_CHECK_EQUAL(TableIncrementApplicatorState::Processing, aIncrementApplicator.Process());
        }
        BOOST_CHECK_EQUAL(TableIncrementApplicatorState::Completed, aIncrementApplicator.Process());
        CheckResult();
    }

    template <int _MaxCount = 500>
    auto TestMerge(
        const Basis::Vector<int>& aOld,
        const Basis::Vector<int>& aDeleted,
        const Basis::Vector<int>& aAdded,
        const Basis::Vector<int>& aExpected,
        const Basis::Vector<std::string>& aExpectedValues,
        int aPartsCount = 1)
    {
        FillOldSnapshot(aOld, "Value1");
        if (!aDeleted.empty())
        {
            FillDeletedIncrement(aDeleted, "Value1");
        }
        if (!aAdded.empty())
        {
            FillAddedIncrement(aAdded, "Value2");
        }
        FillExpectedResult(aExpected, aExpectedValues);
        auto applicator = MakeIncrementApplicator<_MaxCount>();
        TestMerge(aPartsCount, applicator);
        return applicator;
    }
};

BOOST_FIXTURE_TEST_CASE(SuccessCreationTest, TableIncrementApplicatorTests)
{
    FillOldSnapshot({}, "");
    MakeIncrementApplicator<10>();
}

BOOST_FIXTURE_TEST_CASE(InvalidColumnTest, TableIncrementApplicatorTests)
{
    FillOldSnapshot({}, "");
    SortOrder.push_back(100);
    MakeIncrementApplicator<10>(TableIncrementApplicatorState::Error);
}

BOOST_FIXTURE_TEST_CASE(AddHeadValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 1, 2 },
        Basis::Vector<int> { 1, 2, 3, 10, 12 },
        Basis::Vector<std::string> { "Value2", "Value2", "Value1", "Value1", "Value1" });
}

BOOST_FIXTURE_TEST_CASE(AddTailValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 14, 15 },
        Basis::Vector<int> { 3, 10, 12, 14, 15 },
        Basis::Vector<std::string> { "Value1", "Value1", "Value1", "Value2", "Value2" });
}

BOOST_FIXTURE_TEST_CASE(AddMiddleValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 5, 11 },
        Basis::Vector<int> { 3, 5, 10, 11, 12 },
        Basis::Vector<std::string> { "Value1", "Value2", "Value1", "Value2", "Value1" });
}

BOOST_FIXTURE_TEST_CASE(AddAllValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 1, 2, 3, 5, 10, 11, 12, 14, 15 },
        Basis::Vector<int> { 1, 2, 3, 5, 10, 11, 12, 14, 15 },
        Basis::Vector<std::string> { 9, "Value2" });
}

BOOST_FIXTURE_TEST_CASE(AddEmptyDataTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> {},
        Basis::Vector<int> {},
        Basis::Vector<int> { 1, 2, 3 },
        Basis::Vector<int> { 1, 2, 3 },
        Basis::Vector<std::string> { 3, "Value2" });
}

BOOST_FIXTURE_TEST_CASE(TryDeleteHeadValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 1, 2 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<std::string> { 3, "Value1" });
}

BOOST_FIXTURE_TEST_CASE(TryDeleteTailValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 14, 15 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<std::string> { 3, "Value1" });
}

BOOST_FIXTURE_TEST_CASE(TryDeleteMiddleValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 5, 11 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<std::string> { 3, "Value1" });
}


BOOST_FIXTURE_TEST_CASE(DeleteMiddleValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> {},
        Basis::Vector<int> {},
        Basis::Vector<std::string> {});
}

BOOST_FIXTURE_TEST_CASE(DeleteAllValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 1, 2, 3, 5, 10, 11, 12, 14, 15 },
        Basis::Vector<int> {},
        Basis::Vector<int> {  },
        Basis::Vector<std::string> {});
}

BOOST_FIXTURE_TEST_CASE(DeleteEmptyDataTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> {},
        Basis::Vector<int> { 1, 2, 3},
        Basis::Vector<int> {},
        Basis::Vector<int> {},
        Basis::Vector<std::string> {});
}

BOOST_FIXTURE_TEST_CASE(EmptyTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> {},
        Basis::Vector<int> {},
        Basis::Vector<int> {},
        Basis::Vector<int> {},
        Basis::Vector<std::string> {});
}

BOOST_FIXTURE_TEST_CASE(EmptyIncrementTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 1, 2, 3 },
        Basis::Vector<int> {},
        Basis::Vector<int> {},
        Basis::Vector<int> { 1, 2, 3 },
        Basis::Vector<std::string> { 3, "Value1" });
}

BOOST_FIXTURE_TEST_CASE(AddDeleteValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 1, 10, 14 },
        Basis::Vector<int> { 2, 5, 11, 15 },
        Basis::Vector<int> { 2, 3, 5, 11, 12, 15 },
        Basis::Vector<std::string> { "Value2", "Value1", "Value2", "Value2", "Value1", "Value2" });
}

BOOST_FIXTURE_TEST_CASE(AddAndReplaceMiddleValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> {},
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<std::string> { 3, "Value2" });
}

BOOST_FIXTURE_TEST_CASE(DeleteAndReplaceMiddleValuesTest, TableIncrementApplicatorTests)
{
    TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<std::string> { 3, "Value2" });
}

BOOST_FIXTURE_TEST_CASE(ProcessAfterCompletedFailureTest, TableIncrementApplicatorTests)
{
    auto applicator = TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<std::string> { 3, "Value2" });

    BOOST_CHECK_EQUAL(TableIncrementApplicatorState::Error, applicator.Process());
}

BOOST_FIXTURE_TEST_CASE(TestReset, TableIncrementApplicatorTests)
{
    auto applicator = TestMerge(
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<int> { 3, 10, 12 },
        Basis::Vector<std::string> { 3, "Value2" });

    applicator.Reset();

    NewSnapshot.clear();

    Init(applicator);
    TestMerge(1, applicator);
}

BOOST_FIXTURE_TEST_CASE(PartMergeTest, TableIncrementApplicatorTests)
{
    TestMerge<4>(
        Basis::Vector<int>
        {
            3, 10, 12, 15,
            17, 18, 20, 21,
            24, 27, 39, 45
        },
        Basis::Vector<int>
        {
            1, 2, 10, 11,
            20, 21, 24, 46,
            50, 52, 55, 60
        },
        Basis::Vector<int>
        {
            3, 4, 5, 10,
            24, 25, 40, 47,
            52, 53, 54, 56
        },
        Basis::Vector<int>
        {
            3, 4, 5, 10,
            12, 15, 17, 18,
            24, 25, 27, 39,
            40, 45, 47, 52,
            53, 54, 56
        },
        Basis::Vector<std::string>
        {
            "Value2", "Value2", "Value2", "Value2",
            "Value1", "Value1", "Value1", "Value1",
            "Value2", "Value2", "Value1", "Value1",
            "Value2", "Value1", "Value2", "Value2",
            "Value2", "Value2", "Value2"
        },
        8);
}

BOOST_AUTO_TEST_SUITE_END()
}
