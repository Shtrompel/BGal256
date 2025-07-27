#include "SorterArray.hpp"

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static std::string printArray(const std::vector<int> &) __attribute__((unused));

static std::string printArray(const std::vector<int> &vec)
{
    std::string str = "";
    for (int x : vec)
        str += std::to_string(x) + ", ";
    return str;
}

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
#pragma GCC diagnostic pop
#endif

json_t *Scale::toJson()
{
    json_t *rootJ = json_object();
    json_object_set_new(rootJ, "name", json_string(name.c_str()));

    json_t *intervalsJ = json_array();
    for (int interval : intervals)
    {
        json_array_append_new(intervalsJ, json_integer(interval));
    }
    json_object_set_new(rootJ, "intervals", intervalsJ);

    return rootJ;
}

void Scale::fromJson(json_t *rootJ)
{
    // Name
    json_t *nameJ = json_object_get(rootJ, "name");
    if (nameJ)
        name = json_string_value(nameJ);

    // Intervals
    intervals.clear();
    json_t *intervalsJ = json_object_get(rootJ, "intervals");
    if (intervalsJ)
    {
        size_t index;
        json_t *value;
        json_array_foreach(intervalsJ, index, value)
        {
            intervals.push_back(json_integer_value(value));
        }
    }
}

#define DEFINE_ALGORITHM(enumType, classType) \
    algorithms[enumType] =                    \
        dynamic_cast<SorterAlgorithm *>(        \
            new struct classType());

SorterArray::SorterArray()
{
    std::random_device rd;
    this->rng = std::mt19937(rd());

    // instantiate algorithms
    DEFINE_ALGORITHM(t_algorithmtype::BUBBLE, BubbleSort);
    DEFINE_ALGORITHM(t_algorithmtype::INSERTION, InsertionSort);
    DEFINE_ALGORITHM(t_algorithmtype::BINARY_INSERTION, BinaryInsertionSort);
    DEFINE_ALGORITHM(t_algorithmtype::SELECTION, SelectionSort);
    DEFINE_ALGORITHM(t_algorithmtype::DOUBLE_SELECTION, DoubleSelectionSort);
    DEFINE_ALGORITHM(t_algorithmtype::COCKTAIL_SHAKER, CocktailShakerSort);
    DEFINE_ALGORITHM(t_algorithmtype::MERGE, MergeSort);
    DEFINE_ALGORITHM(t_algorithmtype::COMB, CombSort);
    DEFINE_ALGORITHM(t_algorithmtype::GNOME, GnomeSort);
    DEFINE_ALGORITHM(t_algorithmtype::OPTIMIZED_GNOME, OptimizedGnomeSort);
    DEFINE_ALGORITHM(t_algorithmtype::ODD_EVEN, OddEvenSort);
    DEFINE_ALGORITHM(t_algorithmtype::SHELL, ShellSort);
    DEFINE_ALGORITHM(t_algorithmtype::HEAP, HeapSort);
    DEFINE_ALGORITHM(t_algorithmtype::SMOOTH, SmoothSort);
    DEFINE_ALGORITHM(t_algorithmtype::BITONIC, BitonicSort);
    DEFINE_ALGORITHM(t_algorithmtype::QUICK, QuickSort);
    DEFINE_ALGORITHM(t_algorithmtype::BINARY_QUICK, BinaryQuickSort);
    DEFINE_ALGORITHM(t_algorithmtype::RADIX_LSD, RadixSortLSD);
    DEFINE_ALGORITHM(t_algorithmtype::RADIX_MSD, RadixSortMSD);
    
    algorithms[t_algorithmtype::RADIX_LSD_2] = 
        dynamic_cast<SorterAlgorithm *>( 
            new struct RadixSortLSD(2, t_algorithmtype::RADIX_LSD_2, "Radix Sort LSD (base 2)"));
    algorithms[t_algorithmtype::RADIX_MSD_2] = 
        dynamic_cast<SorterAlgorithm *>( 
            new struct RadixSortMSD(2, t_algorithmtype::RADIX_MSD_2, "Radix Sort MSD (base 2)"));
    
    algorithms[t_algorithmtype::RADIX_LSD_16] = 
        dynamic_cast<SorterAlgorithm *>(
            new struct RadixSortLSD(16, t_algorithmtype::RADIX_LSD_16, "Radix Sort LSD (base 16)"));
    algorithms[t_algorithmtype::RADIX_MSD_16] = 
        dynamic_cast<SorterAlgorithm *>(
            new struct RadixSortMSD(16, t_algorithmtype::RADIX_MSD_16, "Radix Sort MSD (base 16)"));
    
    DEFINE_ALGORITHM(t_algorithmtype::CYCLE, CycleSort);
    DEFINE_ALGORITHM(t_algorithmtype::BOGO, BogoSort);
    DEFINE_ALGORITHM(t_algorithmtype::EXCHANGE_BOGO, ExchangeBogoSort);
    DEFINE_ALGORITHM(t_algorithmtype::STALIN, StalinSort);

    #ifdef ENABLE_TEST_SORT
    DEFINE_ALGORITHM(t_algorithmtype::TEST, TestSort);
    #endif

    for (auto &algorithm : algorithms)
    {
        algorithm.second->externalStopFlag = &stopRequested;
    }

    changeAlgorithm(t_algorithmtype::BUBBLE);
}

