#include "SorterArray.hpp"

#include "plugin.hpp"

static void debugPrint(const std::vector<int> &) __attribute__((unused));

static void debugPrint(const std::vector<int> &array)
{
    std::string s = "{";
    for (auto &x : array)
    {
        s += std::to_string(x) + ", ";
    }
    s += "}";
}

#define AAAA() DEBUG("a");

SelectedType::SelectedType(
    elementevent_t elementEvent, 
    int index)
    : elementEvent(elementEvent), index(index)
{
}

SorterArrayEvent::~SorterArrayEvent() {}

SorterArrayEvent::SorterArrayEvent(
    sorterarray_event_t eventType) : eventType(eventType)
{
}

SorterArrayEvent::SorterArrayEvent()
{
}

void SorterArrayEvent::addElementEvent(elementevent_t elementEvent, int index)
{
    SelectedType s{elementEvent, index};
    elements.push_back(s);
}

SorterAlgorithm::SorterAlgorithm(
    const std::string &name,
    t_algorithmtype type,
    bool precalculate)
    : name(name), type(type), precalculate(precalculate)
{
}

bool SorterAlgorithm::compare(int i, int j)
{
    SorterArrayEvent event{SORTER_ARRAY_EVENT_CMP};
    event.addElementEvent(ELEMENT_EVENT_READ, i);
    event.addElementEvent(ELEMENT_EVENT_READ, j);
    event.valueA = i;
    event.valueB = j;
    events.push_back(event);
    return array[i] > array[j];
}

void SorterAlgorithm::swap(int i, int j)
{
    int tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
    SorterArrayEvent event{SORTER_ARRAY_EVENT_SWAP};
    event.addElementEvent(ELEMENT_EVENT_WRITE, i);
    event.addElementEvent(ELEMENT_EVENT_WRITE, j);
    event.valueA = i;
    event.valueB = j;
    events.push_back(event);
}

void SorterAlgorithm::move(int i, int j)
{
    array[i] = array[j];
    SorterArrayEvent event{SORTER_ARRAY_EVENT_MOVE};
    event.addElementEvent(ELEMENT_EVENT_WRITE, i);
    event.addElementEvent(ELEMENT_EVENT_READ, j);
    event.valueA = i;
    event.valueB = j;
    events.push_back(event);
}

void SorterAlgorithm::set(int i, int value)
{
    array[i] = value;
    SorterArrayEvent event{SORTER_ARRAY_EVENT_SET};
    event.addElementEvent(ELEMENT_EVENT_WRITE, i);
    event.valueA = i;
    event.valueB = value;
    events.push_back(event);
}

int SorterAlgorithm::read(int i)
{
    SorterArrayEvent event{SORTER_ARRAY_EVENT_SET};
    event.addElementEvent(ELEMENT_EVENT_READ, i);
    event.valueA = i;
    event.valueB = array[i];
    events.push_back(event);

    return array[i];
}

void SorterAlgorithm::end()
{
    SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
    events.push_back(event);
}

// Bubble Sort

BubbleSort::BubbleSort()
    : SorterAlgorithm("Bubble Sort", t_algorithmtype::BUBBLE)
{
}

void BubbleSort::calculate(
    std::vector<int> arrayParam)
{
    this->array = std::move(arrayParam);
    int n = array.size();
    while (n > 1)
    {
        int newn = 0;
        if (*externalStopFlag)
            return;
        for (int i = 1; i < n; i++)
        {
            // stop on request
            if (*externalStopFlag)
                return;

            if (compare(i - 1, i))
            {
                swap(i - 1, i);
                newn = i;
            }
        }
        n = newn;
    }
    end();
}

// Insertion Sort

InsertionSort::InsertionSort() : SorterAlgorithm("Insertion Sort", t_algorithmtype::INSERTION)
{
}

void InsertionSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = (int)array.size();
    for (int i = 1; i < n; ++i)
    {
        int key = read(i);

        int j = i - 1;
        if (*externalStopFlag)
            return;
        while (j >= 0 && read(j) > key)
        {
            if (*externalStopFlag)
                return;
            move(j + 1, j);
            --j;
        }
        set(j + 1, key);
    }
    end();
}

