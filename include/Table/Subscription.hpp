#pragma once

#include "Common/MsgProcessor/MessageBase.hpp"
#include <Common/MsgProcessor/SysPayload/Generics/CommandReply.hpp>
#include "Common/UniqueId.hpp"
#include "TradingSerialization/Types.hpp"
#include "Columns.hpp"
#include "TradingSerialization/UserRole.hpp"

#include <cereal/types/memory.hpp>

namespace NTPro::Ecn::TradingSerialization::Table
{

struct SubscriptionInfo;

/**
 * \brief Подписка на таблицу UiLocalStore.
 * \ingroup UiLocalStore
 */
struct SubscribeBase : public MessageBase
{
    // TResponse must be defined for subscriptions
    using TResponse = SubscriptionInfo;
    // TAckReply must be defined for all messages processed by the server
    using TAckReply = Common::MsgProcessor::CommandReply;
    using TId = Basis::UniqueId<SubscribeBase>;

    /**
     * \brief Порядок сортировки.
     */
    TSortOrder SortOrder;

    /**
     * \brief Набор фильтров по колонкам.
     */
    FilterGroup FilterExpression;

    SubscribeBase() = default;

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("sortOrder", SortOrder),
            cereal::make_nvp("filterExpression", FilterExpression));
    }

    static void PrintSortOrder(std::ostream& stream, const TSortOrder& aSortOrder)
    {
        stream << "SortOrder { ";
        stream << Basis::ContainerToString(aSortOrder);
        stream << "}; ";
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "TableSubscribe:{ ";
        PrintSortOrder(stream, SortOrder);
        FIELD_TO_STREAM(stream, FilterExpression);
        stream << "}";
    }

    friend bool operator ==(const SubscribeBase& aLhs, const SubscribeBase& aRhs)
    {
        return std::tie(aLhs.SortOrder, aLhs.FilterExpression)
            == std::tie(aRhs.SortOrder, aRhs.FilterExpression);
    }
};

struct QuoterSettingsSubscribe : public SubscribeBase
{
    static constexpr const char* ID = "QuoterSettingsTableSubscribe";
    static constexpr int8_t Roles = Admin + Manager;
};

}
