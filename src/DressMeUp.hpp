#pragma once
#include "plugin.hpp"
#include "widgets/DressMeUpDisplay.hpp"
#include "utils/ClothingManager.hpp"
#include "utils/Globals.hpp"
#include "utils/MathUtils.hpp"

#include <array>

struct DressMeUpGLWidget;
struct DressMeUpDisplay;
struct DressMeUp;

struct ShaderParameters
{
	float spotWidth = 1.2;	 // (0.5-1.5, default 0.9)
	float spotHeight = 0.65; // (0.5-1.5, default 0.65)
	float colorBoost = 1.45; // (1.0-2.0, default 1.45)
	float inputGamma = 2.4;	 // (default 2.4)
	float outputGamma = 2.2; // (default 2.2)

	float effectScale = 2.5f;
};

struct DressMeUp : Module
{
	enum ParamId
	{
		STEPS_COUNT_PARAM,
		STEP_CV_RANGE_PARAM,
		VALUE_CV_RANGE_PARAM,
		OUTPUT_CV_RANGE_PARAM,
		SMOOTH_STEP_PARAM,
		QUANTIZE_VALUES_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		RESET_INPUT,
		PREV_INPUT,
		STEP_INPUT,
		STEPCV_INPUT,
		VALUEACV_INPUT = 10,
		VALUEBCV_INPUT,
		VALUECCV_INPUT,
		VALUEDCV_INPUT,
		VALUECURRENTCV_INPUT,
		CLOCKSTEP_INPUT,
		INPUTS_LEN
	};
	enum OutputId
	{
		OUTPUT_OUTPUT,
		STEP_OUTPUT,
		SUM_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		SMOOTH_STEP_LIGHT,
		QUANTIZE_VALUES_LIGHT,
		LIGHTS_LEN
	};

	DressMeUpDisplay *dressDisplay = nullptr;

	std::shared_ptr<Image> demoImage;

	bool drawDisplayDirty = true;

	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger prevTrigger;

	dsp::PulseGenerator stepPulse;

	bool enableOutputFilter = false;
	HPFilter hpFilter =
		HPFilter(44100, 100);

	std::vector<float> steps;
	ClothingManager clothingManager =
		ClothingManager(&steps);

	int stepIndex = 0;
	float step = 0.0f;
	float lastValue = 0.0f;

	int stepCount = 4;

	bool enableShader = true;
	ShaderParameters shaderParams;

	DressMeUp();

	float getStepValue(int index);

	void process(const ProcessArgs &args) override;

	json_t *toJson() override;

	void fromJson(json_t *rootJ) override;

	void onReset (const ResetEvent &e) override;

	void onRandomize (const RandomizeEvent &e) override;
};

struct DressMeUpWidget : ModuleWidget
{

	DressMeUp *module = nullptr;
	DressMeUpDisplay *dressMeUpDisplay;

	DressMeUpWidget(DressMeUp *module);

	void appendContextMenu(Menu *menu) override;

	void drawLayer(const DrawArgs &args, int layer) override;
};