// Binary Insertion Sort

BinaryInsertionSort::BinaryInsertionSort() : SorterAlgorithm("Binary Insertion", t_algorithmtype::INSERTION)
{
}

void BinaryInsertionSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = (int)array.size();
    for (int i = 1; i < n; ++i)
    {
        int key = read(i);
        int left = 0;
        int j;
        int right = j = i - 1;

        if (*externalStopFlag)
            return;

        // In the sorted range of 0 to (i - 1)
        // Find where to place array[i]
        while (left <= right)
        {
            int mid = (left + right) / 2;
            if (compare(mid, i))
            {
                right = mid - 1;
            }
            else
            {
                left = mid + 1;
            }
        }

        while (j >= left && read(j) > key)
        {
            if (*externalStopFlag)
                return;
            move(j + 1, j);
            --j;
        }
        set(j + 1, key);
    }
    end();
}

// Selection Sort

SelectionSort::SelectionSort() : SorterAlgorithm("Selection Sort", t_algorithmtype::SELECTION)
{
}

void SelectionSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = (int)array.size();
    for (int i = 0; i < n - 1; ++i)
    {
        int minIdx = i;
        if (*externalStopFlag)
            return;
        for (int j = i + 1; j < n; ++j)
        {
            if (*externalStopFlag)
                return;
            if (compare(minIdx, j))
            {
                minIdx = j;
            }
        }
        if (minIdx != i)
        {
            swap(i, minIdx);
        }
    }
    end();
}

// Double Selection Sort

DoubleSelectionSort::DoubleSelectionSort() : SorterAlgorithm("Double Selection Sort", t_algorithmtype::SELECTION)
{
}

void DoubleSelectionSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    int left = 0;
    int right = n - 1;

    while (left < right)
    {
        if (*externalStopFlag)
            return;

        int minIndex = left;
        int maxIndex = left;

        // Find min and max in current range [left, right]
        for (int j = left; j <= right; j++)
        {
            if (*externalStopFlag)
                return;

            // Update minIndex if smaller element found
            if (compare(minIndex, j))
            {
                minIndex = j;
            }

            // Update maxIndex if larger element found
            if (compare(j, maxIndex))
            {
                maxIndex = j;
            }
        }

        // Handle case where min and max are same (all equal elements)
        if (minIndex == maxIndex)
        {
            break;
        }

        // Swap min to left position
        swap(left, minIndex);

        // Adjust maxIndex if it was affected by the previous swap
        if (maxIndex == left)
        {
            maxIndex = minIndex;
        }
        else if (maxIndex == minIndex)
        {
            maxIndex = left;
        }

        // Swap max to right position
        swap(right, maxIndex);

        // Narrow the unsorted range
        left++;
        right--;
    }

    end();
}

// Cocktail Shaker Sort

CocktailShakerSort::CocktailShakerSort() : SorterAlgorithm("Cocktail Shaker Sort", t_algorithmtype::COCKTAIL_SHAKER)
{
}

void CocktailShakerSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    int start = 0;
    int endIndex = n - 1;
    bool swapped = true;

    while (swapped && start < endIndex)
    {
        swapped = false;

        for (int i = start; i < endIndex; i++)
        {
            if (*externalStopFlag)
                return;

            if (compare(i, i + 1))
            {
                swap(i, i + 1);
                swapped = true;
            }
        }

        if (!swapped)
            break;
        endIndex--;

        swapped = false;
        for (int i = endIndex - 1; i >= start; i--)
        {
            if (*externalStopFlag)
                return;

            if (compare(i, i + 1))
            {
                swap(i, i + 1);
                swapped = true;
            }
        }

        start++;
    }

    end();
}
// Merge Sort
MergeSort::MergeSort() : SorterAlgorithm("Merge Sort", t_algorithmtype::MERGE)
{
}

void MergeSort::reset()
{
    SorterAlgorithm::reset();
}

void MergeSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    std::vector<int> buffer(array.size());
    mergeSort(0, (int)array.size(), buffer);
    end();
}

