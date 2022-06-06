#pragma once

#include <Common/Interface.hpp>
#include <Common/InterfaceGenerator.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Справедливая операция.
 * \ingroup NewUiServer
 * Операция выполняется с помощью последовательного вызова метода Process.
 * Каждый вызов метода Process и создание экземпляра класса должны выполняться за констрантное время.
 */
template <typename TStates>
struct IFairOperation
{
    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Performer : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;

        API_METHOD_TEMPLATE_TYPE(TInit, Init, const TInit& /* aInit */)
        API_METHOD_RETURN(TStates, Process)

        CONST_API_METHOD_RETURN(bool, IsInitialized)
        CONST_API_METHOD_RETURN(TStates, GetState)

        API_METHOD(Reset)

    };
};

}
