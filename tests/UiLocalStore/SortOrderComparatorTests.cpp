#include "TableUtilsTests.hpp"

#include "UiLocalStore/SortOrderComparator.hpp"
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

BOOST_AUTO_TEST_SUITE(UiServer_SortOrderComparatorTests)

using namespace TradingSerialization::Table;

struct SortOrderComparatorTests : public BaseTestFixture, public TableUtilsTestsBase
{
    using TSetup = QuoterSettings::QuoterSettingsTableSetup;
    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TComp = SortOrderComparator<QuoterSettingsItem, QuoterSettings::ColumnType>;

    Basis::SPtr<Model::Location> Location1 = TestLocation("Moscow").BuildPtr();
    Basis::SPtr<Model::Location> Location2 = TestLocation("London").BuildPtr();
    Basis::SPtr<Model::Tag> Tag1 = TestTag("Tag1").BuildPtr();
    Basis::SPtr<Model::Tag> Tag2 = TestTag("Tag2").BuildPtr();
    Basis::SPtr<Model::Firm> OwnerFirm1 = TestFirm("Firm1").BuildPtr();
    Basis::SPtr<Model::Firm> OwnerFirm2 = TestFirm("Firm2").BuildPtr();
    Basis::SPtr<Model::InstrumentBasis> Instrument1 = TestInstrumentBasis(TestStock::Usd, TestStock::Rub).BuildPtr();
    Basis::SPtr<Model::InstrumentBasis> Instrument2 = TestInstrumentBasis(TestStock::Eur, TestStock::Usd).BuildPtr();
    Basis::SPtr<Model::Maker> Maker = TestMaker("Maker1").SetMakerType(Model::MakerType::Aggregator).BuildPtr();
    Basis::SPtr<Model::Source> Source1 = TestSource("Source1").SetMaker(Maker).BuildPtr();
    Basis::SPtr<Model::Source> Source2 = TestSource("Source2").SetMaker(Maker).BuildPtr();

    Basis::SPtr<Model::Settings::QuoteGroup> QuoteGroup1 = TestQuoteGroup(Group1)
        .SetId(Group1Id)
        .SetOwnerFirmId(OwnerFirm1->Id)
        .SetLocationId(Location1->Id)
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteGroup> QuoteGroup2 = TestQuoteGroup(Group5)
        .SetId(Group5Id)
        .SetOwnerFirmId(OwnerFirm2->Id)
        .SetLocationId(Location2->Id)
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentSettings> InstrumentSetting11 = TestQuoteInstrumentSettings(QuoteGroup1)
        .SetTagId(Tag1->Id)
        .SetSource(Source1)
        .SetInstrumentBasis(Instrument1)
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentSettings> InstrumentSetting12 = TestQuoteInstrumentSettings(QuoteGroup1)
        .SetTagId(Tag2->Id)
        .SetSource(Source2)
        .SetInstrumentBasis(Instrument1)
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentSettings> InstrumentSetting21 = TestQuoteInstrumentSettings(QuoteGroup2)
        .SetTagId(Tag1->Id)
        .SetSource(Source1)
        .SetInstrumentBasis(Instrument2)
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentAmountSettings> AmountSetting11
        = TestQuoteInstrumentAmountSettings(Basis::Money { 1000 })
        .SetQuoteInstrumentSettings(InstrumentSetting11)
        .SetSpreadMode(Model::Settings::SpreadMode::Fixed)
        .SetSpread(Basis::Real {5.6})
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentAmountSettings> AmountSetting12
        = TestQuoteInstrumentAmountSettings(Basis::Money { 2000 })
        .SetQuoteInstrumentSettings(InstrumentSetting12)
        .SetSpreadMode(Model::Settings::SpreadMode::Mixed)
        .SetMinSpread(Basis::Real {5.4})
        .BuildPtr();
    Basis::SPtr<Model::Settings::QuoteInstrumentAmountSettings> AmountSetting21
        = TestQuoteInstrumentAmountSettings(Basis::Money { 1000 })
        .SetQuoteInstrumentSettings(InstrumentSetting21)
        .SetSpreadMode(Model::Settings::SpreadMode::Fixed)
        .SetSpread(Basis::Real {10})
        .BuildPtr();

    Basis::SPtr<QuoterSettingsItem> Item11 = Basis::MakeSPtr<QuoterSettingsItem>(AmountSetting11, OwnerFirm1, Location1, Tag1);
    Basis::SPtr<QuoterSettingsItem> Item12 = Basis::MakeSPtr<QuoterSettingsItem>(AmountSetting12, OwnerFirm1, Location1, Tag2);
    Basis::SPtr<QuoterSettingsItem> Item21 = Basis::MakeSPtr<QuoterSettingsItem>(AmountSetting21, OwnerFirm2, Location2, Tag1);