void MergeSort::mergeSort(int left, int right, std::vector<int> &buffer)
{
    if (right - left <= 1)
        return;

    // on stop request, abort
    if (*externalStopFlag)
        return;

    int mid = (left + right) / 2;
    mergeSort(left, mid, buffer);
    mergeSort(mid, right, buffer);

    // merge [left,mid) and [mid,right)
    int i = left, j = mid, k = left;

    while (i < mid && j < right)
    {
        if (*externalStopFlag)
            return;
        if (!compare(i, j))
        {
            buffer[k++] = array[i++];
        }
        else
        {
            buffer[k++] = array[j++];
        }
    }

    while (i < mid)
    {
        // on stop request, abort
        if (*externalStopFlag)
            return;
        buffer[k++] = array[i++];
    }

    while (j < right)
    {
        // on stop request, abort
        if (*externalStopFlag)
            return;
        buffer[k++] = array[j++];
    }

    // copy back
    for (int idx = left; idx < right; ++idx)
    {
        // on stop request, abort
        if (*externalStopFlag)
            return;
        // since we must use only set/move, use set
        set(idx, buffer[idx]);
    }
}

SorterArrayEvent MergeSort::step(std::vector<int> &arrayParam)
{
    // algorithm is deterministic so live steps are uneeded
    return SorterArrayEvent{};
}

// Comb Sort

CombSort::CombSort()
    : SorterAlgorithm("Comb Sort", t_algorithmtype::COMB)
{
}

void CombSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    int gap = n;
    bool swapped = true;
    const float shrink = 1.3f; // Optimal shrink factor

    while (gap > 1 || swapped)
    {
        // Update gap size
        gap = static_cast<int>(gap / shrink);
        if (gap < 1)
            gap = 1;

        swapped = false;

        // Single pass with current gap
        for (int i = 0; i < n - gap; i++)
        {
            if (*externalStopFlag)
                return;

            // Swap if elements are out of order
            if (compare(i, i + gap))
            {
                swap(i, i + gap);
                swapped = true;
            }
        }
    }

    end();
}

// Gnome Sort

GnomeSort::GnomeSort()
    : SorterAlgorithm("Gnome Sort", t_algorithmtype::GNOME)
{
}

void GnomeSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    int index = 0; // Start at the beginning

    while (index < n)
    {
        if (*externalStopFlag)
            return; // Handle external interruption

        if (index == 0)
        {
            // Can't go left of index 0 - always move right
            index++;
        }
        else if (compare(index - 1, index))
        {
            // Previous element is larger - swap and move backward
            swap(index, index - 1);
            index--;
        }
        else
        {
            // Elements are in order - move forward
            index++;
        }
    }

    end();
}

// Optimized Gnome Sort

OptimizedGnomeSort::OptimizedGnomeSort()
    : SorterAlgorithm("Optimized Gnome", t_algorithmtype::OPTIMIZED_GNOME)
{
}

void OptimizedGnomeSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    for (int i = 1; i < n; i++)
    {
        if (*externalStopFlag)
            return;

        int current = read(i);

        int lo = 0;
        int hi = i;
        while (lo < hi)
        {
            if (*externalStopFlag)
                return;

            int mid = lo + (hi - lo) / 2;

            if (!compare(i, mid))
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        }

        int j = i;
        while (j > lo)
        {
            if (*externalStopFlag)
                return;

            set(j, read(j - 1));
            j--;
        }

        set(lo, current);
    }

    end();
}

// Odd Even Sort

OddEvenSort::OddEvenSort()
    : SorterAlgorithm("Odd Even Sort", t_algorithmtype::ODD_EVEN)
{
}

void OddEvenSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    bool sorted = false;

    while (!sorted)
    {
        if (*externalStopFlag)
            return; // Handle external interruption

        sorted = true; // Assume sorted until proven otherwise

        // Odd phase: process elements at odd indices (1, 3, 5...)
        for (int i = 1; i < n - 1; i += 2)
        {
            if (*externalStopFlag)
                return;

            if (compare(i, i + 1))
            {
                swap(i, i + 1);
                sorted = false; // Found disorder - not sorted yet
            }
        }

        // Even phase: process elements at even indices (0, 2, 4...)
        for (int i = 0; i < n - 1; i += 2)
        {
            if (*externalStopFlag)
                return;

            if (compare(i, i + 1))
            {
                swap(i, i + 1);
                sorted = false; // Found disorder - not sorted yet
            }
        }
    }

    end();
}

