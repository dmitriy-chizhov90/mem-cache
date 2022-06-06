#pragma once

#include <cstdint>

#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>

#include <Common/Traceable.hpp>
#include <Common/Real.hpp>

#include <cereal/types/boost_variant.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/chrono.hpp>

namespace NTPro::Ecn::TradingSerialization::Table
{

/**
 * \brief Номер колонки.
 * \ingroup UiLocalStore``
 * Используется для нумерации колонок в фильтрах, сортировках и обновлениях.
 * Может быть номером реальной колонки, а также номером дополнительного фильтра.
 */
using TColumnType = int64_t;


static constexpr TColumnType StartFilterValue = 1000;

/**
 * \brief Номера дополнительных фильтров.
 * \ingroup UiLocalStore
 */
enum class Filters : TColumnType
{
    FullTextSearch = StartFilterValue
};

/**
 * \brief Оператор фильтрации.
 * \ingroup UiLocalStore
 */
enum class FilterOperator : int64_t
{
    In,
    GreaterEq,
    LessEq,
    Greater,
    Less

//    Like
};

inline std::ostream& operator<<(std::ostream& out, FilterOperator value)
{
    switch (value)
    {
    case FilterOperator::In:
        return out << "In";
    case FilterOperator::GreaterEq:
        return out << "GreaterEq";
    case FilterOperator::LessEq:
        return out << "LessEq";
    case FilterOperator::Greater:
        return out << "Greater";
    case FilterOperator::Less:
        return out << "Less";
    default :
        return out << "???";
    }
}

/**
 * \brief Типы данных
 * \ingroup UiLocalStore
 * Типы данных полей таблицы или фильтров.
 */
using TString = std::optional<std::string>;
using TInt = std::optional<int64_t>;
using TFloat = std::optional<Basis::Real>;

/**
 * \brief Вектора данных
 * \ingroup UiLocalStore
 * Вектор данных может использоваться:
 * - для задания набора фильтров по колонке;
 * - для передачи значений нескольких колонок.
 */
using TStringValues = std::vector<TString>;
using TIntValues = std::vector<TInt>;
using TFloatValues = std::vector<TFloat>;

using TFilterMplVector = boost::mpl::vector<TString, TInt, TFloat>;

/**
 * \brief Значение ячейки таблицы.
 * \ingroup UiLocalStore
 */
using TCellVariant = boost::variant<TString, TInt, TFloat>;

/**
 * \brief Аргумент оператора фильтрации.
 * \ingroup UiLocalStore
 */
using TFilterVariant = TCellVariant;

/**
 * \brief Строка таблицы.
 * \ingroup UiLocalStore
 * Представляет собой список полей таблицы. Каждое значение соответствуют колонке в табличном enum.
 */
using TTableRowValues = std::vector<TCellVariant>;

/**
 * \brief Список аргументов для оператора фильтрации.
 * \ingroup UiLocalStore
 */
using TFilterValues = TTableRowValues;

/**
 * \brief Набор значений разных типов.
 * \ingroup UiLocalStore
 */
struct ValueSet : public Basis::Traceable
{
    TStringValues StringValues;
    TIntValues IntValues;
    TFloatValues FloatValues;

    ValueSet() = default;

    bool IsEmpty() const
    {
        return StringValues.empty() && IntValues.empty() && FloatValues.empty();
    }

    size_t Size() const
    {
        return StringValues.size() + IntValues.size() + FloatValues.size();
    }

    TTableRowValues GetAllValues() const
    {
        TTableRowValues result;
        for (const auto& v : StringValues)
        {
            result.emplace_back(v);
        }
        for (const auto& v : IntValues)
        {
            result.emplace_back(v);
        }
        for (const auto& v : FloatValues)
        {
            result.emplace_back(v);
        }
        return result;
    }
    template <typename T>
    void Add(T&& aValue)
    {
        if constexpr (std::is_same<typename std::decay<T>::type, TString>::value)
        {
            StringValues.push_back(aValue);
        }
        else if constexpr (std::is_same<typename std::decay<T>::type, TFloat>::value)
        {
            FloatValues.push_back(aValue);
        }
        else
        {
            IntValues.push_back(aValue);
        }
    }

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("stringValues", StringValues),
            cereal::make_nvp("intValues", IntValues),
            cereal::make_nvp("floatValues", FloatValues));
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "ValueSet:{ ";
        FIELD_TO_STREAM(stream, StringValues);
        FIELD_TO_STREAM(stream, IntValues);
        FIELD_TO_STREAM(stream, FloatValues);
        stream << "}";
    }

    friend bool operator ==(const ValueSet& aLhs, const ValueSet& aRhs)
    {
        return std::tie(aLhs.StringValues, aLhs.IntValues, aLhs.FloatValues)
            == std::tie(aRhs.StringValues, aRhs.IntValues, aRhs.FloatValues);
    }
};

/**
 * \brief Тип аргумента для оператора фильтрации.
 * \ingroup UiLocalStore
 */
enum class FilterValueType
{
    String = 0,
    Int,
    Float,

    Unknown
};

inline std::ostream& operator<<(std::ostream& out, FilterValueType value)
{
    switch (value)
    {
    case FilterValueType::String:
        return out << "String";
    case FilterValueType::Int:
        return out << "Int";
    case FilterValueType::Float:
        return out << "Float";
    case FilterValueType::Unknown:
        return out << "Unknown";
    default :
        return out << "???";
    }
}

