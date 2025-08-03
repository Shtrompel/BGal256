#ifndef _SORTER_ALGORITHM
#define _SORTER_ALGORITHM

#include <string>
#include <vector>
#include <random>
#include <queue>

#include "plugin.hpp"

typedef int sorterarray_event_t;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_NONE = 0;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_CMP = 1;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_SWAP = 2;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_MOVE = 3;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_SET = 4;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_READ = 5;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_REMOVE = 6;
constexpr sorterarray_event_t SORTER_ARRAY_EVENT_END = 7;
constexpr size_t SORTER_ARRAY_EVENTS_COUNT = 8;

typedef int elementevent_t;
constexpr sorterarray_event_t ELEMENT_EVENT_NONE = 0;
constexpr sorterarray_event_t ELEMENT_EVENT_READ = 1;
constexpr sorterarray_event_t ELEMENT_EVENT_WRITE = 2;
constexpr sorterarray_event_t ELEMENT_EVENT_REMOVE = 3;

constexpr int MAX_ARRAY_SIZE = 1000;

// #define ENABLE_TEST_SORT

enum class t_algorithmtype
{
    BUBBLE,
    INSERTION,
    BINARY_INSERTION,
    SELECTION,
    DOUBLE_SELECTION,
    COCKTAIL_SHAKER,
    MERGE,
    COMB,
    GNOME,
    OPTIMIZED_GNOME,
    ODD_EVEN,
    SHELL,
    HEAP,
    SMOOTH,
    BITONIC,
    QUICK,
    BINARY_QUICK,
    RADIX_LSD,
    RADIX_MSD,
    RADIX_LSD_2,
    RADIX_MSD_2,
    RADIX_LSD_16,
    RADIX_MSD_16,
    CYCLE,
    BOGO,
    EXCHANGE_BOGO,
    STALIN,

#ifdef ENABLE_TEST_SORT
    TEST,
#endif
    COUNT
};

struct SelectedType
{
    elementevent_t elementEvent = 0;
    int index;

    SelectedType()
    {
    }

    SelectedType(elementevent_t elementEvent, int index);

    elementevent_t getEvent() const
    {
        return elementEvent;
    }

    int getIndex() const
    {
        return index;
    }
};

struct SorterArrayEvent
{
    sorterarray_event_t eventType = 0;
    std::vector<SelectedType> elements;
    int valueA = 0;
    int valueB = 0;

    ~SorterArrayEvent();

    SorterArrayEvent();

    SorterArrayEvent(
        sorterarray_event_t eventType);

    void addElementEvent(elementevent_t elementEvent, int index);

    const std::vector<SelectedType> &getElements() const
    {
        return this->elements;
    }
};

struct SorterAlgorithm
{
    std::vector<int> array;
    std::vector<SorterArrayEvent> events;

    std::string name;
    t_algorithmtype type;
    bool precalculate;
    volatile std::atomic<bool> *externalStopFlag;

    virtual void reset()
    {
        array.clear();
        events.clear();
    }

    virtual void calculate(std::vector<int> arrayParam)
    {
    }

    virtual SorterArrayEvent step(std::vector<int> &arrayParam)
    {
        return SorterArrayEvent{};
    }

    virtual ~SorterAlgorithm() = default;

    SorterAlgorithm(
        const std::string &name,
        t_algorithmtype,

        bool precalculate = true);

    bool compare(int i, int j);

    void swap(int i, int j);

    void move(int i, int j);

    void set(int i, int value);

    int read(int i);

    void end();

    bool doPrecompute()
    {
        return precalculate;
    }

    std::vector<SorterArrayEvent> &&getEvents()
    {
        return std::move(events);
    }
};

// Bubble Sort
struct BubbleSort : SorterAlgorithm
{
    BubbleSort();
    void calculate(std::vector<int> array) override;
};