// Shell Sort

ShellSort::ShellSort()
    : SorterAlgorithm("Shell Sort", t_algorithmtype::SHELL)
{
}

void ShellSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    for (int gap = n / 2; gap > 0; gap /= 2)
    {
        for (int i = gap; i < n; i++)
        {
            int j = i;

            while (j >= gap)
            {
                if (*externalStopFlag)
                    return;

                // Compare using provided function
                if (compare(j - gap, j))
                { // array[j-gap] > array[j]
                    swap(j - gap, j);
                    j -= gap;
                }
                else
                {
                    break;
                }
            }
        }
    }
    end();
}

// Heap Sort

HeapSort::HeapSort()
    : SorterAlgorithm("Heap Sort", t_algorithmtype::HEAP)
{
}

void HeapSort::heapify(int n, int i)
{
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && compare(left, largest))
        largest = left;

    if (right < n && compare(right, largest))
        largest = right;

    if (largest != i)
    {
        swap(i, largest);
        heapify(n, largest);
    }
}

void HeapSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    // Build max-heap
    for (int i = n / 2 - 1; i >= 0; i--)
    {
        if (*externalStopFlag)
            return;
        heapify(n, i);
    }

    // Extract elements from heap
    for (int i = n - 1; i > 0; i--)
    {
        if (*externalStopFlag)
            return;
        swap(0, i);    // Move current root to end
        heapify(i, 0); // Heapify reduced heap
    }
    end();
}

// Smooth Sort

SmoothSort::SmoothSort()
    : SorterAlgorithm("Smooth Sort", t_algorithmtype::SMOOTH)
{
}

void SmoothSort::reset()
{
    SorterAlgorithm::reset();
    L.clear();
}

int SmoothSort::leonardo(int k)
{
    if (k < 2)
    {
        return 1;
    }
    return leonardo(k - 1) + leonardo(k - 2) + 1;
}

void SmoothSort::heapify(int start, int end)
{
    int i = start;
    int j = 0;
    int k = 0;

    while (k < end - start + 1)
    {
        if (k & 0xAAAAAAAA)
        {
            j = j + i;
            i = i >> 1;
        }
        else
        {
            i = i + j;
            j = j >> 1;
        }

        k = k + 1;
    }

    while (i > 0)
    {
        j = j >> 1;
        k = i + j;
        while (k < end)
        {
            if (!compare(k, k - i))
            {
                break;
            }
            swap(k, k - i);
            k = k + i;
        }

        i = j;
    }
}

void SmoothSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();

    int n = array.size();
    int p = n - 1;
    int q = p;
    int r = 0;

    while (p > 0)
    {
        if ((r & 0x03) == 0)
        {
            heapify(r, q);
        }

        if (leonardo(r) == p)
        {
            r = r + 1;
        }
        else
        {
            r = r - 1;
            q = q - leonardo(r);
            heapify(r, q);
            q = r - 1;
            r = r + 1;
        }

        swap(0, p);
        p = p - 1;
    }

    for (int i = 0; i < n - 1; i++)
    {
        int j = i + 1;
        while (j > 0 && !compare(j, j - 1))
        {
            swap(j, j - 1);
            j = j - 1;
        }
    }

    end();
}

// Bitonic Sort

BitonicSort::BitonicSort()
    : SorterAlgorithm("Bitonic Sort", t_algorithmtype::BITONIC)
{
}

int BitonicSort::greatestPowerOfTwoLessThan(int n)
{
    int k = 1;
    while (k < n)
        k = k << 1;
    return k >> 1;
}

void BitonicSort::bitonicMerge(int lo, int n, bool dir)
{
    if (n > 1)
    {
        int m = greatestPowerOfTwoLessThan(n);

        for (int i = lo; i < lo + n - m; i++)
        {
            if (dir == compare(i, i + m))
                swap(i, i + m);
        }

        bitonicMerge(lo, m, dir);
        bitonicMerge(lo + m, n - m, dir);
    }
}

