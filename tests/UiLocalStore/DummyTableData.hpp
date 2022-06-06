#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <Common/Fake.hpp>
#include <Common/Pack.hpp>

#include "UiLocalStore/VersionedDataContainer.hpp"
#include "UiLocalStore/TableUtils.hpp"

#include <boost/test/unit_test.hpp>

#include "UiLocalStore/QuoterSettingsTable/QuoterSettingsIndexMap.hpp"

namespace NTPro::Ecn::NewUiServer
{

enum class DummyColumnType : TradingSerialization::Table::TColumnType
{
    Value = 0,

    Id = 1,
};

inline std::ostream& operator<<(std::ostream& out, DummyColumnType value)
{
    switch (value)
    {
    case DummyColumnType::Id:
        return out << "Id";
    case DummyColumnType::Value:
        return out << "Value";
    default:
        assert(false);
        return out << "???";
    }
}

struct DummyTableSetup
{
    using TTableColumnType = DummyColumnType;

    static constexpr DummyColumnType IdColumn = DummyColumnType::Id;

    static constexpr DummyColumnType ColumnsRange[] =
    {
        DummyColumnType::Value,
        DummyColumnType::Id,
    };

    static constexpr TradingSerialization::Table::ColumnsInfo FieldList[] =
    {
        { DummyColumnType::Value, TradingSerialization::Table::FilterValueType::String },
        { DummyColumnType::Id, TradingSerialization::Table::FilterValueType::Int }
    };
};

struct DummyTableItem : public Basis::Traceable
{
    using TIntFilter = TradingSerialization::Table::TInt;
    using TFloatFilter = TradingSerialization::Table::TFloat;
    using TCellVariant = TradingSerialization::Table::TCellVariant;
    using TTableRowValues = TradingSerialization::Table::ValueSet;
    using TColumnType = DummyColumnType;
    using TFilters = TradingSerialization::Table::Filters;
    using TId = int64_t;

    TId Data = 0;
    std::string Value;

    DummyTableItem() = default;

    DummyTableItem(
        int64_t aData,
        const std::string& aValue = "Value")
        : Data(aData)
        , Value(aValue)
    {}

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            Data,
            Value);
    }

    /// For traceable.
    void ToString(std::ostream& stream) const override
    {
        stream << "<" << Data << ", " << Value << ">";

    }

    bool operator ==(const DummyTableItem& aOther) const
    {
        return std::tie(Data, Value)
            == std::tie(aOther.Data, aOther.Value);
    }

    TCellVariant GetValue(TColumnType aColumn) const
    {
        switch (static_cast<DummyColumnType>(aColumn))
        {
        case DummyColumnType::Id:
            return TradingSerialization::Table::TInt { Data };
        case DummyColumnType::Value:
            return TradingSerialization::Table::TString { Value };
        default:
            assert(false);
            break;
        }
        return TradingSerialization::Table::TInt {};
    }

    TTableRowValues GetValues() const
    {
        return TableUtils<DummyTableSetup>::GetItemValues(*this);
    }

    TId GetId() const
    {
        return Data;
    }
};

struct DummyMultiIndexContainerSetup
{
    /// Data
    using TData = DummyTableItem;
    using TId = DummyTableItem::TId;

    /// Tags
    struct ById
    {
        typedef TId result_type;
        result_type operator()(const VersionedItem<TData>& aData) const
        {
            return aData.Item->Data;
        }
    };

    struct ByValue
    {
        typedef std::string result_type;
        const result_type& operator()(const VersionedItem<TData>& aData) const
        {
            return aData.Item->Value;
        }
    };
};

class DummyTableItemMultiIndex : public BaseMultiIndexContainer<
    DummyTableItemMultiIndex,
    DummyMultiIndexContainerSetup,
    DummyMultiIndexContainerSetup::ByValue>
{
public:
    using ByValueAndIdAndVersion = BySomethingAndId<typename DummyMultiIndexContainerSetup::ByValue>;

    DummyTableItemMultiIndex(Basis::Tracer& aTracer)
        : BaseMultiIndexContainer(aTracer)
    {}
};

struct DummyTableItemIndexRangesInit
{
    const DummyTableItemMultiIndex& Map;
    const TradingSerialization::Table::FilterGroup& FilterExpression;
    std::optional<Model::ActionType> IncrementAction;
    TDataVersion Version;

    DummyTableItemIndexRangesInit(
        const DummyTableItemMultiIndex& aMap,
        const TradingSerialization::Table::FilterGroup& aFilterExpression,
        std::optional<Model::ActionType> aIncrementAction,
        TDataVersion aVersion)
        : Map(aMap)
        , FilterExpression(aFilterExpression)
        , IncrementAction(aIncrementAction)
        , Version(aVersion)
    {}
};

struct SubscribeDummyData : public Basis::Traceable
{
    static constexpr const char* ID = "SubscribeDummyData";

    int Value = 1;

    void ToString(std::ostream& /*stream*/) const override
    {}
};

struct DummyPackUpdate
{
    Basis::Vector<DummyTableItem> Update;
};

struct Data : public Basis::Traceable
{
    using TId = int;
    TId Id {};

    Data() = default;
    Data(TId aId)
        : Id(aId)
    {}

    void ToString(std::ostream&) const override {}

    bool operator ==(const Data& aOther) const
    {
        return Id == aOther.Id;
    }
};

inline bool operator==(const Data& l, const Data& r)
{
    return l.Id == r.Id;
}

}
