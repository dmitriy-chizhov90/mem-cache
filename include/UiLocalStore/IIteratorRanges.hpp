#pragma once

#include <NewUiServer/UiLocalStore/IFairOperation.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Диапазон итераторов.
 * \ingroup NewUiServer
 */
template <typename TIterator>
using TIteratorRange = std::pair<TIterator, TIterator>;

/**
 * \brief Диапазоны итераторов.
 * \ingroup NewUiServer
 */
template <typename _TIterator>
struct IIteratorRanges
{
    template<typename TImpl, typename TOwnership = Basis::Own>
    struct Ranges : public Basis::Interface<TImpl, TOwnership>
    {
        using Basis::Interface<TImpl, TOwnership>::Interface;
        using TIterator = _TIterator;

        API_METHOD(Reset)

        API_METHOD(Add, SINGLEARG(const TIteratorRange<TIterator>&) /* aRange */)

        API_METHOD_RETURN(SINGLEARG(std::optional<TIterator>), Next)
    };
};

}