void BitonicSort::bitonicSort(int lo, int n, bool dir)
{
    if (n > 1)
    {
        int m = n / 2;
        bitonicSort(lo, m, !dir);
        bitonicSort(lo + m, n - m, dir);
        bitonicMerge(lo, n, dir);
    }
}

void BitonicSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    bitonicSort(0, (int)array.size(), true);
    end();
}

// Quick Sort

QuickSort::QuickSort()
    : SorterAlgorithm("Quick Sort", t_algorithmtype::QUICK)
{
}

void QuickSort::reset()
{
    SorterAlgorithm::reset();
}

void QuickSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    quickSort(0, (int)array.size() - 1);
    end();
}

void QuickSort::quickSort(int lo, int hi)
{
    if (*externalStopFlag)
        return;
    if (lo >= hi)
        return;
    int p = partition(lo, hi);
    if (*externalStopFlag)
        return;
    quickSort(lo, p - 1);
    quickSort(p + 1, hi);
}

int QuickSort::partition(int lo, int hi)
{
    int i = lo;
    if (*externalStopFlag)
        return -1;
    for (int j = lo; j < hi; ++j)
    {
        if (*externalStopFlag)
            return -1;
        if (!compare(j, hi))
        {
            swap(i, j);
            ++i;
        }
    }
    swap(i, hi);
    return i;
}

SorterArrayEvent QuickSort::step(std::vector<int> &arrayParam)
{
    return SorterArrayEvent{};
}

// Quick Sort

BinaryQuickSort::BinaryQuickSort()
    : SorterAlgorithm("Binary Quick Sort", t_algorithmtype::BINARY_QUICK)
{
}

int BinaryQuickSort::getBit(int index, int bit)
{
    int value = read(index);
    return (value >> bit) & 1;
}

int BinaryQuickSort::partition(int p, int r, int bit)
{
    int i = p - 1;
    int j = r + 1;

    while (true)
    {
        // Find next element with bit set from left
        do
        {
            i++;
            if (*externalStopFlag)
                return -1;
            if (i > r)
                break;
        } while (i <= r && !getBit(i, bit));

        // Find next element with bit unset from right
        do
        {
            j--;
            if (*externalStopFlag)
                return -1;
            if (j < p)
                break;
        } while (j >= p && getBit(j, bit));

        if (i < j)
        {
            swap(i, j);
        }
        else
        {
            return j;
        }
    }
}

void BinaryQuickSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    // Find maximum value to determine bits needed
    int maxVal = array[0];
    for (int i = 1; i < n; i++)
    {
        if (*externalStopFlag)
            return;
        int val = read(i);
        if (val > maxVal)
            maxVal = val;
    }

    // Calculate number of bits required
    int maxBit = 0;
    if (maxVal > 0)
    {
        int temp = maxVal;
        while (temp)
        {
            maxBit++;
            temp >>= 1;
        }
        maxBit--; // Highest bit index
    }
    else
    {
        // All zeros - already sorted
        end();
        return;
    }

    std::queue<Task> tasks;
    tasks.push(Task(0, n - 1, maxBit));

    while (!tasks.empty())
    {
        if (*externalStopFlag)
            return;

        Task t = tasks.front();
        tasks.pop();

        if (t.p < t.r && t.bit >= 0)
        {
            int q = partition(t.p, t.r, t.bit);
            if (q == -1)
                return; // External stop

            tasks.push(Task(t.p, q, t.bit - 1));
            tasks.push(Task(q + 1, t.r, t.bit - 1));
        }
    }

    end();
}

// Radix Sort LSD

RadixSortLSD::RadixSortLSD(
    int base, 
    t_algorithmtype type,
    std::string title)
    : SorterAlgorithm(title, type, true), base(base)
{
}

