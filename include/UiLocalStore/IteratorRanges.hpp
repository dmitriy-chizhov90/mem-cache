#pragma once

#include <NewUiServer/UiLocalStore/IIteratorRanges.hpp>


#include <Common/Tracer.hpp>

#include <Common/Pack.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Наборы данных.
 * \ingroup NewUiServer
 * Класс Позволяет обработать данные из нескольких наборов.
 * Каждый диапазон данных задается в виде пары итераторов.
 */
template <typename TIterator>
class IteratorRanges
{
private:
    Basis::Vector<TIteratorRange<TIterator>> mRanges;
    std::optional<TIterator> mCurrentIt;
    size_t mCurrentIndex = 0;

public:
    void Reset()
    {
        *this = IteratorRanges {};
    }

    void Add(const TIteratorRange<TIterator>& aRange)
    {
        if (aRange.first == aRange.second)
        {
            return;
        }
        mRanges.push_back(aRange);
    }

    std::optional<TIterator> Next()
    {
        if (mRanges.empty())
        {
            return ResetCurrentIt();
        }
        if (mCurrentIndex >= mRanges.size())
        {
            return ResetCurrentIt();
        }
        if (!mCurrentIt)
        {
            assert(mCurrentIndex == 0);
            mCurrentIt = mRanges[mCurrentIndex].first;
            /// Не должно быть пустых диапазонов
            assert(*mCurrentIt != mRanges[mCurrentIndex].second);
            return mCurrentIt;
        }
        const auto& range = mRanges[mCurrentIndex];
        assert((*mCurrentIt) != range.second);
        if (++(*mCurrentIt) == range.second)
        {
            if (++mCurrentIndex == mRanges.size())
            {
                return ResetCurrentIt();
            }

            mCurrentIt = mRanges[mCurrentIndex].first;
        }
        return mCurrentIt;
    }

private:
    auto ResetCurrentIt()
    {
        Reset();
        return mCurrentIt;
    }
};

}