/**
 * \brief Фильтр.
 * \ingroup UiLocalStore
 * TODO: добавить ограничение на количество значений
 */
struct Filter : public Basis::Traceable
{
    /**
     * \brief Номер колонки.
     * Номер колонки, по которой осуществляется фильтр.
     * Может быть номером реальной колонки, а также номером дополнительного фильтра.
     */
    TColumnType Column{0};
    /**
     * \brief Аргументы фильтрации.
     * Должно быть хотя бы одно значение.
     * Для In допускается больше одного значения.
     * Тип значений должен быть одинаковым.
     */
    ValueSet Values;
    /**
     * \brief Оператор фильтрации.
     */
    FilterOperator Operator{FilterOperator::In};
    /**
     * \brief Инвертировать оператор фильтрации.
     * TODO: Не реализовано
     */
    bool IsInverse = false;

    Filter() = default;
    Filter(
        TColumnType aColumn,
        const ValueSet& aValues,
        FilterOperator aOperator,
        bool aIsInverse)
        : Column(aColumn)
        , Values(aValues)
        , Operator(aOperator)
        , IsInverse(aIsInverse)
    {
    }

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("column", Column),
            cereal::make_nvp("values", Values),
            cereal::make_nvp("operator", Operator),
            cereal::make_nvp("isInverse", IsInverse));
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "Filter:{ ";
        FIELD_TO_STREAM(stream, Column);
        FIELD_TO_STREAM(stream, Values);
        FIELD_TO_STREAM(stream, Operator);
        FIELD_TO_STREAM(stream, IsInverse);
        stream << "}";
    }

    friend bool operator ==(const Filter& aLhs, const Filter& aRhs)
    {
        return std::tie(aLhs.Column, aLhs.Values, aLhs.Operator, aLhs.IsInverse)
            == std::tie(aRhs.Column, aRhs.Values, aRhs.Operator, aRhs.IsInverse);
    }
};

/**
 * \brief Отношение фильтров, входящих в одну группу.
 * \ingroup UiLocalStore
 */
enum class FilterRelation : int64_t
{
    And = 0
};

inline std::ostream& operator<<(std::ostream& out, FilterRelation value)
{
    switch (value)
    {
    case FilterRelation::And:
        return out << "And";
    default :
        return out << "???";
    }
}

struct FilterColumnComparer
{
    using is_transparent = std::true_type;

    bool operator()(const Filter& aLhs, const Filter& aRhs) const
    {
        return aLhs.Column < aRhs.Column;
    }

    bool operator()(const Filter& aLhs, TColumnType aRhs) const
    {
        return aLhs.Column < aRhs;
    }

    bool operator()(TColumnType aLhs, const Filter& aRhs) const
    {
        return aLhs < aRhs.Column;
    }
};

/**
 * \brief Группа фильтров, связанная отношением.
 * \ingroup UiLocalStore
 */
struct FilterGroup : public Basis::Traceable
{
    using TFilters = std::multiset<Filter, FilterColumnComparer>;

    static constexpr size_t MaxCount = 100;

    /**
     * \brief Список фильтров.
     * Для упрощения колонка уникальна.
     */
    TFilters Filters;

    /**
     * \brief Отношение фильтров, входящих в группу.
     */
    FilterRelation Relation{FilterRelation::And};

    FilterGroup() = default;
    FilterGroup(
        const TFilters& aFilters,
        FilterRelation aRelation)
        : Filters(aFilters)
        , Relation(aRelation)
    {
    }

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("filters", Filters),
            cereal::make_nvp("relation", Relation));
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "Filter:{ ";
        FIELD_TO_STREAM(stream, Filters);
        FIELD_TO_STREAM(stream, Relation);
        stream << "}";
    }

    friend bool operator ==(const FilterGroup& aLhs, const FilterGroup& aRhs)
    {
        return std::tie(aLhs.Filters, aLhs.Relation)
            == std::tie(aRhs.Filters, aRhs.Relation);
    }
};

/**
 * \brief Порядок сортировки.
 * \ingroup UiLocalStore
 * Представляет собой упорядоченный список колонок.
 * Чем ближе к началу списка, тем выше приоритет сортировки.
 * TODO: добавить ограничение на количество значений и на неповторяющиеся значения
 */
using TSortOrder = std::vector<TColumnType>;

/**
 * \brief Информация о колонках таблицы.
 * \ingroup UiLocalStore
 */
struct ColumnsInfo
{
    /**
     * \brief Номер колонки.
     * Может быть номером реальной колонки, а также номером дополнительного фильтра.
     */
    TColumnType Column;
    /**
     * \brief Тип.
     * Колонка содержит значения данного типа.
     */
    FilterValueType Type;

    ColumnsInfo() = default;

    template <typename TColumnTypeEnum>
    constexpr ColumnsInfo(TColumnTypeEnum aColumn, FilterValueType aType)
        : Column(static_cast<TColumnType>(aColumn))
        , Type(aType)
    {}
};

/**
 * \brief Информация о дополнительных фильтрах.
 * \ingroup UiLocalStore
 */
const ColumnsInfo FilterFieldList[] =
{
    { Filters::FullTextSearch, FilterValueType::String }
};

}