void RadixSortLSD::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();

    const int radixBase = this->base;

    int maxValue = 0;
    for (int i = 0; i < (int)array.size(); ++i)
        maxValue = std::max(maxValue, read(i));

    int pmax = ceil(
        log(maxValue + 1) /
        log(radixBase));

    for (int p = 0; p < pmax; ++p)
    {
        int base = std::pow(radixBase, p);
        std::vector<int> count(radixBase, 0);
        std::vector<int> copy(array.size());

        for (int i = 0; i < (int)array.size(); ++i)
        {
            copy[i] = read(i);
            int r = copy[i] / base % radixBase;
            count[r]++;
        }

        std::vector<int> bkt(radixBase + 1, 0);
        std::partial_sum(count.begin(), count.end(), bkt.begin() + 1);

        bkt[1] = count[0];
        for (int i = 1; i < (int)count.size(); ++i)
        {
            bkt[i + 1] = bkt[i] + count[i];
        }

        for (int i = 0; i < (int)array.size(); ++i)
        {
            int r = copy[i] / base % radixBase;
            set(bkt[r]++, copy[i]);
        }
    }

    end();
}

// Radix Sort MSD
RadixSortMSD::RadixSortMSD(int base, t_algorithmtype type, std::string title)
    : SorterAlgorithm(title,type, true), base(base)
{
}

void RadixSortMSD::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();
    int n = array.size();
    if (n <= 1)
    {
        end();
        return;
    }

    const int radixBase = this->base; // Base-4 digits

    // Find maximum value to determine digit count
    int maxValue = 0;
    for (int i = 0; i < n; ++i)
    {
        if (*externalStopFlag)
            return;
        // Use read(i) instead of compare(i, i)
        int val = read(i);
        if (val > maxValue)
            maxValue = val;
    }

    // Calculate number of digits (pmax)
    int pmax = 0;
    if (maxValue > 0)
    {
        int temp = maxValue;
        while (temp)
        {
            pmax++;
            temp /= radixBase;
        }
    }
    else
    {
        pmax = 1; // Handle case where all values are 0
    }

    // Start recursive sorting
    radixSortMSD(0, n, 0, pmax, radixBase);
    end();
}

void RadixSortMSD::radixSortMSD(int lo, int hi, int depth, int pmax, int radixBase)
{
    // Base case: subarray of size 0 or 1
    if (hi - lo <= 1)
        return;

    // Base case: exceeded digit depth
    if (depth >= pmax)
        return;

    // Calculate current digit base
    long long base = 1;
    for (int i = 0; i < pmax - depth - 1; i++)
    {
        base *= radixBase;
    }

    // Count digit frequencies
    std::vector<int> count(radixBase, 0);
    for (int i = lo; i < hi; ++i)
    {
        if (*externalStopFlag)
            return;
        int value = read(i);
        int r = (value / base) % radixBase;
        if (r < radixBase)
            count[r]++;
    }

    // Calculate bucket boundaries (inclusive prefix sum)
    std::vector<int> bkt(radixBase, 0);
    if (radixBase > 0)
    {
        bkt[0] = count[0];
        for (int i = 1; i < radixBase; ++i)
        {
            bkt[i] = bkt[i - 1] + count[i];
        }
    }

    // Create a working copy of bucket pointers
    std::vector<int> bkt_work = bkt;

    // In-place permutation using cycle walking
    int nsub = hi - lo;
    int i = 0;
    while (i < nsub)
    {
        if (*externalStopFlag)
            return;

        int r;
        int j;
        while (true)
        {
            // Get digit for current element
            r = (read(lo + i) / base) % radixBase;

            // Get target position from bucket
            j = --bkt_work[r];

            // Stop if element is in correct position
            if (j <= i)
                break;

            // Swap elements
            swap(lo + i, lo + j);
        }
        // Move to next bucket
        i += count[r];
    }

    // Recursively sort buckets
    int start = lo;
    for (int r = 0; r < radixBase; ++r)
    {
        if (count[r] > 1)
        {
            radixSortMSD(start, start + count[r], depth + 1, pmax, radixBase);
        }
        start += count[r];
    }
}

// Cycle Sort
CycleSort::CycleSort()
    : SorterAlgorithm("Cycle Sort", t_algorithmtype::CYCLE)
{
}

