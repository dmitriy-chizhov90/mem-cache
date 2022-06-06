#include "TableUtilsTests.hpp"

#include "UiLocalStore/LocalStoreUtils.hpp"
#include "UiLocalStore/QuoterSettingsTable/QuoterSettingsItem.hpp"
#include "TradingSerialization/Table/Columns.hpp"

#include "TradingTesting/TestDataBuilders/TestQuoteInstrumentAmountSettings.hpp"
#include "TradingTesting/TestDataBuilders/TestQuoteInstrumentSettings.hpp"
#include "TradingTesting/TestDataBuilders/TestQuoteGroup.hpp"
#include "TradingTesting/TestDataBuilders/TestFirm.hpp"
#include "TradingTesting/TestDataBuilders/TestLocation.hpp"
#include "TradingTesting/TestDataBuilders/TestInstrumentBasis.hpp"
#include "TradingTesting/TestDataBuilders/TestTag.hpp"
#include "TradingTesting/TestDataBuilders/TestSource.hpp"
#include "TradingTesting/TestDataBuilders/TestMaker.hpp"
#include "TradingTesting/TestDataBuilders/TestPlatform.hpp"

#include "TradingTesting/TestStock/Assets.hpp"

#include <Basis/BaseTestFixture.hpp>
#include <Common/Fake.hpp>
#include <Common/Pack.hpp>


namespace NTPro::Ecn::NewUiServer
{

BOOST_AUTO_TEST_SUITE(UiServer_LocalStoreUtilsTests)

using namespace TradingSerialization::Table;

struct LocalStoreUtilsTests : public BaseTestFixture, public TableUtilsTestsBase
{
    using TSetup = QuoterSettings::QuoterSettingsTableSetup;

    Basis::SPtr<Model::Location> Location = TestLocation("Moscow").BuildPtr();
    Basis::SPtr<Model::Tag> Tag = TestTag("Tag1").BuildPtr();
    Basis::SPtr<Model::Firm> OwnerFirm = TestFirm("Firm1").BuildPtr();
    Basis::SPtr<Model::InstrumentBasis> Instrument = TestInstrumentBasis(TestStock::Usd, TestStock::Rub).BuildPtr();
    Basis::SPtr<Model::Maker> Maker = TestMaker("Maker1").SetMakerType(Model::MakerType::Aggregator).BuildPtr();
    Basis::SPtr<Model::Source> Source = TestSource("Source1").SetMaker(Maker).BuildPtr();

    Basis::SPtr<Model::Settings::QuoteGroup> QuoteGroup1 = TestQuoteGroup(Group1)
        .SetId(Group1Id)
        .SetOwnerFirmId(OwnerFirm->Id)
        .SetLocationId(Location->Id)
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentSettings> InstrumentSetting1 = TestQuoteInstrumentSettings(QuoteGroup1)
        .SetTagId(Tag->Id)
        .SetSource(Source)
        .SetInstrumentBasis(Instrument)
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentAmountSettings> AmountSetting1
        = TestQuoteInstrumentAmountSettings(Basis::Money { 1000 })
        .SetQuoteInstrumentSettings(InstrumentSetting1)
        .SetSpreadMode(Model::Settings::SpreadMode::Fixed)
        .SetSpread(Basis::Real {5.6})
        .BuildPtr();

    QuoterSettingsItem Item1 { AmountSetting1, OwnerFirm, Location, Tag };

    LocalStoreUtilsTests()
    {
    }

    template <typename TFunc>
    void TestCheckOperation(TFunc aFunc, const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool aResult, bool aOk)
    {
        bool ok = true;
        BOOST_CHECK_EQUAL(aResult, aFunc(aLhs, aRhs, ok));
        BOOST_CHECK_EQUAL(aOk, ok);
    }

    void TestCheckEqual(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool aResult, bool aOk)
    {
        TestCheckOperation(LocalStoreUtils::CheckEqual, aLhs, aRhs, aResult, aOk);
    }

    void TestCheckLessEq(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool aResult, bool aOk)
    {
        TestCheckOperation(LocalStoreUtils::CheckLessEq, aLhs, aRhs, aResult, aOk);
    }

    void TestCheckGreaterEq(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool aResult, bool aOk)
    {
        TestCheckOperation(LocalStoreUtils::CheckGreaterEq, aLhs, aRhs, aResult, aOk);
    }