SorterArray::~SorterArray()
{
    stopCalculating();
    for (auto &algorithm : algorithms)
        delete algorithm.second;
}

void SorterArray::shuffle()
{
    if (!processingFinished)
        stopCalculating();
    reset();
    std::shuffle(array.begin(), array.end(), rng);
    calculate();
}

void SorterArray::changeAlgorithm(t_algorithmtype type)
{
    auto itr = algorithms.find(type);
    if (itr != algorithms.end())
    {
        this->currentType = type;
        this->algorithm = itr->second;
    }
}

void SorterArray::changeValue(int index, int value)
{
    if (index >= 0 && index < static_cast<int>(array.size()))
    {
        array[index] = value;
    }
    calculate();
}

void SorterArray::resetArray(size_t size)
{
    if (processingFinished)
    {
        this->arraySize = size;
        array.clear();
        for (size_t i = 0; i < size; ++i)
            array.push_back(i);
    }
}

void SorterArray::resetArray()
{
    array.resize(arraySize);
    for (size_t i = 0; i < arraySize; ++i)
        array[i] = i;
}

void SorterArray::resetData()
{
    this->traversalIndex = 0;
    this->shuffleIndex = 0;
    this->section = SECTION_SHUFFLE;

    this->events.clear();
    eventIndex = 0;
    eventCurrent = nullptr;

    for (auto &algorithm : algorithms)
        algorithm.second->reset();
}

void SorterArray::reset()
{
    stopCalculating();
    resetData();
    resetArray();
    calculate();
}

void SorterArray::reset(int size, int line, std::string source)
{
    stopCalculating();
    resetData();
    resetArray((size_t)size);
    calculate();
}

void SorterArray::startThread()
{
    waitForThread();

    // Reset control flags
    stopRequested = false;
    processingFinished = false;

    // Create thread attributes
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Create the thread
    int result = pthread_create(&workerThread, &attr, &SorterArray::threadFunction, this);
    pthread_attr_destroy(&attr);

    if (result == 0)
    {
        threadCreated = true;
    }
    else
    {
        // Handle thread creation error
        processingFinished = true;
    }
}

void SorterArray::waitForThread()
{
    if (threadCreated)
    {
        pthread_join(workerThread, NULL);
        threadCreated = false;
    }
}

void SorterArray::stopCalculating()
{
    if (!processingFinished)
    {
        stopRequested = true;
        waitForThread();
    }
}

void *SorterArray::threadFunction(void *arg)
{
    SorterArray *instance = static_cast<SorterArray *>(arg);
    instance->processInThread();
    return NULL;
}

