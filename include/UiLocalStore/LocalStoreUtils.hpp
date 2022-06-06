#pragma once

#include <Common/Pack.hpp>

#include <Trading/Model/ActionType.hpp>

#include "TradingSerialization/Table/Columns.hpp"
#include "TradingSerialization/Table/Subscription.hpp"

#include "NewUiServer/UiLocalStore/ITableProcessorApi.hpp"
#include "NewUiServer/UiLocalStore/ITableSorter.hpp"
#include <NewUiServer/UiLocalStore/TableUtils.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Набор функций и классов для фильтрации и сортировки данных в UiLocalStore.
 * \ingroup NewUiServer
 */
struct LocalStoreUtils
{
    using TUiColumnType = TradingSerialization::Table::TColumnType;
    using TUiFilter = TradingSerialization::Table::Filter;
    using TUiFilters = TradingSerialization::Table::FilterGroup;
    using TFilterVariant = TradingSerialization::Table::TFilterVariant;
    using TUiFilterValues = TradingSerialization::Table::TFilterValues;
    using TFilterOperator = TradingSerialization::Table::FilterOperator;
    using TSortOrder = TradingSerialization::Table::TSortOrder;

    template <typename TComparer>
    class CompareVisitor
    {
        const TFilterVariant& mFieldValue;
        TComparer mComparer;
        bool& mOk;

    public:
        CompareVisitor(const TFilterVariant& aFieldValue, TComparer aComparer, bool& outOk)
            : mFieldValue(aFieldValue)
            , mComparer(aComparer)
            , mOk(outOk)
        {}

        template <typename TFilterType>
        bool operator ()(const TFilterType& aFilter)
        {
            auto* pValue = boost::get<TFilterType>(&mFieldValue);
            if (pValue)
            {
                return mComparer(*pValue, aFilter);
            }
            else
            {
                mOk = false;
                return false;
            }
        }
    };

    template <typename TComparator>
    static bool CheckInternal(
        const LocalStoreUtils::TFilterVariant& aLhs,
        const LocalStoreUtils::TFilterVariant& aRhs,
        bool& outOk)
    {
        return boost::apply_visitor(CompareVisitor { aLhs, TComparator {}, outOk }, aRhs);
    }

    template <template <typename> class TComparator>
    struct VariantFunc
    {
        template <typename T>
        bool operator()(const T& aX, const T& aY) const
        {
            using namespace TradingSerialization::Table;

            if constexpr (std::is_same<T, TFloat>::value)
            {
                if (aX && aY)
                {
                    return TComparator {} (Basis::Real { *aX }, Basis::Real { *aY });
                }
            }
            return TComparator {} (aX, aY);
        }
    };

    /**
     * \brief Проверить объект на соответствие одному фильтру.
     * outOk - принимает false в случае ошибок применения фильтра.
     */
    static bool CheckOperation(
        const TUiFilterValues& aFilterValues,
        const TFilterVariant& aFieldValue,
        TFilterOperator aOperator,
        bool& outOk);

    /**
     * \brief Функции сравнения вариантов.
     */
    static bool CheckEqual(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk);
    static bool CheckGreaterEq(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk);
    static bool CheckLessEq(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk);
    static bool CheckGreater(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk);
    static bool CheckLess(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk);

    static bool CheckInValues(const TUiFilterValues& aFilterValues, const TFilterVariant& aFieldValue, bool& outOk);
    static bool CheckGreaterEqValues(const TUiFilterValues& aFilterValues, const TFilterVariant& aFieldValue, bool& outOk);
    static bool CheckLessEqValues(const TUiFilterValues& aFilterValues, const TFilterVariant& aFieldValue, bool& outOk);
    static bool CheckGreaterValues(const TUiFilterValues& aFilterValues, const TFilterVariant& aFieldValue, bool& outOk);
    static bool CheckLessValues(const TUiFilterValues& aFilterValues, const TFilterVariant& aFieldValue, bool& outOk);

    // Функция возвращает опциональное целочисленное значение из опционального фильтра
    // Возвращает ошибку, если в параметрах не одно значение.
    static bool ExtractIntBound(
        const std::optional<TradingSerialization::Table::Filter>& aBound,
        TradingSerialization::Table::TInt& outBound,
        std::string& aError);

    static std::pair<
        TradingSerialization::Table::TInt,
        TradingSerialization::Table::TInt> ExtractRangeFilters(
            const TUiFilters& aFilters,
            TUiColumnType aColumn,
            std::string& aError);

    static TradingSerialization::Table::TInt ExtractSingleIntFilter(
            const TUiFilters& aFilters,
            TUiColumnType aColumn,
            std::string& aError);

    static TradingSerialization::Table::TIntValues ExtractMultipleIntFilters(
            const TUiFilters& aFilters,
            TUiColumnType aColumn,
            std::string& aError);

    static bool MakeSqlSingleIntFilter(
        const TUiFilters& aFilters,
        TUiColumnType aColumn,
        const std::string& aDbColumn,
        std::vector<std::string>& outFilters,
        std::string& aError);

    static bool MakeSqlMultipleIntFilter(
        const TUiFilters& aFilters,
        TUiColumnType aColumn,
        const std::string& aDbColumn,
        std::vector<std::string>& outFilters,
        std::string& aError);

    /**
     * \brief Проверить объект на соответствие фильтрам.
     * outOk - принимает false в случае ошибок применения фильтров.
     */
    template <typename TTableSetup, typename TData>
    static bool CheckDataByFilter(const TData& aData, const TUiFilters& aFilters, bool& outOk)
    {
        using namespace TradingSerialization::Table;

        if (aFilters.Relation == FilterRelation::And)
        {
            for (const auto& filter : aFilters.Filters)
            {
                if (!TableUtils<TTableSetup>::ColumnTypeIsValid(filter.Column))
                {
                    outOk = false;
                    return false;
                    
                }

                auto fieldValue = aData.GetValue(static_cast<typename TData::TColumnType>(filter.Column));
                if (!CheckOperation(filter.Values.GetAllValues(), fieldValue, filter.Operator, outOk) || !outOk)
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            outOk = false;
            return false;
        }
    }
};

template <typename TData>
struct DataIncrement
{
    using TDataPack = Basis::Pack<TData>;

    TDataPack Deleted;
    TDataPack Added;

    void Clear()
    {
        Deleted.clear();
        Added.clear();
    }

    size_t GetSize() const
    {
        return Deleted.size() + Added.size();
    }
};

}
