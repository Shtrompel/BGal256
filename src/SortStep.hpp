#pragma once
#include "plugin.hpp"
#include "utils/SorterArray.hpp"
#include "widgets/SorterArrayScreen.hpp"
#include <unordered_map>

struct SortStep : Module {
    enum ParamId {
        SCALE_PARAM,
        RESET_BUTTON_PARAM,
        SHUFFLE_BUTTON_PARAM,
        RECALCULATE_BUTTON_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        STEP_INPUT,
        RESET_INPUT,
        SHUFFLE_STEP_INPUT,
        SORT_STEP_INPUT,
        TRAVERSE_STEP_INPUT,
        RANDOMIZE_INPUT,
        RECALCULATE_INPUT,
        ALGORITHM_INPUT,
        ARRAYSIZE_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        //STEP_OUTPUT,
        CV_OUTPUT,
        VALUE_OUTPUT,
        END_OUTPUT,
        SHUFFLE_END_OUTPUT,
        SORT_END_OUTPUT,
        TRAVERSE_END_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    SorterArrayWidget* sorterScreen = nullptr;
    Module* expander = nullptr, *expanderR = nullptr;

    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger randomizeTrigger;
    dsp::SchmittTrigger stepTrigger;
    std::unordered_map<InputId, dsp::SchmittTrigger> triggersMap;

    SorterArray sorterArray;
    dsp::PulseGenerator pulseDone;
    dsp::PulseGenerator pulseShuffle, pulseSort, pulseTraverse;
    dsp::PulseGenerator pulseStepOut[2];
    bool recalcOnChange = false;

    int lastAlgorithm;


    SortStep();
    bool checkTrigger(InputId inputId);
    void outputEvent(SorterArrayEvent* event);
    void process(const ProcessArgs& args) override;

    void onReset(const ResetEvent &) override;

    void onRandomize(const RandomizeEvent &) override;

    json_t*  dataToJson() override;

    void dataFromJson(json_t* rootJ) override;
};

struct SortStepWidget : ModuleWidget {

    SortStep* module;

    SortStepWidget(SortStep* module);
};