void SorterArray::processInThread()
{
    // array is copied inside SorterAlgorithm::calculate
    this->algorithm->calculate(array);

    this->events = this->algorithm->events;
    this->eventIndex = 0;
    this->processingFinished = true;
}

void SorterArray::calculate()
{
    //DEBUG("wetfftwtfw %d %d", __LINE__, (int)processingFinished);
    if (!processingFinished)
        return;
   // DEBUG("wetfftwtfw %d %d", __LINE__, (int)processingFinished);

    stopRequested = false;

    if (algorithm)
    {
        algorithm->events.clear();
        if (algorithm->doPrecompute())
        {
            startThread();
        }
        else
        {
            this->processingFinished = true;
        }
    }
}

SorterArrayEvent *SorterArray::step()
{
    if (!processingFinished)
        return nullptr;

    if (section == SECTION_SHUFFLE)
    {
        if (isDoneShuffle())
        {
            shuffleIndex = 0;
            calculate();
            this->section = SECTION_STEP;
            tShuffle = true;
        }
        else
            return stepShuffle();
    }

    if (!processingFinished)
    {
        return nullptr;
    }

    if (section == SECTION_STEP)
    {
        bool doneBool = isDoneSort();
        if (doneBool)
        {
            this->section = SECTION_TRAVERSE;
            tSort = true;
        }
        else
        {
            SorterArrayEvent *event = stepSort();
            return event;
        }
    }

    if (section == SECTION_TRAVERSE)
    {
        if (isDoneTraverse())
        {
            traversalIndex = 0;
            section = SECTIONS_COUNT;
            tTraverse = true;
            tDone = true;
        }
        else
            return stepTraverse();
    }

    return nullptr;
}

SorterArrayEvent *SorterArray::stepShuffle()
{
    if (!processingFinished)
        return nullptr;

    this->section = SECTION_SHUFFLE;
    this->traversalIndex = 0;
    this->traverseFrames = 0;
    this->eventIndex = 0;

    // Fisher-Yates shuffle

    if (shuffleFrames++ % shuffleSkip != 0)
    {
        return nullptr;
    }

    if (shuffleIndex >= (int)array.size())
    {
        tShuffle = true;
        shuffleIndex = 0;
    }

    {
        eventLocal.eventType = SORTER_ARRAY_EVENT_SWAP;
        eventLocal.valueA = shuffleIndex;

        std::uniform_int_distribution<int> int_dist(0, array.size() - 1);
        eventLocal.valueB = int_dist(rng);

        eventLocal.elements = {
            SelectedType{ELEMENT_EVENT_WRITE, shuffleIndex},
            SelectedType{ELEMENT_EVENT_WRITE, eventLocal.valueB}};

        int tmp = array[shuffleIndex];
        array[shuffleIndex] = array[eventLocal.valueB];
        array[eventLocal.valueB] = tmp;

        ++shuffleIndex;

        eventCurrent = &eventLocal;
        return &eventLocal;
    }

    return nullptr;
}

#define IS_BETWEEN(x, s) (0 <= (int)x && (int)x < (int)s)

