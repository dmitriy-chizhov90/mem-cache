#pragma once

#include <NewUiServer/UiLocalStore/ITableSorter.hpp>
#include <NewUiServer/UiLocalStore/SortOrderComparator.hpp>
#include <NewUiServer/UiLocalStore/TableUtils.hpp>

#include "TradingSerialization/Table/Columns.hpp"

#include <Common/Tracer.hpp>

#include <Common/Pack.hpp>

namespace NTPro::Ecn::NewUiServer
{

/**
 * \brief Табличный сортировальщик.
 * \ingroup NewUiServer
 */
template <typename TSetup>
class TableSorter
{
public:
    static constexpr int MaxCount = TSetup::MaxCount;

    using TData = typename TSetup::TData;
    using TTableSetup = typename TSetup::TTableSetup;

    using TDataPack = Basis::Pack<TData>;
    using TIt = typename TDataPack::iterator;
    using State = TableSorterState;

    using TTableColumnType = typename TTableSetup::TTableColumnType;

    using TSortOrder = TradingSerialization::Table::TSortOrder;
    using TComparator = SortOrderComparator<TData, TTableColumnType>;

    struct TInit
    {
        const TSortOrder& SortOrder;
        TDataPack& Result;
        TDataPack& TmpBuffer;

        TInit(
            const TSortOrder& aSortOrder,
            TDataPack& outResult,
            TDataPack& outTmpBuffer)
            : SortOrder(aSortOrder)
            , Result(outResult)
            , TmpBuffer(outTmpBuffer)
        {}
    };

private:
    /**
     * \brief Этап выполнения операции.
     */
    State mState = State::Initializing;
    /**
     * \brief Флаг ошибки.
     */
    bool mOk = true;

    /// Входные параметы
    /**
     * \brief Порядок сортировки.
     */
    const TSortOrder* mSortOrder = nullptr;
    /**
     * \brief Массив данных для сортировки.
     * Туда же сохраняется результат выполнения операции.
     */
    TDataPack* mResult;
    /**
     * \brief Временный буфер.
     * Используется для сортировки слиянием.
     */
    TDataPack* mTmpBuffer;

    /**
     * \brief Итератор частей массива.
     * На первом этапе происходит сортировка массива по частям.
     * Итератор указывает на ту часть массива, которая будет обрабатываться на текущей итерации.
     * Используется только на этапе TableSorterState::PartSort.
     */
    TIt mCurrentPartIt;

    /**
     * \brief Cостояние сортировки слиянием.
     * Используется только на этапе TableSorterState::MergeSort.
     */
    struct MergeSortInfo
    {
        /**
         * \brief Итераторы на части массива.
         * Указывают текущие части, которые сливаются друг с другом.
         */
        struct PartIterators
        {
            /**
             * \brief Итераторы слияния.
             * Указывают текущие элементы слияния.
             */
            struct MergeIterators
            {
                /**
                 * \brief Итератор элемента левой части.
                 */
                TIt ILeft;
                /**
                 * \brief Итератор элемента правой части.
                 */
                TIt IRight;
                /**
                 * \brief Итератор целевого элемента слияния.
                 */
                TIt ITarget;
            };

            /**
             * \brief Итератор начала первой (левой) части.
             */
            TIt SortIt1;
            /**
             * \brief Итератор на целевой буфер слияния.
             */
            TIt Target;

            /**
             * \brief Итератор начала второй (правой) части.
             * Начало второй части совпадает с концом первой.
             * Первая (левая) часть - [SortIt1, SortIt2)
             */
            TIt SortIt2;
            /**
             * \brief Итератор конца второй (правой) части.
             * Вторая (правая) часть - [SortIt2, SortIt3)
             */
            TIt SortIt3;

            /**
             * \brief Итераторы слияния.
             * \see MergeIterators.
             */
            std::optional<MergeIterators> ItemIt;
        };

        /**
         * \brief Количество элементов в одной части массива.
         * На втором этапе происходит последовательное слияние отсортированных частей массива.
         * После того, как все части с текущим MergeCount слиты, MergeCount увеличивается вдвое.
         */
        int MergeCount = MaxCount;
        /**
         * \brief Итераторы на части массива.
         * \see PartIterators.
         */
        std::optional<PartIterators> PartIt;
    };

