#pragma once

#include <Common/Collections.hpp>
#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>
#include <Common/SPtr.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Фабрика кэшируемых элементов.
 * \ingroup NewUiServer
 * Абстрагирует работу с Data Item.
 */
template <typename TData>
struct IDataItemBuilder
{

    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Logic : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        CONST_API_METHOD_TEMPLATE_TYPE_RETURN(TModelData, Basis::SPtr<TData>, CreateItem, const TModelData& /* aData */)
    };
};

}
