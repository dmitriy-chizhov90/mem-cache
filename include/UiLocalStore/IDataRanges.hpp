#pragma once

#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>

#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Индексированные диапазоны.
 * \ingroup NewUiServer
 */
template <typename TData>
struct IDataRanges
{
    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Ranges : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD_TEMPLATE_TYPE_RETURN(TInit, bool, Init, const TInit& /* aInit */)

        API_METHOD(Reset)

        CONST_API_METHOD_RETURN(bool, IsInitialized)

        API_METHOD_RETURN(SINGLEARG(Basis::SPtr<TData>), GetNext)
    };
};

}