    using TPartIterators = typename MergeSortInfo::PartIterators;
    using TMergeIterators = typename TPartIterators::MergeIterators;

    /**
     * \brief Cостояние сортировки слиянием.
     * \see MergeSortInfo.
     */
    MergeSortInfo mMergeInfo;
    /**
     * \brief Функтор сравнения.
     * \see SortOrderComparator.
     */
    std::unique_ptr<TComparator> mComparator;
    /**
     * \brief Tracer.
     */
    Basis::Tracer& mTracer;

public:
    TableSorter(
        Basis::Tracer& aTracer)
        : mTracer(aTracer)
    {
    }

    bool IsInitialized() const
    {
        return mState != State::Initializing;
    }

    void Init(const TInit& aInit)
    {
        assert(!IsInitialized());

        mSortOrder = &aInit.SortOrder;
        mResult = &aInit.Result;
        mTmpBuffer = &aInit.TmpBuffer;

        mCurrentPartIt = mResult->begin();
        mComparator = std::make_unique<TComparator>(*mSortOrder, mOk);

        /// Валидация входных параметров (порядка сортировки)
        auto sortOrderError = TableUtils<TTableSetup>::CheckSortOrder(*mSortOrder);
        if (!sortOrderError.empty())
        {
            mState = State::Error;
            mTracer.ErrorSlow("TableSorter:", sortOrderError, ", ", *mSortOrder);
            return;
        }

        /// Временный буфер должен иметь размер исходного буфера.
        /// TODO: аллокация может занимать много времени.
        mTmpBuffer->resize(mResult->size());
        /// Если объект создан переходим в следующее состояние.
        mState = State::PartSort;
    }

    void Reset()
    {
        mSortOrder = nullptr;
        mResult = nullptr;
        mTmpBuffer = nullptr;

        mComparator.reset();

        mState = State::Initializing;
        mOk = true;

        mMergeInfo = MergeSortInfo {};
    }

    /**
     * \brief Выполнение операции.
     * Два этапа сортировки: сортировка по частям и сортировка слиянием.
     * Каждый вызов Process выполняется за константное время.
     */
    State Process()
    {
        switch (mState)
        {
        case State::PartSort:
            ProcessPartSort();
            break;
        case State::MergeSort:
            ProcessMergeSort();
            break;
        default:
            /// Process можно вызывать только в состояниях обработки операции.
            mTracer.ErrorSlow("Sorter.Process: state is invalid:", mState);
            mState = State::Error;
            break;
        }
        return mState;
    }

    State GetState() const
    {
        return mState;
    }

private:
    /**
     * \brief Простая сортировка.
     * Сортировка текущей части с помощью стандартной функции сортировки из std.
     * Возвращает true, если в процессе работы не возникло ошибок.
     */
    bool PlainSort(TIt aSecondIt)
    {
        std::sort(mCurrentPartIt, aSecondIt, *mComparator);
        /// Компаратор может выставить флаг ошибки в False
        if (!mOk)
        {
            mState = State::Error;
            mTracer.Error("Sorter: PlainSort failed");
            return false;
        }
        return true;
    }

    /**
     * \brief Выполнение частичной сортировки.
     * Выполняется очередная итерация частичной сортировки.
     */
    void ProcessPartSort()
    {
        /// Текущее состояние должно быть PartSort.
        if (mState != State::PartSort)
        {
            mTracer.ErrorSlow("Sorter.ProcessPartSort: state is invalid:", mState);
            mState = State::Error;
            return;
        }

        /// Проверяем расстояние до конца массива.
        const auto distance = mResult->end() - mCurrentPartIt;
        if (distance > MaxCount)
        {
            /// Сортируем очередную часть. Перемещаем итератор текущей части. Отдаем управление.
            auto nextIt = mCurrentPartIt + MaxCount;
            PlainSort(nextIt);
            mCurrentPartIt = nextIt;
        }
        else
        {
            /// Сортируем последнюю часть.
            if (PlainSort(mResult->end()))
            {
                /// Если ошибок не было переходим ко второму этапу.
                mState = State::MergeSort;
                TrySetCompleted();
            }
        }
    }