// Insertion Sort
struct InsertionSort : SorterAlgorithm
{
    InsertionSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Binary Insertion Sort
struct BinaryInsertionSort : SorterAlgorithm
{
    BinaryInsertionSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Selection Sort
struct SelectionSort : SorterAlgorithm
{
    SelectionSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Double Selection Sort
struct DoubleSelectionSort : SorterAlgorithm
{
    DoubleSelectionSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Cocktail Shaker Sort
struct CocktailShakerSort : SorterAlgorithm
{
    CocktailShakerSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Merge Sort
struct MergeSort : SorterAlgorithm
{
    MergeSort();
    void reset() override;
    void calculate(std::vector<int> arrayParam) override;
    SorterArrayEvent step(std::vector<int> &arrayParam) override;

private:
    void mergeSort(int left, int right, std::vector<int> &buffer);
};

// Comb Sort
struct CombSort : SorterAlgorithm
{
    CombSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Gnome Sort
struct GnomeSort : SorterAlgorithm
{
    GnomeSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Optimized Gnome Sort
struct OptimizedGnomeSort : SorterAlgorithm
{
    OptimizedGnomeSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Odd Even Sort
struct OddEvenSort : SorterAlgorithm
{
    OddEvenSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Shell Sort
struct ShellSort : SorterAlgorithm
{
    ShellSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Heap Sort
struct HeapSort : SorterAlgorithm
{
    HeapSort();
    void heapify(int n, int i);
    void calculate(std::vector<int> arrayParam) override;
};

// Smooth Sort
struct SmoothSort : SorterAlgorithm
{
    SmoothSort();
    void reset() override;
    // std::vector<int> leonardoNumbers(int max);
    std::vector<int> L;

    int leonardo(int k);
    void heapify(int start, int end);
    void calculate(std::vector<int> arrayParam) override;
};

// Bitonic Sort
struct BitonicSort : SorterAlgorithm
{
    BitonicSort();

    int greatestPowerOfTwoLessThan(int n);
    void bitonicMerge(int lo, int n, bool dir);
    void bitonicSort(int lo, int n, bool dir);

    void calculate(std::vector<int> arrayParam) override;
};

// Quick Sort
struct QuickSort : SorterAlgorithm
{
    QuickSort();
    void reset() override;
    void calculate(std::vector<int> arrayParam) override;
    SorterArrayEvent step(std::vector<int> &arrayParam) override;

private:
    void quickSort(int lo, int hi);
    int partition(int lo, int hi);
};

// Quick Sort
struct BinaryQuickSort : SorterAlgorithm
{
    struct Task {
        int p, r;
        int bit;
        Task(int p, int r, int bit) : p(p), r(r), bit(bit) {}
    };

    BinaryQuickSort();
    void calculate(std::vector<int> arrayParam) override;

    int partition(int p, int r, int bit);
    int getBit(int index, int bit);
};

// Radix Sort LSD
struct RadixSortLSD : SorterAlgorithm
{
    int base;
    RadixSortLSD(
        int base = 10, 
        t_algorithmtype type = t_algorithmtype::RADIX_LSD,
        std::string title = "Radix Sort LSD (base 10)");
    void calculate(std::vector<int> arrayParam) override;
};

// Radix Sort MSD
struct RadixSortMSD : SorterAlgorithm
{
    int base;
    RadixSortMSD(
        int base = 10, 
        t_algorithmtype type = t_algorithmtype::RADIX_MSD,
        std::string title = "Radix Sort MSD (base 10)");
    void calculate(std::vector<int> arrayParam) override;
    void radixSortMSD(int lo, int hi, int depth, int pmax, int RADIX);
};

// Cycle Sort
struct CycleSort : SorterAlgorithm
{
    CycleSort();
    void calculate(std::vector<int> arrayParam) override;
};

// Bogo Sort
struct BogoSort : SorterAlgorithm
{
    BogoSort();
    void reset() override;
    void calculate(std::vector<int> arrayParam) override;
    SorterArrayEvent step(std::vector<int> &arrayParam) override;

private:
    bool isSorted(std::vector<int> &arrayParam);

    std::random_device dev;
    std::mt19937 rng;

    int index = 0;
};

// Exchange Bogo Sort
struct ExchangeBogoSort : SorterAlgorithm
{
    ExchangeBogoSort();
    void reset() override;
    void calculate(std::vector<int> arrayParam) override;
    SorterArrayEvent step(std::vector<int> &arrayParam) override;

private:
    bool isSorted(std::vector<int> &arrayParam);

    std::random_device dev;
    std::mt19937 rng;

    int index = 0;
};

// Stalin Sort
struct StalinSort : SorterAlgorithm
{
    StalinSort();
    void reset() override;
    void calculate(std::vector<int> arrayParam) override;
    SorterArrayEvent step(std::vector<int> &arrayParam) override;

private:
    int index = 0;
    int sortedEnd = 0;
};

// Test Sort
#ifdef ENABLE_TEST_SORT
struct TestSort : SorterAlgorithm
{
    TestSort()
        : SorterAlgorithm("Test Sort", t_algorithmtype::TEST)
    {
    }

    void calculate(std::vector<int> arrayParam) override
    {
        while (1)
        {
            if (*externalStopFlag)
                return;
        }
        end();
    }
};
#endif

#endif // _SORTER_ALGORITHM