    template <typename TFunc>
    void TestCheckOperation(
        TFunc aFunc,
        const TFilterValues& aFilters,
        const TFilterVariant& aRhs,
        bool aResult,
        bool aOk)
    {
        bool ok = true;
        BOOST_CHECK_EQUAL(aResult, aFunc(aFilters, aRhs, ok));
        BOOST_CHECK_EQUAL(aOk, ok);
    }

    void TestCheckIn(const TFilterValues& aFilterValues, const TFilterVariant& aRhs, bool aResult, bool aOk)
    {
        TestCheckOperation(LocalStoreUtils::CheckInValues, aFilterValues, aRhs, aResult, aOk);
    }

    void TestCheckOperation(
        const TFilterValues& aFilters,
        const TFilterVariant& aRhs,
        FilterOperator aOperator,
        bool aResult,
        bool aOk)
    {
        bool ok = true;
        BOOST_CHECK_EQUAL(aResult, LocalStoreUtils::CheckOperation(aFilters, aRhs, aOperator, ok));
        BOOST_CHECK_EQUAL(aOk, ok);
    }
};

BOOST_FIXTURE_TEST_CASE(CheckEqual, LocalStoreUtilsTests)
{
    BOOST_TEST_CONTEXT("Equal")
    {
        TestCheckEqual(TString {"string"}, TString {"string"}, true, true);
        TestCheckEqual(TInt {100}, TInt {100}, true, true);
        TestCheckEqual(TFloat {0.00001}, TFloat {0.00001}, true, true);
    }

    BOOST_TEST_CONTEXT("Not Equal")
    {
        TestCheckEqual(TString {"one"}, TString {"other"}, false, true);
        TestCheckEqual(TInt {100}, TInt {200}, false, true);
        TestCheckEqual(TFloat {0.00001}, TFloat {0.00002}, false, true);
    }

    BOOST_TEST_CONTEXT("Incorrect")
    {
        TestCheckEqual(TString {"one"}, TInt {200}, false, false);
        TestCheckEqual(TInt {100}, TFloat {0.00002}, false, false);
        TestCheckEqual(TFloat {0.00001}, TString {"one"}, false, false);
    }

    BOOST_TEST_CONTEXT("Equal none")
    {
        TestCheckEqual(TString {std::nullopt}, TString {std::nullopt}, true, true);
        TestCheckEqual(TInt {std::nullopt}, TInt {std::nullopt}, true, true);
        TestCheckEqual(TFloat {std::nullopt}, TFloat {std::nullopt}, true, true);
    }

    BOOST_TEST_CONTEXT("Not Equal none")
    {
        TestCheckEqual(TString {""}, TString {std::nullopt}, false, true);
        TestCheckEqual(TInt {0}, TInt {std::nullopt}, false, true);
        TestCheckEqual(TFloat {0}, TFloat {std::nullopt}, false, true);
    }

    BOOST_TEST_CONTEXT("Real")
    {
        TestCheckEqual(TFloat {0.00001}, TFloat {0.00001000000001}, true, true);
        TestCheckEqual(TFloat {0.00001}, TFloat {0.00000999999999}, true, true);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckLess, LocalStoreUtilsTests)
{
    BOOST_TEST_CONTEXT("Less")
    {
        TestCheckLessEq(TString {"alfa"}, TString {"betta"}, true, true);
        TestCheckLessEq(TInt {100}, TInt {200}, true, true);
        TestCheckLessEq(TFloat {0.00001}, TFloat {0.00002}, true, true);
    }

    BOOST_TEST_CONTEXT("Equal")
    {
        TestCheckLessEq(TString {"string"}, TString {"string"}, true, true);
        TestCheckLessEq(TInt {100}, TInt {100}, true, true);
        TestCheckLessEq(TFloat {0.00001}, TFloat {0.00001}, true, true);
    }

    BOOST_TEST_CONTEXT("Greater")
    {
        TestCheckLessEq(TString {"betta"}, TString {"alfa"}, false, true);
        TestCheckLessEq(TInt {200}, TInt {100}, false, true);
        TestCheckLessEq(TFloat {0.00002}, TFloat {0.00001}, false, true);
    }

    BOOST_TEST_CONTEXT("Incorrect")
    {
        TestCheckLessEq(TString {"one"}, TInt {200}, true, false);
        TestCheckLessEq(TInt {100}, TFloat {0.00002}, true, false);
        TestCheckLessEq(TFloat {0.00001}, TString {"other"}, true, false);
    }

    BOOST_TEST_CONTEXT("Less with none")
    {
        TestCheckLessEq(TString {std::nullopt}, TString {""}, true, true);
        TestCheckLessEq(TInt {std::nullopt}, TInt {0}, true, true);
        TestCheckLessEq(TFloat {std::nullopt}, TFloat {0}, true, true);
    }

    BOOST_TEST_CONTEXT("Real")
    {
        TestCheckLessEq(TFloat {0.00001}, TFloat {0.00001000000001}, true, true);
        TestCheckLessEq(TFloat {0.00000999999999}, TFloat {0.00001}, true, true);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckGreater, LocalStoreUtilsTests)
{
    BOOST_TEST_CONTEXT("Greater")
    {
        TestCheckGreaterEq(TString {"betta"}, TString {"alfa"}, true, true);
        TestCheckGreaterEq(TInt {200}, TInt {100}, true, true);
        TestCheckGreaterEq(TFloat {0.00002}, TFloat {0.00001}, true, true);
    }

    BOOST_TEST_CONTEXT("Equal")
    {
        TestCheckGreaterEq(TString {"string"}, TString {"string"}, true, true);
        TestCheckGreaterEq(TInt {100}, TInt {100}, true, true);
        TestCheckGreaterEq(TFloat {0.00001}, TFloat {0.00001}, true, true);
    }

    BOOST_TEST_CONTEXT("Less")
    {
        TestCheckGreaterEq(TString {"alfa"}, TString {"betta"}, false, true);
        TestCheckGreaterEq(TInt {100}, TInt {200}, false, true);
        TestCheckGreaterEq(TFloat {0.00001}, TFloat {0.00002}, false, true);
    }

    BOOST_TEST_CONTEXT("Incorrect")
    {
        TestCheckGreaterEq(TString {"one"}, TInt {200}, true, false);
        TestCheckGreaterEq(TInt {100}, TFloat {0.00002}, true, false);
        TestCheckGreaterEq(TFloat {0.00001}, TString {"other"}, true, false);
    }

    BOOST_TEST_CONTEXT("Greater with none")
    {
        TestCheckGreaterEq(TString {""}, TString {std::nullopt}, true, true);
        TestCheckGreaterEq(TInt {0}, TInt {std::nullopt}, true, true);
        TestCheckGreaterEq(TFloat {0}, TFloat {std::nullopt}, true, true);
    }

    BOOST_TEST_CONTEXT("Real")
    {
        TestCheckGreaterEq(TFloat {0.00001000000001}, TFloat {0.00001}, true, true);
        TestCheckGreaterEq(TFloat {0.00001}, TFloat {0.00000999999999}, true, true);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckInOneValue, LocalStoreUtilsTests)
{
    BOOST_TEST_CONTEXT("In")
    {
        TestCheckIn(
            TFilterValues { TString {"one"}, TString {"other"} },
            TString {"other"},
            true,
            true);
    }

    BOOST_TEST_CONTEXT("Not In")
    {
        TestCheckIn(
            TFilterValues { TString {"one"}, TString {"other"} },
            TString {"third"},
            false,
            true);
    }

    BOOST_TEST_CONTEXT("Incorrect")
    {
        TestCheckIn(
            TFilterValues { TString {"one"}, TInt {100} },
            TString {"third"},
            false,
            false);
    }

    BOOST_TEST_CONTEXT("Incorrect but found before")
    {
        TestCheckIn(
            TFilterValues { TString {"one"}, TInt {100} },
            TString {"one"},
            true,
            true);
    }

    BOOST_TEST_CONTEXT("Incorrect and found after")
    {
        TestCheckIn(
            TFilterValues { TInt {100}, TString {"one"} },
            TString {"one"},
            false,
            false);
    }

    BOOST_TEST_CONTEXT("Empty")
    {
        TestCheckIn(
            TFilterValues {},
            TString {"one"},
            false,
            true);
    }
}

BOOST_FIXTURE_TEST_CASE(CheckOperation, LocalStoreUtilsTests)
{
    BOOST_TEST_CONTEXT("In")
    {
        TestCheckOperation(
            TFilterValues { TString {"one"}, TString {"other"} },
            TString {"other"},
            FilterOperator::In,
            true,
            true);
    }

    BOOST_TEST_CONTEXT("Less")
    {
        TestCheckOperation(
            TFilterValues { TInt {100} },
            TInt {99},
            FilterOperator::LessEq,
            true,
            true);
    }

    BOOST_TEST_CONTEXT("Greater")
    {
        TestCheckOperation(
            TFilterValues { TInt {100} },
            TInt {101},
            FilterOperator::GreaterEq,
            true,
            true);
    }

    BOOST_TEST_CONTEXT("Unknown Operation")
    {
        TestCheckOperation(
            TFilterValues { TInt {100} },
            TInt {101},
            static_cast<FilterOperator>(1000),
            false,
            false);
    }
}

BOOST_FIXTURE_TEST_CASE(QuoterSettingsItemGetDataTests, LocalStoreUtilsTests)
{
    BOOST_TEST_CONTEXT("Integer Id")
    {
        auto value = Item1.GetValue(QuoterSettings::ColumnType::GroupId);
        BOOST_CHECK_EQUAL(boost::get<TInt>(value), TInt{1});
    }

    BOOST_TEST_CONTEXT("Real")
    {
        auto value = Item1.GetValue(QuoterSettings::ColumnType::Amount);
        BOOST_CHECK_EQUAL(boost::get<TFloat>(value), TFloat{1000});
    }

    BOOST_TEST_CONTEXT("Optional Real")
    {
        auto value = Item1.GetValue(QuoterSettings::ColumnType::Spread);
        BOOST_CHECK_EQUAL(boost::get<TFloat>(value), TFloat{5.6});
    }

    BOOST_TEST_CONTEXT("Optional Real none")
    {
        auto value = Item1.GetValue(QuoterSettings::ColumnType::MinSpread);
        BOOST_CHECK_EQUAL(boost::get<TFloat>(value), TFloat{});
    }

    BOOST_TEST_CONTEXT("Enum")
    {
        auto value = Item1.GetValue(QuoterSettings::ColumnType::SpreadMode);
        BOOST_CHECK_EQUAL(boost::get<TInt>(value), TInt{0});
    }

    BOOST_TEST_CONTEXT("String")
    {
        auto value = Item1.GetValue(QuoterSettings::ColumnType::GroupName);
        BOOST_CHECK_EQUAL(boost::get<TString>(value), TString{Group1});
    }
}

BOOST_FIXTURE_TEST_CASE(CheckDataByFilterTests, LocalStoreUtilsTests)
{
    BOOST_TEST_CONTEXT("Ok")
    {
        bool ok = true;
        BOOST_CHECK(LocalStoreUtils::CheckDataByFilter<TSetup>(Item1, MakerFilterGroup(), ok));
        BOOST_CHECK(ok);
    }

    BOOST_TEST_CONTEXT("Not Ok")
    {
        bool ok = true;
        Item1.ModelData->QuoteInstrumentSettings->QuoteGroup.GetNonConstRefUnsafe().Id = 1000;
        BOOST_CHECK(!LocalStoreUtils::CheckDataByFilter<TSetup>(Item1, MakerFilterGroup(), ok));
        BOOST_CHECK(ok);
    }

    BOOST_TEST_CONTEXT("Invalid Column")
    {
        bool ok = true;
        auto group = MakerFilterGroup();
        auto filter = *group.Filters.begin();
        group.Filters.erase(group.Filters.begin());
        filter.Column = -1;
        group.Filters.emplace(filter);
        BOOST_CHECK(!LocalStoreUtils::CheckDataByFilter<TSetup>(Item1, group, ok));
        BOOST_CHECK(!ok);
    }

    BOOST_TEST_CONTEXT("Empty filters")
    {
        bool ok = true;
        auto group = MakerFilterGroup();
        group.Filters.clear();
        BOOST_CHECK(LocalStoreUtils::CheckDataByFilter<TSetup>(Item1, group, ok));
        BOOST_CHECK(ok);
    }

    BOOST_TEST_CONTEXT("Invalid relation")
    {
        bool ok = true;
        auto group = MakerFilterGroup();
        group.Relation = static_cast<FilterRelation>(-1);
        BOOST_CHECK(!LocalStoreUtils::CheckDataByFilter<TSetup>(Item1, group, ok));
        BOOST_CHECK(!ok);
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