SorterArrayEvent *SorterArray::stepSort()
{
    if (!algorithm->doPrecompute())
    {
        eventLocal = algorithm->step(array);
        this->eventCurrent = &eventLocal;
        if (this->eventCurrent->eventType == SORTER_ARRAY_EVENT_END)
        {
            tSort = true;
            return nullptr;
        }
        if (filterEvent[eventLocal.eventType])
        {
            return nullptr;
        }
        return &eventLocal;
    }

    if (!processingFinished || events.empty())
        return nullptr;

    if (eventIndex >= (int)this->events.size())
    {
        // DEBUG("q3y3q4yffy: %d %d", (int)eventIndex, (int)this->events.size());
        calculate();
        tSort = true;
        return nullptr;
    }

    this->section = SECTION_STEP;
    this->traversalIndex = 0;
    this->shuffleIndex = 0;
    this->shuffleFrames = 0;
    this->traverseFrames = 0;

    while (eventIndex < (int)this->events.size())
    {
        SorterArrayEvent &event = this->events[eventIndex++];
        this->eventCurrent = &event;

        switch (event.eventType)
        {
        case SORTER_ARRAY_EVENT_NONE:
        case SORTER_ARRAY_EVENT_CMP:
        case SORTER_ARRAY_EVENT_READ:
            break;

        case SORTER_ARRAY_EVENT_SWAP:
        {
            if (IS_BETWEEN(event.valueA, array.size()) && IS_BETWEEN(event.valueB, array.size()))
            {
                int tmp = array[event.valueA];
                array[event.valueA] = array[event.valueB];
                array[event.valueB] = tmp;
            }
        }
        break;

        case SORTER_ARRAY_EVENT_MOVE:
            if (IS_BETWEEN(event.valueA, array.size()) && IS_BETWEEN(event.valueB, array.size()))
                array[event.valueA] = array[event.valueB];
            break;

        case SORTER_ARRAY_EVENT_SET:
            if (IS_BETWEEN(event.valueA, array.size()))
                array[event.valueA] = event.valueB;
            break;

        case SORTER_ARRAY_EVENT_REMOVE:
            // Should be used only in non deterministic algorithms
            break;
        }

        if (filterEvent[event.eventType])
        {
            continue;
        }

        return &event;
    }

    return nullptr;
}

SorterArrayEvent *SorterArray::stepTraverse()
{
    if (!processingFinished)
        return nullptr;

    this->section = SECTION_TRAVERSE;
    this->eventIndex = 0;
    this->shuffleIndex = 0;
    this->shuffleFrames = 0;

    if (traverseSkip && traverseFrames++ % traverseSkip != 0)
    {
        return nullptr;
    }

    if (traversalIndex >= (int)array.size() || traversalIndex < 0)
    {
        traversalIndex = 0;
        tTraverse = true;
    }

    eventLocal.eventType = SORTER_ARRAY_EVENT_READ;
    eventLocal.valueA = traversalIndex;

    eventLocal.elements = {
        SelectedType{ELEMENT_EVENT_READ, traversalIndex}};

    ++traversalIndex;

    eventCurrent = &eventLocal;
    return &eventLocal;
}

bool SorterArray::isDone()
{
    return section > SECTION_TRAVERSE;
}

bool SorterArray::isDoneShuffle()
{
    return shuffleIndex >= (int)array.size();
}

bool SorterArray::isDoneSort()
{
    if (!processingFinished)
        return false;

    if (array.empty())
        return true;

    if (algorithm->doPrecompute())
    {
        if (events.empty() ||
            eventIndex < 0 ||
            eventIndex >= (int)events.size())
        {
            return true;
        }

        if (events[eventIndex].eventType == SORTER_ARRAY_EVENT_END)
        {
            return true;
        }
    }
    else
    {
        if (!eventCurrent)
            return false;

        if (eventCurrent->eventType == SORTER_ARRAY_EVENT_END)
        {
            return true;
        }
    }

    /*
    return !processingFinished ||
           events.empty() ||
           array.empty() ||
           eventIndex < 0 ||
           eventIndex >= (int)events.size();
           */
    return false;
}

bool SorterArray::isDoneTraverse()
{
    return traversalIndex >= (int)array.size();
}

bool SorterArray::triggerDone()
{
    bool tmp = tDone;
    tDone = false;
    return tmp;
}

bool SorterArray::triggerShuffle()
{
    bool tmp = tShuffle;
    tShuffle = false;
    return tmp;
}

bool SorterArray::triggerSort()
{
    bool tmp = tSort;
    tSort = false;
    return tmp;
}

bool SorterArray::triggerTraverse()
{
    bool tmp = tTraverse;
    tTraverse = false;
    return tmp;
}