    TSortOrder SortOrder;
    bool Ok = true;

    SortOrderComparatorTests()
    {
    }

    auto MakeComparator(const TSortOrder& aSortOrder)
    {
        SortOrder = aSortOrder;
        return TComp { SortOrder, Ok };
    }

    void CheckLess(
        const TComp& aComp,
        const Basis::SPtr<QuoterSettingsItem>& aLhs,
        const Basis::SPtr<QuoterSettingsItem>& aRhs,
        bool aLess = true,
        bool aOk = true)
    {
        BOOST_CHECK_EQUAL(aLess, aComp(aLhs, aRhs));
        BOOST_CHECK_EQUAL(aOk, Ok);
    }
};

BOOST_FIXTURE_TEST_CASE(OrderByIdTests, SortOrderComparatorTests)
{
    auto comp = MakeComparator(TSortOrder { static_cast<TColumnType>(QuoterSettings::ColumnType::Id) });
    BOOST_TEST_CONTEXT("Item11 < Item12") { CheckLess(comp, Item11, Item12, true); }
    BOOST_TEST_CONTEXT("Item12 >= Item11") { CheckLess(comp, Item12, Item11, false); }

    BOOST_TEST_CONTEXT("Item21 >= Item11") { CheckLess(comp, Item21, Item11, false); }
    BOOST_TEST_CONTEXT("Item11 < Item21") { CheckLess(comp, Item11, Item21, true); }

    BOOST_TEST_CONTEXT("Item21 >= Item12") { CheckLess(comp, Item21, Item12, false); }
    BOOST_TEST_CONTEXT("Item12 < Item21") { CheckLess(comp, Item12, Item21, true); }
}

BOOST_FIXTURE_TEST_CASE(OrderByAmountTests, SortOrderComparatorTests)
{
    auto comp = MakeComparator(TSortOrder { static_cast<TColumnType>(QuoterSettings::ColumnType::Amount) });
    BOOST_TEST_CONTEXT("Item11 < Item12") { CheckLess(comp, Item11, Item12, true); }
    BOOST_TEST_CONTEXT("Item12 >= Item11") { CheckLess(comp, Item12, Item11, false); }

    BOOST_TEST_CONTEXT("Item21 >= Item11") { CheckLess(comp, Item21, Item11, false); }
    BOOST_TEST_CONTEXT("Item11 >= Item21") { CheckLess(comp, Item11, Item21, false); }

    BOOST_TEST_CONTEXT("Item21 < Item12") { CheckLess(comp, Item21, Item12, true); }
    BOOST_TEST_CONTEXT("Item12 >= Item21") { CheckLess(comp, Item12, Item21, false); }
}

BOOST_FIXTURE_TEST_CASE(OrderByAmountAndIdTests, SortOrderComparatorTests)
{
    auto comp = MakeComparator(TSortOrder
    {
        static_cast<TColumnType>(QuoterSettings::ColumnType::Amount),
        static_cast<TColumnType>(QuoterSettings::ColumnType::Id)
    });
    BOOST_TEST_CONTEXT("Item11 < Item12") { CheckLess(comp, Item11, Item12, true); }
    BOOST_TEST_CONTEXT("Item12 >= Item11") { CheckLess(comp, Item12, Item11, false); }

    BOOST_TEST_CONTEXT("Item21 >= Item11") { CheckLess(comp, Item21, Item11, false); }
    BOOST_TEST_CONTEXT("Item11 < Item21") { CheckLess(comp, Item11, Item21, true); }

    BOOST_TEST_CONTEXT("Item21 < Item12") { CheckLess(comp, Item21, Item12, true); }
    BOOST_TEST_CONTEXT("Item12 >= Item21") { CheckLess(comp, Item12, Item21, false); }
}

BOOST_FIXTURE_TEST_CASE(EmptyOrderTests, SortOrderComparatorTests)
{
    auto comp = MakeComparator(TSortOrder {});
    BOOST_TEST_CONTEXT("Item11 >= Item12") { CheckLess(comp, Item11, Item12, false); }
    BOOST_TEST_CONTEXT("Item12 >= Item11") { CheckLess(comp, Item12, Item11, false); }

    BOOST_TEST_CONTEXT("Item21 >= Item11") { CheckLess(comp, Item21, Item11, false); }
    BOOST_TEST_CONTEXT("Item11 >= Item21") { CheckLess(comp, Item11, Item21, false); }

    BOOST_TEST_CONTEXT("Item21 >= Item12") { CheckLess(comp, Item21, Item12, false); }
    BOOST_TEST_CONTEXT("Item12 >= Item21") { CheckLess(comp, Item12, Item21, false); }
}

BOOST_AUTO_TEST_SUITE_END()
}
