#pragma once

#include <boost/mpl/find.hpp>

#include <Common/Collections.hpp>
#include <Common/Pack.hpp>
#include <Common/StringUtils.hpp>

#include "TradingSerialization/Table/Columns.hpp"
#include "TradingSerialization/Table/Subscription.hpp"

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Набор функций для валидации фильтров.
 * \ingroup NewUiServer
 */
template <typename TTableSetup>
class TableUtils
{
    using TUiFilterOperator = TradingSerialization::Table::FilterOperator;
    using TUiSubscription = TradingSerialization::Table::SubscribeBase;
    using TTableColumnType = typename TTableSetup::TTableColumnType;

public:
    static constexpr char Ok[] = "";
    static constexpr char IdColumnIsAbsent[] = "Sort order must contain Id column";

    /**
     * \brief Получить индекс аргумента, заданного в варианте.
     * \ingroup NewUiServer
     */
    template <typename TColumnMplVector, typename TType>
    static constexpr auto GetColumnIndex()
    {
        return boost::mpl::find<TColumnMplVector, TType>::type::pos::value;
    }

    /**
     * \brief Получить тип аргумента.
     * \ingroup NewUiServer
     */
    template <typename TType>
    static constexpr TradingSerialization::Table::FilterValueType GetFilterValueIndex()
    {
        using namespace TradingSerialization::Table;

        const auto result = static_cast<FilterValueType>(GetColumnIndex<TFilterMplVector, TType>());
        if (result < FilterValueType::String || result > FilterValueType::Unknown)
        {
            return FilterValueType::Unknown;
        }
        return result;
    }

    /**
     * \brief Визитор для получения типа аргумента, заданного в варианте.
     * \ingroup NewUiServer
     */
    class FilterTypeVisitor
    {
    public:
        template <typename TValue>
        TradingSerialization::Table::FilterValueType operator()(const TValue&)
        {
            return GetFilterValueIndex<TValue>();
        }
    };

    template <typename TValue, typename TMinValue, typename TMaxValue>
    static bool ValidateEnumValue(TValue aValue, TMinValue aMinValue, TMaxValue aMaxValue)
    {
        return (aValue >= static_cast<TValue>(aMinValue) && (aValue <= static_cast<TValue>(aMaxValue)));
    }

    static bool IsOperationAllowed(
        TradingSerialization::Table::FilterValueType aType,
        TradingSerialization::Table::FilterOperator aOperator)
    {
        using namespace TradingSerialization::Table;

        switch (aType)
        {
        case FilterValueType::String:
            return aOperator == FilterOperator::In;
        case FilterValueType::Int:
            [[fallthrough]];
        case FilterValueType::Float:
            return aOperator == FilterOperator::In
            || aOperator == FilterOperator::GreaterEq
            || aOperator == FilterOperator::LessEq
            || aOperator == FilterOperator::Greater
            || aOperator == FilterOperator::Less;
        default:
            return false;
        }
    }

    static bool ColumnTypeIsValid(
        TradingSerialization::Table::TColumnType aColumn)
    {
        return TableUtils::ValidateEnumValue(aColumn, TTableSetup::ColumnsRange[0], TTableSetup::ColumnsRange[1]);
    }

    static bool FilterColumnTypeIsValid(TradingSerialization::Table::TColumnType aColumn)
    {
        using namespace TradingSerialization::Table;

        return TableUtils::ValidateEnumValue(aColumn, Filters::FullTextSearch, Filters::FullTextSearch);
    }

    static std::string ValidateColumn(
    TradingSerialization::Table::TColumnType aColumn)
    {
        if (!ColumnTypeIsValid(aColumn))
        {
            return "Wrong column number: " + Basis::ToString(aColumn);
        }

        return Ok;
    }

    static std::string ValidateFilterColumn(
    TradingSerialization::Table::TColumnType aColumn)
    {
        auto error = ValidateColumn(aColumn);
        if (error == Ok)
        {
            return Ok;
        }

        if (!FilterColumnTypeIsValid(aColumn))
        {
            return "Wrong filter column number: " + Basis::ToString(aColumn);
        }

        return Ok;
    }

    static std::string ValidateRelation(TradingSerialization::Table::FilterRelation aRelation)
    {
        using namespace TradingSerialization::Table;

        if (!ValidateEnumValue(aRelation, FilterRelation::And, FilterRelation::And))
        {
            return "Wrong filter relation: " + Basis::ToString(static_cast<int>(aRelation));
        }

        return Ok;
    }

    static std::string ValidateOperator(TradingSerialization::Table::FilterOperator aOperator)
    {
        using namespace TradingSerialization::Table;

        if (!ValidateEnumValue(aOperator, FilterOperator::In, FilterOperator::LessEq))
        {
            return "Wrong filter operator: " + Basis::ToString(static_cast<int>(aOperator));
        }

        return Ok;
    }

    static size_t GetMaxFilterValuesNumberForOperator(TradingSerialization::Table::FilterOperator aOperator)
    {
        using namespace TradingSerialization::Table;

        if (aOperator == FilterOperator::In)
        {
            return std::numeric_limits<size_t>::max();
        }
        return 1;
    }

    static const TradingSerialization::Table::ColumnsInfo* GetColumnsInfo(
    TradingSerialization::Table::TColumnType aColumn)
    {
        using namespace TradingSerialization::Table;

        if (ColumnTypeIsValid(aColumn))
        {
            return &TTableSetup::FieldList[aColumn];
        }

        if (FilterColumnTypeIsValid(aColumn))
        {
            return &FilterFieldList[aColumn - StartFilterValue];
        }

        return nullptr;
    }