json_t *SorterArray::toJson()
{
    json_t *root = json_object();

    // Serialize the internal std::vector<int> array
    json_t *arrayJson = json_array();
    for (int value : array)
    {
        json_array_append_new(arrayJson, json_integer(value));
    }
    json_object_set_new(root, "array", arrayJson);

    // serialize integers
    json_object_set_new(root, "arraySize", json_integer(arraySize));
    json_object_set_new(root, "shuffleSkip", json_integer(shuffleSkip));
    json_object_set_new(root, "traverseSkip", json_integer(traverseSkip));
    json_object_set_new(root, "eventIndex", json_integer(eventIndex));
    json_object_set_new(root, "shuffleIndex", json_integer(shuffleIndex));
    json_object_set_new(root, "traversalIndex", json_integer(traversalIndex));
    json_object_set_new(root, "shuffleFrames", json_integer(shuffleFrames));
    json_object_set_new(root, "traverseFrames", json_integer(traverseFrames));
    json_object_set_new(root, "keyOffset", json_integer(keyOffset));
    json_object_set_new(root, "currentType", json_integer(static_cast<int>(currentType)));
    json_object_set_new(root, "section", json_integer(static_cast<int>(section)));

    // serialize booleans
    json_object_set_new(root, "enableKeyOutput", json_boolean(enableKeyOutput));
    json_object_set_new(root, "tDone", json_boolean(tDone));
    json_object_set_new(root, "tShuffle", json_boolean(tShuffle));
    json_object_set_new(root, "tSort", json_boolean(tSort));
    json_object_set_new(root, "tTraverse", json_boolean(tTraverse));

    // serialize the scale object
    json_object_set_new(root, "outScale", outScale.toJson());

    // serialize events vector std::vector<SorterArrayEvent>
    json_t *eventsJson = json_array();
    for (const SorterArrayEvent &event : events)
    {
        json_t *eventObj = json_object();

        // Serialize primitive members
        json_object_set_new(eventObj, "eventType", json_integer(event.eventType));
        json_object_set_new(eventObj, "valueA", json_integer(event.valueA));
        json_object_set_new(eventObj, "valueB", json_integer(event.valueB));

        // Serialize elements vector
        json_t *elementsArray = json_array();
        for (const SelectedType &element : event.getElements())
        {
            json_t *elementObj = json_object();
            json_object_set_new(elementObj, "event", json_integer(element.getEvent()));
            json_object_set_new(elementObj, "index", json_integer(element.getIndex()));
            json_array_append_new(elementsArray, elementObj);
        }
        json_object_set_new(eventObj, "elements", elementsArray);

        json_array_append_new(eventsJson, eventObj);
    }
    json_object_set_new(root, "events", eventsJson);

    // Serialize current algorithm type
    json_object_set_new(root, "outScale", outScale.toJson());

    // Serialize filterEvent array
    json_t *filterEventJson = json_array();
    for (int i = 0; i < (int)SORTER_ARRAY_EVENTS_COUNT; i++)
    {
        json_array_append_new(filterEventJson, json_boolean(filterEvent[i]));
    }
    json_object_set_new(root, "filterEvent", filterEventJson);

    // Serialize section
    json_object_set_new(root, "section", json_integer(section));

    return root;
}

