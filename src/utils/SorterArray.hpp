#ifndef _SORTER_ARRAY
#define _SORTER_ARRAY

#include <map>
#include <vector>
#include <random>

#include "plugin.hpp"

#include "SorterAlgorithm.hpp"

#define AAAAA() DEBUG("%d", __LINE__)

typedef int section_t;
constexpr section_t SECTION_SHUFFLE = 0;
constexpr section_t SECTION_STEP = 1;
constexpr section_t SECTION_TRAVERSE = 2;
constexpr section_t SECTIONS_COUNT = 3;

struct Scale
{
    std::string name = "Chromatic";
    std::vector<int> intervals = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    json_t *toJson();

    void fromJson(json_t *rootJ);
};

struct SorterArray
{

    bool isFromJson = false;

    std::vector<int> array;
    size_t arraySize = 0;

    std::map<t_algorithmtype, SorterAlgorithm *> algorithms;

    int shuffleSkip = 1;
    int traverseSkip = 1;

    int eventIndex = 0;
    // For stepShuffle and stepTraverse
    int shuffleIndex = 0;
    int traversalIndex = 0;
    int shuffleFrames = 0;
    int traverseFrames = 0;

    bool enableKeyOutput = false;
    Scale outScale{};
    int keyOffset{};

    std::vector<SorterArrayEvent> events;

    // Local event to be stored in SorterArrayEvent
    SorterArrayEvent eventLocal = SorterArrayEvent(0);
    SorterArrayEvent *eventCurrent = nullptr;

    // state of the current algorithm
    t_algorithmtype currentType;
    SorterAlgorithm *algorithm = nullptr;

    std::mt19937 rng{};

    //std::thread workerThread;
    pthread_t workerThread{};
    volatile std::atomic_bool processingFinished = ATOMIC_VAR_INIT(true);
    volatile std::atomic_bool stopRequested =  ATOMIC_VAR_INIT(false);
    bool threadCreated = false;
    
   // std::future<void> workerFuture;

    bool filterEvent[SORTER_ARRAY_EVENTS_COUNT] = {false};
    int section = 0;

    bool tDone = false;
    bool tShuffle = false;
    bool tSort = false;
    bool tTraverse = false;

    SorterArray();

    ~SorterArray();

    void shuffle();

    void changeAlgorithm(t_algorithmtype type);

    void changeValue(int index, int value);

    void resetArray(size_t size);

    void resetArray();

    void resetData();

    void reset();

    void reset(int size, int line = -1, std::string source = "_");


    void startThread();

    void waitForThread();

    void stopCalculating();

    static void* threadFunction(void* arg);

    void processInThread();



    void calculate();

    SorterArrayEvent *step(); //

    SorterArrayEvent *stepShuffle();

    SorterArrayEvent *stepSort();

    SorterArrayEvent *stepTraverse();

    bool isDone();

    bool isDoneShuffle();

    bool isDoneSort();

    bool isDoneTraverse();

    bool triggerDone();

    bool triggerShuffle();

    bool triggerSort();

    bool triggerTraverse();

    json_t *toJson();

    void fromJson(json_t *rootJ);

    int size() const;
    int at(int i);
};

#endif // _SORTER_ARRAY