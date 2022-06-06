#pragma once

#include "RowRange.hpp"

#include "Common/MsgProcessor/MessageBase.hpp"
#include <Common/MsgProcessor/SysPayload/Generics/CommandReply.hpp>
#include "TradingSerialization/Types.hpp"

namespace NTPro::Ecn::TradingSerialization::Table
{

struct Update;

struct RowWindowRequest : public MessageBase
{
    static constexpr const char* ID = "RowWindowRequest";
    static constexpr int8_t Roles = Admin + Manager;

    // TResponse must be defined for subscriptions
    using TResponse = Update;
    // TAckReply must be defined for all messages processed by the server
    using TAckReply = Common::MsgProcessor::CommandReply;

    Common::MsgProcessor::CmdId SubscriptionId{};
    /// Диапазон строк
    RowRange Rows;

    RowWindowRequest() = default;

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("subscriptionId", SubscriptionId),
            cereal::make_nvp("rows", Rows));
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "RowWindowRequest:{ ";
        FIELD_TO_STREAM(stream, SubscriptionId);
        FIELD_TO_STREAM(stream, Rows);
        stream << "}";
    }

    MessageTraceTarget GetTraceTarget() const override
    {
        return MessageTraceTarget::HighFrequency;
    }
};

}