void SorterArray::fromJson(json_t *rootJ)
{
    this->isFromJson = true;

    json_t *j;

    // Deserialize array
    json_t *arrayJson = json_object_get(rootJ, "array");
    if (arrayJson)
    {
        array.clear();
        size_t index;
        json_t *value;
        json_array_foreach(arrayJson, index, value)
        {
            array.push_back(json_integer_value(value));
        }
    }

    // Deserialize integers
    // Repeat similarly for:
    // traverseSkip, eventIndex, shuffleIndex, traversalIndex,
    // shuffleFrames, traverseFrames, keyOffset, section
    // (Code omitted for brevity - same pattern as above)
    if ((j = json_object_get(rootJ, "arraySize")))
        arraySize = (size_t)json_integer_value(j);
    if ((j = json_object_get(rootJ, "shuffleSkip")))
        shuffleSkip = json_integer_value(j);
    if ((j = json_object_get(rootJ, "traverseSkip")))
        traverseSkip = json_integer_value(j);
    if ((j = json_object_get(rootJ, "eventIndex")))
        eventIndex = json_integer_value(j);
    if ((j = json_object_get(rootJ, "shuffleIndex")))
        shuffleIndex = json_integer_value(j);
    if ((j = json_object_get(rootJ, "traversalIndex")))
        traversalIndex = json_integer_value(j);
    if ((j = json_object_get(rootJ, "shuffleFrames")))
        shuffleFrames = json_integer_value(j);
    if ((j = json_object_get(rootJ, "traverseFrames")))
        traverseFrames = json_integer_value(j);
    if ((j = json_object_get(rootJ, "enableKeyOutput")))
        enableKeyOutput = json_boolean_value(j);
    if ((j = json_object_get(rootJ, "keyOffset")))
        keyOffset = json_integer_value(j);
    if ((j = json_object_get(rootJ, "currentType")))
        currentType = (t_algorithmtype)json_integer_value(j);
    if ((j = json_object_get(rootJ, "section")))
        section = json_integer_value(j);

    // Deserialize booleans
    json_t *j_enableKeyOutput = json_object_get(rootJ, "enableKeyOutput");
    if (j_enableKeyOutput)
        enableKeyOutput = json_boolean_value(j_enableKeyOutput);

    // Repeat for tDone, tShuffle, tSort, tTraverse
    if ((j = json_object_get(rootJ, "tDone")))
        tDone = json_integer_value(j);
    if ((j = json_object_get(rootJ, "tShuffle")))
        tShuffle = json_boolean_value(j);
    if ((j = json_object_get(rootJ, "tSort")))
        tSort = json_integer_value(j);
    if ((j = json_object_get(rootJ, "tTraverse")))
        tTraverse = json_integer_value(j);

    // Deserialize Scale
    json_t *j_outScale = json_object_get(rootJ, "outScale");
    if (j_outScale)
        outScale.fromJson(j_outScale); // Assumes fromJson()

    // Deserialize std::vector<SorterArrayEvent> events
    events.clear();
    json_t *eventsArray = json_object_get(rootJ, "events");
    if (eventsArray)
    {
        size_t eventIndex;
        json_t *eventObj;
        json_array_foreach(eventsArray, eventIndex, eventObj)
        {
            SorterArrayEvent event;

            // Deserialize primitive members
            event.eventType = json_integer_value(json_object_get(eventObj, "eventType"));
            event.valueA = json_integer_value(json_object_get(eventObj, "valueA"));
            event.valueB = json_integer_value(json_object_get(eventObj, "valueB"));

            // Deserialize elements vector
            json_t *elementsArray = json_object_get(eventObj, "elements");
            if (elementsArray)
            {
                size_t elementIndex;
                json_t *elementObj;
                json_array_foreach(elementsArray, elementIndex, elementObj)
                {
                    SelectedType element;
                    element.elementEvent = json_integer_value(json_object_get(elementObj, "event"));
                    element.index = json_integer_value(json_object_get(elementObj, "index"));
                    event.elements.push_back(element);
                }
            }

            events.push_back(event);
        }
    }

    // Deserialize filterEvent array
    json_t *filterEventJson = json_object_get(rootJ, "filterEvent");
    if (filterEventJson)
    {
        for (int i = 0; i < (int)SORTER_ARRAY_EVENTS_COUNT; i++)
        {
            json_t *b = json_array_get(filterEventJson, i);
            if (b)
                filterEvent[i] = json_boolean_value(b);
        }
    }

    // Reset transient state
    isFromJson = true; // Flag to handle post-load initialization
    eventCurrent = nullptr;
    processingFinished = true;

    waitForThread();
}

// helper getters
int SorterArray::size() const
{
    return (int)array.size();
}

int SorterArray::at(int i)
{
    if (i < 0 || i >= static_cast<int>(array.size())) {
        return 0;
    }
    return array[i];
}