void CycleSort::calculate(std::vector<int> arrayParam)
{
    array = std::move(arrayParam);
    events.clear();

    int n = array.size();

    for (int cycleStart = 0; cycleStart < n - 1; ++cycleStart)
    {
        if (*externalStopFlag)
            return;
        int x = read(cycleStart);
        int pos = cycleStart;

        // Find position where we put the item
        for (int i = cycleStart + 1; i < n; ++i)
        {
            if (*externalStopFlag)
                return;
            if (read(i) < x)
                pos++;
        }

        // Skip if already in the correct position
        if (pos == cycleStart)
            continue;

        // Handle duplicates
        while (x == read(pos))
        {
            if (*externalStopFlag)
                return;
            pos++;
        }

        // Put the item into the correct position
        swap(pos, x);

        // Rotate the rest of the cycle
        while (pos != cycleStart)
        {
            if (*externalStopFlag)
                return;
            pos = cycleStart;

            for (int i = cycleStart + 1; i < n; ++i)
            {
                if (*externalStopFlag)
                    return;
                if (read(i) < x)
                    pos++;
            }

            while (x == read(pos))
            {
                if (*externalStopFlag)
                    return;
                pos++;
            }

            x = array[pos];
            swap(pos, x);
        }
    }

    end();

    /*
    """Sort an array in place and return the number of writes."""
    writes = 0

    # Loop through the array to find cycles to rotate.
    # Note that the last item will already be sorted after the first n-1 cycles.
    for cycle_start in range(0, len(array) - 1):
        item = array[cycle_start]

        # Find where to put the item.
        pos = cycle_start
        for i in range(cycle_start + 1, len(array)):
            if array[i] < item:
                pos += 1

        # If the item is already there, this is not a cycle.
        if pos == cycle_start:
            continue

        # Otherwise, put the item there or right after any duplicates.
        while item == array[pos]:
            pos += 1

        array[pos], item = item, array[pos]
        writes += 1

        # Rotate the rest of the cycle.
        while pos != cycle_start:
            # Find where to put the item.
            pos = cycle_start
            for i in range(cycle_start + 1, len(array)):
                if array[i] < item:
                    pos += 1

            # Put the item there or right after any duplicates.
            while item == array[pos]:
                pos += 1
            array[pos], item = item, array[pos]
            writes += 1

    return writes
    */
}

// Bogo Sort

BogoSort::BogoSort()
    : SorterAlgorithm("Bogo Sort", t_algorithmtype::BOGO, false)
{
    this->rng = std::mt19937(dev());
}

void BogoSort::reset()
{
    this->index = 0;
}

void BogoSort::calculate(std::vector<int> arrayParam)
{
}

SorterArrayEvent BogoSort::step(std::vector<int> &arrayParam)
{
    if (arrayParam.empty())
    {
        SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
        return event;
    }

    int n = static_cast<int>(arrayParam.size());

    if (index == 0 && isSorted(arrayParam))
    {
        SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
        return event;
    }

    if (index >= n)
    {
        if (isSorted(arrayParam))
        {
            SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
            return event;
        }
        else
            index = 0;
    }

    // Fisher-Yates shuffle step
    int i = index;
    std::uniform_int_distribution<> dist(i, n - 1); // j ∈ [i, n-1]
    int j = dist(rng);

    std::swap(arrayParam[i], arrayParam[j]);
    index++;

    SorterArrayEvent event{SORTER_ARRAY_EVENT_SWAP};
    event.addElementEvent(ELEMENT_EVENT_WRITE, i);
    event.addElementEvent(ELEMENT_EVENT_WRITE, j);
    event.valueA = i;
    event.valueB = j;
    return event;
}

bool BogoSort::isSorted(std::vector<int> &arrayParam)
{
    if (arrayParam.empty() || arrayParam.size() == 1)
        return true;

    for (size_t i = 0; i < arrayParam.size() - 1; ++i)
    {
        if (arrayParam[i] > arrayParam[i + 1])
            return false;
    }
    return true;
}

// Bogo Sort

ExchangeBogoSort::ExchangeBogoSort()
    : SorterAlgorithm("Exchange Bogo Sort", t_algorithmtype::EXCHANGE_BOGO, false)
{
    this->rng = std::mt19937(dev());
}

