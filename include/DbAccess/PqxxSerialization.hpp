#pragma once

#include "Basis/DbAccess/PqxxReader.hpp"

#include "Common/TypeTraits.hpp"

#include <Wt/Dbo/Field.h>

#include <boost/algorithm/string.hpp>

namespace NTPro::Ecn::DbAccess
{

// Получает список полей в БД для структуры.
// Вызывается в методе persist структуры Mapping<T>.
struct DbFieldsGetter
{
    Basis::Vector<std::string> Fields;
    Basis::UnorderedMap<std::string, size_t> Indexes;

    template <typename TValue>
    void act(Wt::Dbo::FieldRef<TValue> aField)
    {
        Indexes.emplace(aField.name(), Fields.size());
        Fields.push_back(aField.name());
    }
};

// Заполняет поле структуры Mappint<T> значением из Row.
// Вызывается в методе persist структуры Mapping<T>.
struct DbFieldsConverter
{
    const DbAccess::PqxxReader::TRow& Row;
    const DbFieldsGetter& Fields;
    std::string Error;
    
    DbFieldsConverter(
        const DbAccess::PqxxReader::TRow& aRow,
        const DbFieldsGetter& aFields);

    template <typename TField>
    void act(Wt::Dbo::FieldRef<TField> aField)
    {
        auto index = Fields.Indexes.at(aField.name());
        const auto& value = Row.at(index);

        if (!value)
        {
            return;
        }
        
        if constexpr (Common::is_optional<TField>::value)
        {
            ActInternal<typename TField::value_type>(aField, *value);
        }
        else
        {
            ActInternal<TField>(aField, *value);
        }
    }

private:

    /// Для опциональных типов TField и TValue могут различаться
    template <typename TValue, typename TField>
    void ActInternal(Wt::Dbo::FieldRef<TField> aField, const std::string& aDbValue)
    {
        static_assert(
            Common::is_string<TValue>::value
            || std::is_integral<TValue>::value
            || std::is_enum<TValue>::value
            || Common::is_sequential_id<TValue>::value
            || Common::is_date_time<TValue>::value,
            "DbFieldsConverter type not implemented");

        try
        {
            if constexpr (Common::is_fixed_size_string<TValue>::value)
            {
                TValue v;
                v.AssignDownsized(aDbValue.c_str());
                aField.setValue(std::move(v));
            }
            else if constexpr (Common::is_string<TValue>::value)
            {
                aField.setValue(TValue { aDbValue.c_str() });
            }
            else if constexpr (std::is_same<TValue, bool>::value)
            {
                if (aDbValue == "t")
                {
                    aField.setValue(true);
                }
                else if (aDbValue == "f")
                {
                    aField.setValue(false);
                }
            }
            else if constexpr (
                std::is_integral<TValue>::value
                || std::is_enum<TValue>::value)
            {
                aField.setValue(static_cast<TValue>(std::stoll(aDbValue)));
            }
            else if constexpr (Common::is_sequential_id<TValue>::value)
            {
                aField.setValue(TValue { static_cast<typename TValue::TId>(std::stoll(aDbValue))});
            }
            else if constexpr (std::is_same<TValue, Basis::DateTime>::value)
            {
                aField.setValue(ParseDateTime(aDbValue));
            }
        }
        catch(std::exception& e)
        {
            Error = aField.name() + ": " + e.what();
        }
    }

    static Basis::DateTime ParseDateTime(const std::string& aDbValue);
};

}
