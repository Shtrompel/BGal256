#ifndef _GROSS_TOILET
#define _GROSS_TOILET

#include "plugin.hpp"
#include "asset.hpp"
#include <osdialog.h>

#include "dep/dr_wav/dr_wav.h"

#include <array>
#include <iomanip>

#include "widgets/BPMDisplay.hpp"
#include "widgets/BufferWidget.hpp"
#include "utils/MathUtils.hpp"

#define AAAAA() INFO("Got Here: %d", __LINE__);


// A value set with trial and error
// Used to match the input audio when it is expected to via
// automation. 

// Trial and error number so the out audio will try to match 
// the inputed audio instead of skipping forward. 
// A better solution should be found since there is an audible
// audio offset effect when dry input is enabled.
constexpr int GNOME_PLEASING_NUMBER = 11;

struct JwHorizontalSwitch : SVGSwitch {
    JwHorizontalSwitch();
};

#define ABS(x)((x>0?x:-x))


struct BufferSludger : Module {
    enum ParamId {
        BPM_PARAM,
        MODE_PARAM,
        EXTERNAL_BPM_PARAM,
        MIX_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        STEP_INPUT,
        RESET_INPUT,
        PHASE_INPUT,
        CLEAR_INPUT,
        AUDIO_INPUT,
        AUTOMATION_INPUT,
        MIX_INPUT,
        AUDIO_RIGHT_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        AUDIO_OUTPUT,
        AUDIO_RIGHT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        CLOCK_STEP_LIGHT,
        OUTPUT_LIGHT,
        EXTERNAL_BPM_LIGHT,
        LIGHTS_LEN
    };

    static const int AUTOMATION_MODE_LINEAR = 1;
    static const int AUTOMATION_MODE_DERIVATIVE = 2;

    static const int INTERPOLATION_MODE_NONE = 1;
    static const int INTERPOLATION_MODE_LINEAR = 2;
    static const int INTERPOLATION_MODE_CUBIC = 3;
    static const int INTERPOLATION_MODE_OPTIMAL_2X = 4;
    static const int INTERPOLATION_MODE_OPTIMAL_8X = 5;
    static const int INTERPOLATION_MODE_OPTIMAL_32X = 6;
    
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger phaseTrigger;
    dsp::SchmittTrigger clearBufferTrigger;

    //BufferSludgerTransposer* transposerExtendor = nullptr;


    int bpmValue = -1;

    float masterLength = 0.5;
    float timeSinceStep = 0.0;
    float phaseOut = 0.0;

    float diffAdd = 0;
    float automationPhase = 0.0;

    // Keep stuff from last process
    float lmasterLength = -1;
    float lastPhaseIn = 0.0;
    float lastAutomationIn = 0.0;
    float lautomationPhase = 0.0;
    int lsampleRate = -1;

    // Anti Clicking Fader
    bool antiClickFilter = true;
    float fadeGain = 1.0f;
    int fadeCounter = 0; // Samples passed since last click
    long lastOutputIndex = -1;
    float lastOutput[2];
    float rampSamplesMs = 15;

    //
    float externalAutomation = 0.0f;

    bool isStereo = false;

    // If audio plays at maxSpeed4Filter speed, assume its a click to filter it
    const float maxSpeed4Filter = 32;

    bool firstBeat = true;

    int visualMode = BUFFER_DISPLAY_DRAW_MODE_SAMPLES;
    int automationMode = AUTOMATION_MODE_LINEAR;
    int8_t interpolationMode = INTERPOLATION_MODE_OPTIMAL_8X;
    bool enableOutputFilter = true;
    bool enableSpeedChange = false;

    int uiDownsampling = 32;

    long recordingIndex = 0;
    long outputIndex = 0; // For ui only

    std::array<std::vector<float>, 2> samples;
    int lastResizeFrame = 0; // Avoid calling samples.resize too many times
    float output[2] = {0.0, 0.0};

    LowPassFilter outFilter[2];


    BufferSludger();

    void onReset() override;

    void reset(bool resetFirstBeat = false);

    void resizeBuffer(int sampleRate, bool disableSpeed = false);

    void resizeBufferSpeed(int sampleRate, float speed);

    void process(const ProcessArgs& args) override;

    void loadWavFile();

    json_t* toJson() override;

    void fromJson(json_t* rootJ) override;
};

struct BufferSludgerWidget : ModuleWidget {
    BufferSludgerWidget(BufferSludger* module);

    void appendContextMenu(Menu* menu) override;
};

#endif // _GROSS_TOILET