void ExchangeBogoSort::reset()
{
}

void ExchangeBogoSort::calculate(std::vector<int> arrayParam)
{
}

SorterArrayEvent ExchangeBogoSort::step(std::vector<int> &arrayParam)
{
    if (arrayParam.empty())
    {
        SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
        return event;
    }

    int n = static_cast<int>(arrayParam.size());

    if (isSorted(arrayParam))
    {
        SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
        return event;
    }

    std::uniform_int_distribution<> dist(0, n - 1); // j ∈ [i, n-1]

    int i, j;
    do
    {
        i = dist(rng);
        j = dist(rng);
        int mn = std::min(i, j);
        int mx = std::max(i, j);
        i = mn, j = mx;
    } while (i == j);

    if (arrayParam[i] > arrayParam[j])
    {
        std::swap(arrayParam[i], arrayParam[j]);

        SorterArrayEvent event{SORTER_ARRAY_EVENT_SWAP};
        event.addElementEvent(ELEMENT_EVENT_WRITE, i);
        event.addElementEvent(ELEMENT_EVENT_WRITE, j);
        event.valueA = i;
        event.valueB = j;
        return event;
    }

    SorterArrayEvent event{SORTER_ARRAY_EVENT_CMP};
    event.addElementEvent(ELEMENT_EVENT_READ, i);
    event.addElementEvent(ELEMENT_EVENT_READ, j);
    event.valueA = i;
    event.valueB = j;
    return event;
}

bool ExchangeBogoSort::isSorted(std::vector<int> &arrayParam)
{
    if (arrayParam.empty() || arrayParam.size() == 1)
        return true;

    for (size_t i = 0; i < arrayParam.size() - 1; ++i)
    {
        if (arrayParam[i] > arrayParam[i + 1])
            return false;
    }
    return true;
}

// Stalin Sort

StalinSort::StalinSort()
    : SorterAlgorithm("Stalin Sort", t_algorithmtype::STALIN, false)
{
}

void StalinSort::reset()
{
    this->index = 0;
    this->sortedEnd = 0;
}

void StalinSort::calculate(std::vector<int> arrayParam)
{
}

SorterArrayEvent StalinSort::step(std::vector<int> &arrayParam)
{
    if (arrayParam.empty())
    {
        SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
        return event;
    }

    int n = static_cast<int>(arrayParam.size());

    if (index >= n - 1)
    {
        // arrayParam.resize(sortedEnd + 1);
        SorterArrayEvent event{SORTER_ARRAY_EVENT_END};
        return event;
    }

    if (arrayParam[index] <= arrayParam[index + 1])
    {
        SorterArrayEvent event{SORTER_ARRAY_EVENT_CMP};
        event.addElementEvent(ELEMENT_EVENT_READ, index);
        event.addElementEvent(ELEMENT_EVENT_READ, index + 1);
        event.valueA = index;
        event.valueB = index + 1;

        ++index;
        return event;
    }
    else
    {
        SorterArrayEvent event{SORTER_ARRAY_EVENT_REMOVE};
        event.addElementEvent(ELEMENT_EVENT_REMOVE, index + 1);
        event.valueA = index + 1;

        arrayParam.erase(arrayParam.begin() + index + 1);
        return event;
    }
    /*
        if (arrayParam[index] >= arrayParam[sortedEnd])
        {
            sortedEnd++;
            std::swap(arrayParam[sortedEnd], arrayParam[index]);

            SorterArrayEvent event{SORTER_ARRAY_EVENT_SET};
            event.addElementEvent(ELEMENT_EVENT_WRITE, sortedEnd);
            event.addElementEvent(ELEMENT_EVENT_WRITE, index);
            event.valueA = sortedEnd;
            event.valueB = index;

            index++;
            return event;
        }
        else
        {
            SorterArrayEvent event{SORTER_ARRAY_EVENT_SWAP};
            event.addElementEvent(ELEMENT_EVENT_WRITE, index);
            event.addElementEvent(ELEMENT_EVENT_WRITE, index);
            event.valueA = index;
            event.valueB = index;

            index++;
            return event;
        }
        */
}