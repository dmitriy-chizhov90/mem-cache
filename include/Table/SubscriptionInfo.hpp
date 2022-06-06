#pragma once

#include "Columns.hpp"
#include "RowRange.hpp"

#include "Common/MsgProcessor/MessageBase.hpp"
#include <Common/MsgProcessor/SysPayload/Generics/CommandReply.hpp>
#include "TradingSerialization/Types.hpp"

namespace NTPro::Ecn::TradingSerialization::Table
{

/**
 * \brief Подтверждение успешной подписки.
 * \ingroup UiLocalStore
 * Отправляется сервером один раз в ответ на успешную подписку.
 */
struct SubscriptionInfo : public MessageBase
{
    static constexpr const char* ID = "TableSubscriptionInfo";

    /**
     * \brief
     * Top соответствует первой строке в Data, Bottom - последней
     */
    size_t RowCount = 0;

    SubscriptionInfo(size_t aRowCount = 0)
        : RowCount(aRowCount)
    {
    }

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("rowCount", RowCount));
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "TableSubscriptionInfo:{ ";
        FIELD_TO_STREAM(stream, RowCount);
        stream << "}";
    }

    bool operator==(const SubscriptionInfo& aOther) const
    {
        return RowCount == aOther.RowCount;
    }
};

}