    static std::string ValidateFilter(const TradingSerialization::Table::Filter& aFilter)
    {
        using namespace TradingSerialization::Table;

        auto error = ValidateOperator(aFilter.Operator);
        if (!error.empty())
        {
            return error;
        }

        if (aFilter.Values.IsEmpty())
        {
            return std::string { "Filter empty" };
        }

        if (aFilter.Values.Size() > GetMaxFilterValuesNumberForOperator(aFilter.Operator))
        {
            return std::string { "Too many filter values" };
        }

        const ColumnsInfo* columnInfo = GetColumnsInfo(aFilter.Column);
        if (!columnInfo)
        {
            return std::string { "Wrong column" };
        }

        for (const auto& value : aFilter.Values.GetAllValues())
        {
            auto filterType = boost::apply_visitor(FilterTypeVisitor {}, value);
            if (filterType == FilterValueType::Unknown)
            {
                return std::string { "Unknown filter type" };
            }

            if (columnInfo->Type != filterType)
            {
                return std::string { "Wrong filter type" };
            }

            if (!IsOperationAllowed(filterType, aFilter.Operator))
            {
                return "Operator " + Basis::ToString(aFilter.Operator)
                + " not allowed for type " + Basis::ToString(filterType);
            }
        }

        return Ok;
    }

    static std::string ValidateFilterGroup(const TradingSerialization::Table::FilterGroup& aFilterGroup)
    {
        using namespace TradingSerialization::Table;

        auto error = ValidateRelation(aFilterGroup.Relation);
        if (!error.empty())
        {
            return error;
        }

        if (aFilterGroup.Filters.size() > FilterGroup::MaxCount)
        {
            return "Too many filters: " + Basis::ToString(aFilterGroup.Filters.size());
        }

        for (const auto& filter : aFilterGroup.Filters)
        {
            error = ValidateFilterColumn(filter.Column);
            if (!error.empty())
            {
                return error;
            }

            error = ValidateFilter(filter);
            if (!error.empty())
            {
                return error;
            }
        }

        return Ok;
    }

    static std::string CheckSortOrder(const TradingSerialization::Table::TSortOrder& aSortOrder)
    {
        using namespace TradingSerialization::Table;

        Basis::Set<TColumnType> columns;

        for (const auto column : aSortOrder)
        {
            auto error = ValidateColumn(column);
            if (!error.empty())
            {
                return error;
            }
            if (!columns.emplace(column).second)
            {
                return "Sort column duplicated: " + Basis::ToString(static_cast<TTableColumnType>(column));
            }
        }

        const auto idColumn = static_cast<TColumnType>(TTableSetup::IdColumn);
        if (!columns.count(idColumn))
        {
            return IdColumnIsAbsent;
        }

        return Ok;
    }

    static std::string ValidateSortOrder(TradingSerialization::Table::TSortOrder& outSortOrder)
    {
        using namespace TradingSerialization::Table;

        auto error = CheckSortOrder(outSortOrder);
        if (!error.empty())
        {
            if (error == IdColumnIsAbsent)
            {
                const auto idColumn = static_cast<TColumnType>(TTableSetup::IdColumn);
                outSortOrder.push_back(idColumn);
                return CheckSortOrder(outSortOrder);
            }
        }

        return error;
    }

    static std::string ValidateSubscription(TUiSubscription& outSubscriptionInfo)
    {
        using namespace TradingSerialization::Table;

        auto error = ValidateSortOrder(outSubscriptionInfo.SortOrder);
        if (!error.empty())
        {
            return error;
        }
        return ValidateFilterGroup(outSubscriptionInfo.FilterExpression);
    }

    template <typename TData>
    static TradingSerialization::Table::ValueSet GetItemValues(const TData& aData)
    {
        TradingSerialization::Table::ValueSet result;
        for (const auto& field : TTableSetup::FieldList)
        {
            boost::apply_visitor(
                [&result](const auto& aValue) { result.Add(aValue); },
                aData.GetValue(static_cast<TTableColumnType>(field.Column)));
        }

        return result;
    }

    static TradingSerialization::Table::Filter CreateSingleIntFilter(
        TTableColumnType aColumn,
        int64_t aFilter,
        TUiFilterOperator aOp = TUiFilterOperator::In)
    {
        using namespace TradingSerialization::Table;

        ValueSet filters;
        filters.IntValues.push_back(aFilter);

        return TradingSerialization::Table::Filter(
            static_cast<TColumnType>(aColumn),
            filters,
            aOp,
            false);
    }

    static TradingSerialization::Table::Filter CreateSingleStringFilter(
        TTableColumnType aColumn,
        const std::string& aFilter,
        TUiFilterOperator aOp = TUiFilterOperator::In)
    {
        using namespace TradingSerialization::Table;

        ValueSet filters;
        filters.StringValues.push_back(aFilter);

        return TradingSerialization::Table::Filter(
            static_cast<TColumnType>(aColumn),
            filters,
            aOp,
            false);
    }

    template <typename TIterator>
    static TradingSerialization::Table::Filter CreateMultiIntFilter(
        TTableColumnType aColumn,
        TIterator aBegin,
        TIterator aEnd)
    {
        using namespace TradingSerialization::Table;

        ValueSet filters;
        filters.IntValues.reserve(std::distance(aBegin, aEnd));
        std::transform(
            aBegin,
            aEnd,
            std::back_inserter(filters.IntValues),
            [](auto a) { return static_cast<int64_t>(a); });

        return TradingSerialization::Table::Filter(
            static_cast<TColumnType>(aColumn),
            filters,
            FilterOperator::In,
            false);
    }
};
}
