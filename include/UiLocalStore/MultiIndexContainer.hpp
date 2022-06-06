#pragma once

#include "UiLocalStore/VersionedDataContainer.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace NTPro::Ecn::NewUiServer
{

template <typename TDerived, typename TContainerSetup, typename... TIndices>
class BaseMultiIndexContainer
{
public:
    using TData = typename TContainerSetup::TData;
    using TId = typename TData::TId;
    using TDataItem = VersionedItem<TData>;

    using ById = typename TContainerSetup::ById;

public:
    struct ByVersion
    {
        typedef TDataVersion result_type;
        result_type operator()(const VersionedItem<TData>& aData) const
        {
            return aData.Version;
        }
    };

    struct ByIsRewrited
    {
        typedef std::optional<TDataVersion> result_type;
        const result_type& operator()(const VersionedItem<TData>& aData) const
        {
            return aData.IsRewritedBy;
        }
    };

    struct ByIdAndVersion
    {
    };

    struct ByVersionAndId
    {
    };

    struct ByIsRewritedAndId
    {
    };

    template <typename TIndex>
    struct BySomethingAndId
    {
    };

    template <typename TIndex>
    using TCustomIndex = boost::multi_index::ordered_non_unique<
        boost::multi_index::tag<BySomethingAndId<TIndex>>,
        boost::multi_index::composite_key<
            TDataItem,
            TIndex,
            ById,
            ByVersion>>;

    using Type = Basis::MultiIndex
    <
        TDataItem,
        boost::multi_index::indexed_by
        <
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<ByIdAndVersion>,
            boost::multi_index::composite_key<
                TDataItem,
                ById,
                ByVersion>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<ByVersionAndId>,
            boost::multi_index::composite_key<
                TDataItem,
                ByVersion,
                ById>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<ByIsRewritedAndId>,
            boost::multi_index::composite_key<
                TDataItem,
                ByIsRewrited,
                ById>>,
        TCustomIndex<TIndices>...
        >
    >;

    using TIdIndex = typename Type::template index<ByIdAndVersion>::type;
    using TIdIterator = typename TIdIndex::iterator;
    using TIdConstIterator = typename TIdIndex::const_iterator;

    using TVersionIndex = typename Type::template index<ByVersionAndId>::type;
    using TVersionIterator = typename TVersionIndex::iterator;
    using TVersionConstIterator = typename TVersionIndex::const_iterator;

    using TRewritedIndex = typename Type::template index<ByIsRewritedAndId>::type;
    using TRewritedIterator = typename TRewritedIndex::iterator;
    using TRewritedConstIterator = typename TRewritedIndex::const_iterator;

    BaseMultiIndexContainer(Basis::Tracer& aTracer)
        : mTracer(aTracer)
    {}

    void Emplace(const Basis::SPtr<TData>& aData, TDataVersion aVersion)
    {
        auto& index = mContainer.template get<ByIdAndVersion>();
        auto [iter, res] = index.emplace(aData, aVersion);
        assert(res);
        if (res)
        {
            if (iter == index.cbegin())
            {
                return;
            }
            --iter;
            ById getId;
            if (getId(*iter) == getId(VersionedItem<TData>(aData, aVersion)))
            {
                mContainer.modify(iter, [aVersion](TDataItem& aItem) { aItem.IsRewritedBy = aVersion; });
            }
        }
    }

    void Erase(typename TData::TId aId, TDataVersion aVersion)
    {
        auto& index = mContainer.template get<ByIdAndVersion>();
        auto [it1, it2] = index.equal_range(std::make_tuple(aId));
        if (it1 == it2)
        {
            mTracer.InfoSlow("Erase failed:", aId);
            return;
        }
        --it2;
        index.modify(it2, [aVersion](TDataItem& aItem) { aItem.IsRewritedBy = aVersion; });
    }

    void ErasePrevious(TDataVersion aVersion)
    {
        mTracer.InfoSlow("Erase previous:", aVersion);

        using TIsRewrited = std::optional<TDataVersion>;
        auto& index = mContainer.template get<ByIsRewritedAndId>();
        /// Отсекаем все элементы, которые не были перезаписаны, то есть у которых IsRewritedBy == std::nullopt
        auto itBegin = index.lower_bound(std::make_tuple(TIsRewrited {0}));
        /// Выбираем все элементы, которые были перезаписаны версиями до текущей aVersion
        auto itEnd = index.lower_bound(std::make_tuple(TIsRewrited {aVersion}));
        index.erase(itBegin, itEnd);
    }

    size_t Size() const
    {
        return mContainer.size();
    }

    void Clear()
    {
        mContainer.clear();
        mStartTime = Basis::DateTime {};
    }

    Basis::DateTime GetMinTime() const
    {
        return mStartTime;
    }

    void ProcessInitialPack() {}

protected:
    Type mContainer;

    Basis::Tracer& mTracer;

    Basis::DateTime mStartTime;
    
    template <typename TSetup, typename TRangeDerived>
    class BaseIndexRanges
    {
    public:
        using TData = typename TSetup::TData;
        using TMap = TDerived;
        using TDataId = typename TMap::TId;

        using TIdRanges = typename TSetup::TIdRanges;
        using TAddedRanges = typename TSetup::TAddedRanges;
        using TDeletedRanges = typename TSetup::TDeletedRanges;

        using IIdRanges = typename IIteratorRanges<typename TMap::TIdConstIterator>::template Ranges<TIdRanges>;
        using IAddedRanges = typename IIteratorRanges<typename TMap::TVersionConstIterator>::template Ranges<TAddedRanges>;
        using IDeletedRanges = typename IIteratorRanges<typename TMap::TRewritedConstIterator>::template Ranges<TDeletedRanges>;

        using TUiFilter = TradingSerialization::Table::Filter;
        using TUiFilters = TradingSerialization::Table::FilterGroup;

        enum class BaseRangeType
        {
            Id = 1000,
            Added,
            Deleted,
            Custom,
            Nothing
        };

        struct TInit
        {
            const TMap& Map;
            const TUiFilters& FilterExpression;
            std::optional<Model::ActionType> IncrementAction;
            TDataVersion Version;

            TInit(
                const TMap& aMap,
                const TUiFilters& aFilterExpression,
                std::optional<Model::ActionType> aIncrementAction,
                TDataVersion aVersion)
                : Map(aMap)
                , FilterExpression(aFilterExpression)
                , IncrementAction(aIncrementAction)
                , Version(aVersion)
            {}
        };

        enum class InitResult
        {
            Inited,
            NotInited,
            Error
        };

        IIdRanges IdRanges;
        IAddedRanges AddedRanges;
        IDeletedRanges DeletedRanges;

    private:
        TRangeDerived* mDerived;

    protected:
        const TMap* mMap = nullptr;
        const TUiFilters* mFilterExpression = nullptr;

        BaseRangeType mRangeType = BaseRangeType::Nothing;

        TDataVersion mVersion{0};

        std::optional<TDataId> mPreviousId;

        Basis::Tracer& mTracer;

    public:

        BaseIndexRanges(
            Basis::Tracer& aTracer,
            TRangeDerived* aDerived)
            : mDerived(aDerived)
            , mTracer(aTracer)
        {
        }

        bool IsInitialized() const
        {
            return mRangeType != BaseRangeType::Nothing;
        }

        bool Init(const TInit& aInit)
        {
            assert(!IsInitialized());

            mMap = &aInit.Map;
            mFilterExpression = &aInit.FilterExpression;
            mVersion = aInit.Version;

            if (aInit.IncrementAction)
            {
                if (aInit.IncrementAction == Model::ActionType::New)
                {
                    InitAddedRange();
                    return true;
                }

                if (aInit.IncrementAction == Model::ActionType::Delete)
                {
                    InitDeletedRange();
                    return true;
                }

                assert(false);
                return false;
            }

            assert(mFilterExpression->Relation == TradingSerialization::Table::FilterRelation::And);

            auto res = mDerived->InitCustom();
            if (res.has_value())
            {
                mRangeType = BaseRangeType::Custom;
                return *res;
            }
            return InitIdRange();
        }

        void Reset()
        {
            IdRanges.Reset();
            AddedRanges.Reset();
            DeletedRanges.Reset();
            mDerived->ResetCustom();

            mMap = nullptr;
            mFilterExpression = nullptr;

            mRangeType = BaseRangeType::Nothing;
            mPreviousId = std::nullopt;
        }

        Basis::SPtr<TData> GetNext()
        {
            switch(mRangeType)
            {
            case BaseRangeType::Id:
                return GetNext(IdRanges);
            case BaseRangeType::Added:
                return GetVersionedNext(AddedRanges);
            case BaseRangeType::Deleted:
                return GetVersionedNext(DeletedRanges);
            case BaseRangeType::Custom:
                return mDerived->CustomGetNext();
            default:
                mTracer.Error("BaseIndexRanges.GetNext uninitialized");
                assert(false);
                break;
            }
            return nullptr;
        }

    protected:
        void InitAddedRange()
        {
            const auto& index = mMap->mContainer.template get<typename TMap::ByVersionAndId>();
            AddedRanges.Add(index.equal_range(std::make_tuple(mVersion)));
            mRangeType = BaseRangeType::Added;
            mTracer.InfoSlow("MultiIndexContainer.Init added by version", mVersion);
        }

        void InitDeletedRange()
        {
            using TIsRewrited = std::optional<TDataVersion>;
            const auto& index = mMap->mContainer.template get<typename TMap::ByIsRewritedAndId>();
            DeletedRanges.Add(index.equal_range(std::make_tuple(TIsRewrited {mVersion})));
            mRangeType = BaseRangeType::Deleted;
            mTracer.InfoSlow("MultiIndexContainer.Init deleted by version:", mVersion);
        }

        bool InitIdRange()
        {
            const auto& index = mMap->mContainer.template get<typename TMap::ByIdAndVersion>();
            auto range = std::make_pair(index.cbegin(), index.cend());
            IdRanges.Add(range);
            mRangeType = BaseRangeType::Id;
            mTracer.InfoSlow("MultiIndexContainer.Init by id: ", index.size());
            return true;
        }

        template <typename TRanges>
        Basis::SPtr<TData> GetNext(TRanges& outRanges)
        {
            using TIterator = decltype(outRanges.Next());
            TIterator it;
            while (true)
            {
                it = outRanges.Next();
                if (!it || (*it)->Item->GetId() != mPreviousId)
                {
                    break;
                }
            }

            if (!it)
            {
                return nullptr;
            }

            while (true)
            {
                mPreviousId = (*it)->Item->GetId();
                const auto& isRewrited = (*it)->IsRewritedBy;
                if (!isRewrited || *isRewrited > mVersion)
                {
                    break;
                }
                it = outRanges.Next();
                if (!it)
                {
                    return nullptr;
                }
            }

            return (*it)->Item;
        }

        template <typename TRanges>
        Basis::SPtr<TData> GetVersionedNext(TRanges& outRanges)
        {
            using TIterator = decltype(outRanges.Next());
            if (TIterator it = outRanges.Next())
            {
                return (*it)->Item;
            }

            return nullptr;
        }

        template <typename TIndex, typename TRanges>
        bool InitFilterIteratorRangeByKey(
            const TradingSerialization::Table::TInt& aFieldFilter,
            TRanges& outRanges)
        {
            if (!aFieldFilter)
            {
                mTracer.Error("InitFilterIteratorRangeByKey: key value is not initialized");
                return false;
            }
            
            const auto& index = mMap->mContainer.template get<TIndex>();
            outRanges.Add(index.equal_range(*aFieldFilter));
            return true;
        }

        template <typename TIndex, typename TRanges>
        bool InitFilterIteratorRangeByBounds(
            const TradingSerialization::Table::TInt& aLowBound,
            const TradingSerialization::Table::TInt& aUpperBound,
            TRanges& outRanges)
        {
            static constexpr char funcName[] = "InitFilterIteratorRangeByBounds: ";
                        
            if (!aLowBound && !aUpperBound)
            {
                mTracer.ErrorSlow(std::string { funcName } + "no bounds specified");
                return false;
            }

            const auto& index = mMap->mContainer.template get<TIndex>();
            auto begin = index.cbegin();
            auto end = index.cend();

            if (aLowBound)
            {
                mTracer.InfoSlow("Lower bound: ", *aLowBound);
                begin = index.lower_bound(*aLowBound);
            }
            
            if (aUpperBound)
            {
                mTracer.InfoSlow("Upper bound: ", *aUpperBound);
                end = index.upper_bound(*aUpperBound);
            }

            outRanges.Add(std::make_pair(begin, end));
            return true;
        }
    };

};

}