    /**
     * \brief Выполнение сортировки слиянием.
     * Заводятся три цикла.
     * Внешний цикл на каждой итерации увеличивает размер части в два раза.
     * Средний цикл проходит по частям текущего размера и сливает две соседние части между собой.
     * Внутренний цикл пробегает по элементам двух соседних частей и производит слияние.
     */
    void ProcessMergeSort()
    {
        /// Текущее состояние должно быть MergeSort.
        if (mState != State::MergeSort)
        {
            mTracer.ErrorSlow("Sorter.ProcessMergeSort: state is invalid:", mState);
            mState = State::Error;
            return;
        }

        /// Внешний цикл по MergeCount.
        if (TrySetCompleted())
        {
            /// Должны были выйти после PartSort или в конце итерации MergeSort.
            assert(false);
            return;
        }

        /// Средний цикл по частям размера MergeCount.
        auto& mergeParts = mMergeInfo.PartIt;
        if (!mergeParts)
        {
            /// Начало среднего цикла
            mergeParts = TPartIterators {};
            mergeParts->Target = mTmpBuffer->begin();
            mergeParts->SortIt1 = mResult->begin();
            FindSecondPart();
        }
        if (mResult->end() - mergeParts->SortIt1 > mMergeInfo.MergeCount)
        {
            /// До конца массива умещается одна часть и ещё хотя бы один элемент из второй части.
            /// Слияние двух частей.
            if (!PlainMerge())
            {
                /// Cлияние не завершено.
                if (!mOk)
                {
                    /// Произошла ошибка.
                    mState = State::Error;
                    mTracer.Error("Sorter: ProcessMergeSort failed");
                }
                return;
            }

            if (!mergeParts->ItemIt)
            {
                /// Завершение итерации среднего цикла. Перемещаемся к следующим двум частям
                const auto partsDistance = mergeParts->SortIt3 - mergeParts->SortIt1;
                mergeParts->Target = mergeParts->Target += partsDistance;
                mergeParts->SortIt1 = mergeParts->SortIt3;
                FindSecondPart();
            }
        }
        else
        {
            /// До конца массива умещается только одна часть.
            /// Добавляем её во временный буфер.
            if (MoveTail())
            {
                /// Завершение последней итерации среднего цикла.
                /// Итератор частей должен уже смотреть на конец массива данных.
                assert(mergeParts->SortIt1 == mResult->end());
            }
        }

        if (mergeParts->SortIt1 == mResult->end())
        {
            /// Средний цикл завершен.
            /// Завершение итерации внешнего цикла.
            mergeParts = std::nullopt;
            std::swap(*mTmpBuffer, *mResult);
            mMergeInfo.MergeCount <<= 1;
            /// Проверяем завершение всей сортировки.
            TrySetCompleted();
        }
    }

    /**
     * \brief Проверка завершения сортировки.
     * Вызывается в состоянии MergeSort.
     */
    bool TrySetCompleted()
    {
        assert(mState == State::MergeSort);
        const auto elementsCount = static_cast<int>(mResult->size());
        if (mMergeInfo.MergeCount >= elementsCount)
        {
            /// Осталась одна часть. Она должна быть отсортированной.
            /// Выходим из внешнего цикла и завершаем сортировку.
            mState = State::Completed;
            return true;
        }
        return false;
    }

    /**
     * \brief Сортировка слиянием.
     * Сливает две части массива. Каждая часть отсортирована.
     * Один вызов PlainMerge обрабатывает константное число элементов и возвращает управление.
     * Возвращает true, если слили все элементы из двух частей, false - в противном случае.
     */
    bool PlainMerge()
    {
        auto& mergeParts = *mMergeInfo.PartIt;
        auto& itemIt = mergeParts.ItemIt;

        /// Внутренний цикл по элементам двух частей.
        if (!itemIt)
        {
            /// Начало внутреннего цикла.
            InitMergeIterators();
        }

        int counter = 0;
        /// Ограничиваем количество обрабатываемых элементов константным числом.
        while (++counter <= MaxCount)
        {
            if (itemIt->ILeft == mergeParts.SortIt2)
            {
                /// Дошли до конца левой части.
                if (itemIt->IRight == mergeParts.SortIt3)
                {
                    /// Дошли до конца правой части.
                    /// Выходим из внутреннего цикла.
                    itemIt = std::nullopt;
                    return true;
                }
                /// Есть элемент справа, нет элемента слева. Берем правый элемент.
                MergerRightElement();
                continue;
            }
            /// Слева есть элементы, иначе вышли бы по условию выше.
            if (itemIt->IRight == mergeParts.SortIt3)
            {
                /// Есть элемент слева, нет элемента справа. Берем левый элемент.
                MergerLeftElement();
                continue;
            }

            /// Есть элементы и слева, и справа, иначе вышли бы по условиям выше.
            if ((*mComparator)(*itemIt->ILeft, *itemIt->IRight))
            {
                /// Левый элемент меньше правого. Берем левый элемент.
                MergerLeftElement();
            }
            else
            {
                /// Левый элемент не меньше правого. Берем правый элемент.
                MergerRightElement();
            }
        }
        if (itemIt->ILeft == mergeParts.SortIt2
            && itemIt->IRight == mergeParts.SortIt3)
        {
            /// Дошли до концов левой и правой частей.
            itemIt = std::nullopt;
            return true;
        }
        return false;
    }

