#pragma once

#include "Columns.hpp"
#include "RowRange.hpp"

#include "Common/MsgProcessor/MessageBase.hpp"
#include <Common/MsgProcessor/SysPayload/Generics/CommandReply.hpp>
#include "TradingSerialization/Types.hpp"

namespace NTPro::Ecn::TradingSerialization::Table
{

/**
 * \brief Видимый диапазон строк для таблицы.
 * \ingroup UiLocalStore
 */
struct Update : public MessageBase
{
    static constexpr const char* ID = "TableUpdate";

    /**
     * \brief Диапазон строк
     * Top соответствует первой строке в Data, Bottom - последней
     */
    RowRange Rows;

    /**
     * \brief Набор видимых строк.
     * Каждая строка содержит набор полей в порядке, определенном в табличном FieldList.
     */
    std::vector<ValueSet> Values;


    Update() = default;

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("rows", Rows),
            cereal::make_nvp("data", Values));
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "TableUpdate:{ ";
        FIELD_TO_STREAM(stream, Rows);
        FIELD_TO_STREAM(stream, Values);
        stream << "}";
    }

    MessageTraceTarget GetTraceTarget() const override
    {
        return MessageTraceTarget::HighFrequency;
    }
};

}
