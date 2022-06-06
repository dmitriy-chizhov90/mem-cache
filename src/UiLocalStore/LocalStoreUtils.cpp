#include <NewUiServer/UiLocalStore/LocalStoreUtils.hpp>

namespace NTPro::Ecn::NewUiServer
{

using namespace TradingSerialization::Table;

bool LocalStoreUtils::CheckOperation(
    const TUiFilterValues& aFilterValues,
    const TFilterVariant& aFieldValue,
    TFilterOperator aOperator,
    bool& outOk)
{
    switch (aOperator)
    {
    case TFilterOperator::In:
        return CheckInValues(aFilterValues, aFieldValue, outOk);
    case TFilterOperator::GreaterEq:
        return CheckGreaterEqValues(aFilterValues, aFieldValue, outOk);
    case TFilterOperator::LessEq:
        return CheckLessEqValues(aFilterValues, aFieldValue, outOk);
    case TFilterOperator::Greater:
        return CheckGreaterValues(aFilterValues, aFieldValue, outOk);
    case TFilterOperator::Less:
        return CheckLessValues(aFilterValues, aFieldValue, outOk);
    }
    outOk = false;
    return false;
}

bool LocalStoreUtils::CheckEqual(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk)
{
    return CheckInternal<VariantFunc<std::equal_to>>(aLhs, aRhs, outOk);
}

bool LocalStoreUtils::CheckGreaterEq(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk)
{
    return !CheckInternal<VariantFunc<std::less>>(aLhs, aRhs, outOk);
}

bool LocalStoreUtils::CheckLessEq(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk)
{
    return !CheckInternal<VariantFunc<std::greater>>(aLhs, aRhs, outOk);
}

bool LocalStoreUtils::CheckGreater(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk)
{
    return CheckInternal<VariantFunc<std::greater>>(aLhs, aRhs, outOk);
}

bool LocalStoreUtils::CheckLess(const TFilterVariant& aLhs, const TFilterVariant& aRhs, bool& outOk)
{
    return CheckInternal<VariantFunc<std::less>>(aLhs, aRhs, outOk);
}

bool LocalStoreUtils::CheckInValues(
    const TUiFilterValues& aFilterValues,
    const TFilterVariant& aFieldValue,
    bool& outOk)
{
    for (const auto& filterVariant : aFilterValues)
    {
        if (CheckEqual(aFieldValue, filterVariant, outOk))
        {
            return true;
        }
        if (!outOk)
        {
            return false;
        }
    }

    return false;
}

bool LocalStoreUtils::CheckGreaterEqValues(
    const TUiFilterValues& aFilterValues,
    const TFilterVariant& aFieldValue,
    bool& outOk)
{
    if (aFilterValues.size() != 1)
    {
        assert(false);
        return false;
    }
    return CheckGreaterEq(aFieldValue, aFilterValues.at(0), outOk);
}

bool LocalStoreUtils::CheckLessEqValues(
    const TUiFilterValues& aFilterValues,
    const TFilterVariant& aFieldValue,
    bool& outOk)
{
    if (aFilterValues.size() != 1)
    {
        assert(false);
        return false;
    }
    return CheckLessEq(aFieldValue, aFilterValues.at(0), outOk);
}

bool LocalStoreUtils::CheckGreaterValues(
    const TUiFilterValues& aFilterValues,
    const TFilterVariant& aFieldValue,
    bool& outOk)
{
    if (aFilterValues.size() != 1)
    {
        assert(false);
        return false;
    }
    return CheckGreater(aFieldValue, aFilterValues.at(0), outOk);
}

bool LocalStoreUtils::CheckLessValues(
    const TUiFilterValues& aFilterValues,
    const TFilterVariant& aFieldValue,
    bool& outOk)
{
    if (aFilterValues.size() != 1)
    {
        assert(false);
        return false;
    }
    return CheckLess(aFieldValue, aFilterValues.at(0), outOk);
}
    
bool LocalStoreUtils::ExtractIntBound(
    const std::optional<TradingSerialization::Table::Filter>& aBound,
    TradingSerialization::Table::TInt& outBound,
    std::string& aError)
{
    outBound.reset();

    if (!aBound)
    {
        return true;
    }

    if (aBound->Values.IntValues.size() != 1)
    {
        aError = "Require one bound value";
        return false;
    }

    outBound = aBound->Values.IntValues[0];
    return true;
}
        
std::pair<
    TradingSerialization::Table::TInt,
    TradingSerialization::Table::TInt> LocalStoreUtils::ExtractRangeFilters(
        const LocalStoreUtils::TUiFilters& aFilters,
        LocalStoreUtils::TUiColumnType aColumn,
        std::string& aError)
{
    using namespace TradingSerialization::Table;

    std::pair<
        TradingSerialization::Table::TInt,
        TradingSerialization::Table::TInt> nullResult;
    TradingSerialization::Table::TInt gt, ls;

    assert(aFilters.Relation == FilterRelation::And);
    auto [it, end] = aFilters.Filters.equal_range(aColumn);

    for (; it != end; ++it)
    {
        if (it->Operator == FilterOperator::GreaterEq
            || it->Operator == FilterOperator::Greater)
        {
            if (!ExtractIntBound(*it, gt, aError))
            {
                return nullResult;
            }
        }
        else if (it->Operator == FilterOperator::LessEq
            || it->Operator == FilterOperator::Less)
        {
            if (!ExtractIntBound(*it, ls, aError))
            {
                return nullResult;
            }
        }
    }
    return std::make_pair(gt, ls);
}

TradingSerialization::Table::TInt LocalStoreUtils::ExtractSingleIntFilter(
    const LocalStoreUtils::TUiFilters& aFilters,
    LocalStoreUtils::TUiColumnType aColumn,
    std::string& aError)
{
    using namespace TradingSerialization::Table;

    TradingSerialization::Table::TInt result;

    assert(aFilters.Relation == FilterRelation::And);
    
    auto it = aFilters.Filters.find(aColumn);
    if (it == aFilters.Filters.cend()
        || it->Operator != FilterOperator::In)
    {
        return std::nullopt;
    }
    
    if (!ExtractIntBound(*it, result, aError))
    {
        return std::nullopt;
    }
    return result;
}

TradingSerialization::Table::TIntValues LocalStoreUtils::ExtractMultipleIntFilters(
    const LocalStoreUtils::TUiFilters& aFilters,
    LocalStoreUtils::TUiColumnType aColumn,
    std::string&)
{
    using namespace TradingSerialization::Table;

    assert(aFilters.Relation == FilterRelation::And);
    
    auto it = aFilters.Filters.find(aColumn);
    if (it == aFilters.Filters.cend()
        || it->Operator != FilterOperator::In)
    {
        return TIntValues {};
    }
    return it->Values.IntValues;
}

bool LocalStoreUtils::MakeSqlSingleIntFilter(
    const TUiFilters& aFilters,
    TUiColumnType aColumn,
    const std::string& aDbColumn,
    std::vector<std::string>& outFilters,
    std::string& aError)
{
    auto target = LocalStoreUtils::ExtractSingleIntFilter(aFilters, aColumn, aError);

    if (!aError.empty())
    {
        return false;
    }
    if (target)
    {
        outFilters.push_back(aDbColumn + "=" + Basis::ToString(*target));
    }
    return true;
}

bool LocalStoreUtils::MakeSqlMultipleIntFilter(
    const TUiFilters& aFilters,
    TUiColumnType aColumn,
    const std::string& aDbColumn,
    std::vector<std::string>& outFilters,
    std::string& aError)
{
    auto target = LocalStoreUtils::ExtractMultipleIntFilters(aFilters, aColumn, aError);

    if (!aError.empty())
    {
        return false;
    }
    if (!target.empty())
    {
        outFilters.push_back(aDbColumn + " IN (" + Basis::ContainerToStringPlain(target) + ")");
    }
    return true;
}

}
