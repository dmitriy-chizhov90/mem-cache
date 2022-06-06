#pragma once

#include <NewUiServer/UiLocalStore/IIteratorRanges.hpp>


#include <Common/Tracer.hpp>

#include <Common/Pack.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Итераторы по паку.
 * \ingroup NewUiServer
 * Класс позволяет обработать элементы пака или вектора.
 * \implements IDataRanges
 */
template <typename TSetup>
class PackRanges
{
public:
    using TData = typename TSetup::TData;
    using TDataPack = typename TSetup::TDataPack;
    using TPackIterator = typename TSetup::TPackIterator;
    using TPackRanges = typename TSetup::TPackRanges;

    using IPackRanges = typename IIteratorRanges<TPackIterator>::template Ranges<TPackRanges>;

    IPackRanges Ranges;

private:
    const TDataPack* mPack = nullptr;

    Basis::Tracer& mTracer;

public:

    PackRanges(Basis::Tracer& aTracer)
        : mTracer(aTracer)
    {
    }

    bool IsInitialized() const
    {
        return mPack;
    }

    bool Init(const TDataPack& aPack)
    {
        assert(!IsInitialized());

        mPack = &aPack;
        auto range = std::make_pair(mPack->cbegin(), mPack->cend());
        Ranges.Add(range);

        return true;
    }

    void Reset()
    {
        Ranges.Reset();
        mPack = nullptr;
    }

    Basis::SPtr<TData> GetNext()
    {
        auto it = Ranges.Next();
        if (it)
        {
            return **it;
        }
        return nullptr;
    }
};

}
