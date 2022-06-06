#pragma once

#include "Common/Traceable.hpp"

namespace NTPro::Ecn::TradingSerialization::Table
{

struct RowRange : public Ecn::Basis::Traceable
{
    static constexpr int64_t MaxRowWindow = 100;

    int64_t Top = -1;
    int64_t Bottom = -1;

    template <class Archive>
    void serialize(Archive& anArchive)
    {
        anArchive(
            cereal::make_nvp("top", Top),
            cereal::make_nvp("bottom", Bottom));
    }

    virtual void ToString(std::ostream& stream) const override
    {
        stream << "RowRange:{ ";
        FIELD_TO_STREAM(stream, Top);
        FIELD_TO_STREAM(stream, Bottom);
        stream << "}";
    }
};

}