    /**
     * \brief Переместить остаток.
     * Переместить оставшуюся часть массива во временный буфер слитых частей.
     * Возвращает true, если весь остаток перемещен.
     */
    bool MoveTail()
    {
        auto& mergeParts = mMergeInfo.PartIt;
        assert(mergeParts);

        int counter = 0;
        while ((mergeParts->SortIt1 != mResult->end()) && (++counter <= MaxCount))
        {
            MoveTailElement();
        }

        return (mergeParts->SortIt1 == mResult->end());
    }

    /**
     * \brief Переместить один элемент из остатка.
     */
    void MoveTailElement()
    {
        auto& mergeParts = mMergeInfo.PartIt;
        assert(mergeParts);

        std::swap(*mergeParts->Target, *mergeParts->SortIt1);
        ++mergeParts->SortIt1;
        ++mergeParts->Target;
    }

    /**
     * \brief Инициализировать итераторы слияния.
     * Вызывается в начале нового внутреннего цикла.
     */
    void InitMergeIterators()
    {
        assert(mMergeInfo.PartIt);
        auto& mergeParts = *mMergeInfo.PartIt;
        auto& itemIt = mergeParts.ItemIt;
        assert(!itemIt);

        itemIt = TMergeIterators {};
        itemIt->ILeft = mergeParts.SortIt1;
        itemIt->IRight = mergeParts.SortIt2;
        itemIt->ITarget = mergeParts.Target;
    }

    /**
     * \brief Переместить правый элемент.
     * Перемещает элемент из правой части в целевой буфер.
     */
    void MergerRightElement()
    {
        assert(mMergeInfo.PartIt);
        auto& mergeParts = *mMergeInfo.PartIt;
        auto& itemIt = mergeParts.ItemIt;
        assert(itemIt);

        std::swap(*itemIt->ITarget, *itemIt->IRight);
        ++itemIt->ITarget;
        ++itemIt->IRight;
    }

    /**
     * \brief Переместить левый элемент.
     * Перемещает элемент из левой части в целевой буфер.
     */
    void MergerLeftElement()
    {
        assert(mMergeInfo.PartIt);
        auto& mergeParts = *mMergeInfo.PartIt;
        auto& itemIt = mergeParts.ItemIt;
        assert(itemIt);

        std::swap(*itemIt->ITarget, *itemIt->ILeft);
        ++itemIt->ITarget;
        ++itemIt->ILeft;
    }

    /**
     * \brief Найти вторую часть.
     * Находим конец левой части (он же начало правой части). После этого находим конец правой части.
     * Нужно вызывать из среднего цикла.
     */
    void FindSecondPart()
    {
        auto& mergeParts = mMergeInfo.PartIt;
        assert(mergeParts);

        {
        const auto part1MaxDistance = mResult->end() - mergeParts->SortIt1;
        mergeParts->SortIt2 = (part1MaxDistance < mMergeInfo.MergeCount)
            ? mResult->end()
            : mergeParts->SortIt1 + mMergeInfo.MergeCount;
        }
        {
        const auto part2MaxDistance = mResult->end() - mergeParts->SortIt2;
        mergeParts->SortIt3 = (part2MaxDistance < mMergeInfo.MergeCount)
            ? mResult->end()
            : mergeParts->SortIt2 + mMergeInfo.MergeCount;
        }

        assert(mergeParts->SortIt1 <= mergeParts->SortIt2);
        assert(mergeParts->SortIt2 <= mergeParts->SortIt3);
    }
};

}
