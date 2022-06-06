#pragma once

#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include "UiLocalStore/TableUtils.hpp"
#include "TradingSerialization/Table/DataTables/QuoterSettingsTableColumns.hpp"

#include <boost/test/unit_test.hpp>

namespace NTPro::Ecn::NewUiServer
{

struct TableUtilsTestsBase
{
    static constexpr TradingSerialization::Table::QuoterSettings::ColumnType MaxColumnType
        = TradingSerialization::Table::QuoterSettings::ColumnType::MaxSpread;

    static constexpr char Group1[] = "Group1";
    static constexpr char Group2[] = "Group2";
    static constexpr char Group5[] = "Group5";

    static constexpr int64_t Group1Id = 1;
    static constexpr int64_t Group5Id = 5;

    static void Compare(
        const TradingSerialization::Table::ColumnsInfo& aLhs,
        const TradingSerialization::Table::ColumnsInfo& aRhs)
    {
        BOOST_CHECK_EQUAL(aLhs.Column, aRhs.Column);
        BOOST_CHECK_EQUAL(aLhs.Type, aRhs.Type);
    }

    static void Compare(
        const TradingSerialization::Table::ColumnsInfo& aLhs,
        const TradingSerialization::Table::ColumnsInfo* aRhs)
    {
        BOOST_REQUIRE(aRhs);
        Compare(aLhs, *aRhs);
    }

    static auto MakeIntFilter()
    {
        using namespace TradingSerialization::Table;

        Filter filter;
        filter.Column = static_cast<TColumnType>(QuoterSettings::ColumnType::GroupId);
        filter.Operator = FilterOperator::In;
        filter.Values.Add(TInt {Group1Id});
        filter.Values.Add(TInt {Group5Id});
        return filter;
    }

    static auto MakeStringFilter()
    {
        using namespace TradingSerialization::Table;

        Filter filter;
        filter.Column = static_cast<TColumnType>(QuoterSettings::ColumnType::GroupName);
        filter.Operator = FilterOperator::In;
        filter.Values.Add(TString {Group1});
        filter.Values.Add(TString {Group2});
        return filter;
    }

    static auto MakerFilterGroup()
    {
        using namespace TradingSerialization::Table;

        FilterGroup group;
        group.Filters.insert(MakeIntFilter());
        group.Filters.insert(MakeStringFilter());
        group.Relation = FilterRelation::And;
        return group;
    }

    static auto MakeSortOrder()
    {
        using namespace TradingSerialization::Table;

        return TSortOrder
        {
            static_cast<TColumnType>(QuoterSettings::ColumnType::OwnerFirmName),
            static_cast<TColumnType>(QuoterSettings::ColumnType::GroupName)
        };
    }

    static auto MakeSubscription()
    {
        using namespace TradingSerialization::Table;

        SubscribeBase subscription;
        subscription.FilterExpression = MakerFilterGroup();
        subscription.SortOrder = MakeSortOrder();
        return subscription;
    }

};

}
