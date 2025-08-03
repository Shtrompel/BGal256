#define GLEW_STATIC
#include <GL/glew.h>
//  #include <GL/gl.h>
#include "DressMeUp.hpp"

DressMeUp::DressMeUp()
{
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

	configParam(STEP_CV_RANGE_PARAM, 0.f, 2.f, 0.f,
				"Step input range mode");
	configParam(VALUE_CV_RANGE_PARAM, 0.f, 2.f, 0.f,
				"Value range mode");
	configParam(OUTPUT_CV_RANGE_PARAM, 0.f, 10.f, 1.f,
				"Output Range");
	configParam(STEPS_COUNT_PARAM, 1.f, 4.f, 4.f,
				"Step count");
	paramQuantities[STEPS_COUNT_PARAM]->snapEnabled = true;
	configSwitch(SMOOTH_STEP_PARAM, 0.0f, 1.0f, 0.0f,
				 "Smooth Step", {"Snap Step", "Smooth Step"});
	configSwitch(QUANTIZE_VALUES_PARAM, 0.0f, 1.0f, 1.0f,
				 "Quantize Values", {"Disable", "Enable"});

	configInput(RESET_INPUT, "Reset");
	configInput(PREV_INPUT, "Previous Step");
	configInput(STEPCV_INPUT, "Set Step From Voltage");
	configInput(VALUECURRENTCV_INPUT, "Set Value Of Current Step");
	configInput(CLOCKSTEP_INPUT, "Next Step");
	configInput(VALUEACV_INPUT, "Step A Value");
	configInput(VALUEBCV_INPUT, "Step B Value");
	configInput(VALUECCV_INPUT, "Step C Value");
	configInput(VALUEDCV_INPUT, "Step D Value");

	configOutput(OUTPUT_OUTPUT, "Output Voltage");
	configOutput(STEP_OUTPUT, "Step Output");
	configOutput(SUM_OUTPUT, "Sum Output");

	steps.resize(CLOTHTYPE_COUNT);
}

float DressMeUp::getStepValue(int index)
{
	index = ((index % stepCount) + stepCount) % stepCount;
	if (params[QUANTIZE_VALUES_PARAM].getValue() == 1.0f)
	{
		if (dressDisplay && dressDisplay->enable)
		{
			float out = clothingManager.getClothId(index);
			int clothCount = clothingManager.getClothCounter(index);
			if (clothCount != 0)
				out /= clothCount;
			else
				out = 0.0;
			return out;
		}
		else
			return 0.0f;
	}
	else
	{
		return steps[index];
	}
}

void DressMeUp::process(const ProcessArgs &args)
{
	if ((int)hpFilter.getSampleRate() != args.sampleRate)
	{
		hpFilter.setCutoff(args.sampleRate, 100);
	}

	if (resetTrigger.process(inputs[RESET_INPUT].getVoltage()))
	{
		stepIndex = 0;
	}

	float stepCVMin = 0, stepCVMax = 0;
	float valueCVMin = 0, valueCVMax = 0;

	switch ((int)params[STEP_CV_RANGE_PARAM].getValue())
	{
	case 2:
		stepCVMin = 0;
		stepCVMax = 10;
		break;
	case 1:
		stepCVMin = -5;
		stepCVMax = 5;
		break;
	case 0:
		stepCVMin = -10;
		stepCVMax = 10;
		break;
	}

	switch ((int)params[VALUE_CV_RANGE_PARAM].getValue())
	{
	case 2:
		valueCVMin = 0;
		valueCVMax = 10;
		break;
	case 1:
		valueCVMin = -5;
		valueCVMax = 5;
		break;
	case 0:
		valueCVMin = -10;
		valueCVMax = 10;
		break;
	}

	stepCount = (int)params[STEPS_COUNT_PARAM].getValue();
	// if current step controlled by STEPCV_INPUT
	float step{};
	bool stepTrigger = false;
	if (inputs[STEPCV_INPUT].isConnected())
	{
		float fstep = inputs[STEPCV_INPUT].getVoltage();
		fstep = (fstep - stepCVMin) / (stepCVMax - stepCVMin);
		fstep = rack::math::clamp(fstep, 0.f, 1.f);
		step = (fstep * stepCount);
		stepIndex = (int)(step);
	}
	else
	{
		// normal trigger based step
		if (clockTrigger.process(inputs[CLOCKSTEP_INPUT].getVoltage()))
		{
			stepTrigger = true;
			stepIndex++;
		}
		if (prevTrigger.process(inputs[PREV_INPUT].getVoltage()))
		{
			stepTrigger = true;
			stepIndex--;
		}
	}

	stepIndex = ((stepIndex % stepCount) + stepCount) % stepCount;
	// for updating the current worn cloth
	
	auto updateClothValue = [this](
								DressMeUpDisplay *dressDisplay,
								float value,
								int stepIndex)
	{
		if (!dressDisplay || !dressDisplay->enable)
			return;

		int id = value * (clothingManager.getClothCounter(stepIndex) + 1);
		if (clothingManager.getClothId(stepIndex) != id)
		{
			dressDisplay->changeCloth(stepIndex, id);
			// clothingManager.changeCloth(stepIndex, id);
		}
	};
	

	// change the cloth from current selected position
	if (inputs[VALUECURRENTCV_INPUT].isConnected())
	{
		float value = inputs[VALUECURRENTCV_INPUT].getVoltage();
		value = (value - valueCVMin) / (valueCVMax - valueCVMin);
		steps[stepIndex] = rack::math::clamp(value, 0.0f, 1.0f);
		updateClothValue(dressDisplay, value, stepIndex);
	}

	// change the current worn cloth for every cloth position input
	for (int i = 0; i < stepCount; ++i)
	{
		auto &input = inputs[(InputId)((int)VALUEACV_INPUT + i)];
		if (!input.isConnected())
			continue;

		float value = input.getVoltage();
		value = (value - valueCVMin) / (valueCVMax - valueCVMin);
		this->steps[i] = value;
		updateClothValue(dressDisplay, value, i);
	}

	// output
	float out = 0.0f;
	float outA = 0.0f;
	if (params[SMOOTH_STEP_PARAM].getValue() == 0.0f)
	{
		out = outA = getStepValue(stepIndex);
	}
	else
	{
		float fract = fmodf(step, 1.0f);
		if (fract == 0.0f)
		{
			out = getStepValue(stepIndex);
		}
		else
		{
			outA = getStepValue(stepIndex);
			float outB = getStepValue((int)ceil(step));
			out = rangeMap(fract, 0.0f, 1.0f, outA, outB);
		}
	}

	bool stepOutBool;
	stepOutBool = stepTrigger;
	stepOutBool = stepOutBool || (params[QUANTIZE_VALUES_PARAM].getValue() == 1.0f &&
								  this->lastValue != outA);
	stepOutBool = stepOutBool && outA != 0.0f;

	if (stepOutBool)
	{
		stepPulse.trigger(1e-3f);
	}

	this->lastValue = outA;
	if (outputs[SUM_OUTPUT].isConnected())
	{
		float sum = 0.0f;
		for (int i = 0; i < stepCount; ++i)
			sum += getStepValue(i);
		sum *= params[OUTPUT_CV_RANGE_PARAM].getValue();
		outputs[SUM_OUTPUT].setVoltage(sum);
	}

	out *= params[OUTPUT_CV_RANGE_PARAM].getValue();
	if (enableOutputFilter)
	{
		out = hpFilter.process(out);
	}
	outputs[OUTPUT_OUTPUT].setVoltage(out);

	float triggerValue = stepPulse.process(args.sampleTime) ? 10.f : 0.f;
	outputs[STEP_OUTPUT].setVoltage(triggerValue);

	// update visuals
	if (dressDisplay && dressDisplay->enable)
		dressDisplay->highlightWorn(stepIndex);
	lights[SMOOTH_STEP_LIGHT].setBrightness(
		params[SMOOTH_STEP_PARAM].getValue());
	lights[QUANTIZE_VALUES_LIGHT].setBrightness(
		params[QUANTIZE_VALUES_PARAM].getValue());
}

json_t *DressMeUp::toJson()
{

	json_t *rootJ = Module::toJson();

	json_t *stepsJ = json_array();
	for (float s : steps)
	{
		json_array_append_new(stepsJ, json_real(s));
	}
	json_object_set_new(rootJ, "steps", stepsJ);

	// Load from, "clothingManager"
	json_t *clothCurrentJ = json_array();
	for (float s : clothingManager.clothCurrent)
	{
		json_array_append_new(clothCurrentJ, json_real(s));
	}
	json_object_set_new(rootJ, "clothCurrent", clothCurrentJ);

	json_object_set_new(rootJ, "stepIndex", json_integer(stepIndex));

	json_object_set_new(rootJ, "step", json_real(step));

	json_object_set_new(rootJ, "lastValue", json_real(lastValue));

	json_object_set_new(rootJ, "enableShader", json_boolean(enableShader));

	json_object_set_new(rootJ, "enableOutputFilter", json_boolean(enableOutputFilter));

	json_t *shaderJ = json_object();
	json_object_set_new(shaderJ, "spotWidth", json_real(shaderParams.spotWidth));
	json_object_set_new(shaderJ, "spotHeight", json_real(shaderParams.spotHeight));
	json_object_set_new(shaderJ, "colorBoost", json_real(shaderParams.colorBoost));
	json_object_set_new(shaderJ, "inputGamma", json_real(shaderParams.inputGamma));
	json_object_set_new(shaderJ, "outputGamma", json_real(shaderParams.outputGamma));
	json_object_set_new(shaderJ, "effectScale", json_real(shaderParams.effectScale));
	json_object_set_new(rootJ, "shaderParams", shaderJ);

	return rootJ;
}

// Deserialize widget state from JSON
void DressMeUp::fromJson(json_t *rootJ)
{
	Module::fromJson(rootJ);

	json_t *stepsJ = json_object_get(rootJ, "steps");
	if (json_is_array(stepsJ))
	{
		steps.clear();
		size_t idx;
		json_t *val;
		json_array_foreach(stepsJ, idx, val)
		{
			if (json_is_number(val))
			{
				steps.push_back((float)json_number_value(val));
			}
		}
	}

	// Load from, "clothingManager"
	json_t *clothCurrentJ = json_object_get(rootJ, "clothCurrent");
	if (json_is_array(clothCurrentJ))
	{
		size_t idx;
		json_t *val;
		json_array_foreach(clothCurrentJ, idx, val)
		{
			if (json_is_number(val))
			{
				if (dressDisplay)
					dressDisplay->changeCloth(idx, (float)json_number_value(val));
				else
					clothingManager.clothCurrent[idx] = ((float)json_number_value(val));
			}
		}
	}

	json_t *stepIdxJ = json_object_get(rootJ, "stepIndex");
	if (json_is_integer(stepIdxJ))
	{
		stepIndex = (int)json_integer_value(stepIdxJ);
	}

	json_t *stepJ = json_object_get(rootJ, "step");
	if (json_is_real(stepJ))
	{
		step = (int)json_real_value(stepJ);
	}

	json_t *lastValueJ = json_object_get(rootJ, "lastValue");
	if (json_is_real(lastValueJ))
	{
		lastValue = (int)json_real_value(lastValueJ);
	}

	json_t *enableJ = json_object_get(rootJ, "enableShader");
	if (json_is_boolean(enableJ))
	{
		enableShader = (bool)json_boolean_value(enableJ);
	}

	json_t *enableOutputFilterJ = json_object_get(rootJ, "enableOutputFilter");
	if (json_is_boolean(enableOutputFilterJ))
	{
		enableOutputFilter = (bool)json_boolean_value(enableOutputFilterJ);
	}

	json_t *shaderJ = json_object_get(rootJ, "shaderParams");
	if (json_is_object(shaderJ))
	{
		json_t *j;
		j = json_object_get(shaderJ, "spotWidth");
		if (json_is_number(j))
			shaderParams.spotWidth = (float)json_number_value(j);
		j = json_object_get(shaderJ, "spotHeight");
		if (json_is_number(j))
			shaderParams.spotHeight = (float)json_number_value(j);
		j = json_object_get(shaderJ, "colorBoost");
		if (json_is_number(j))
			shaderParams.colorBoost = (float)json_number_value(j);
		j = json_object_get(shaderJ, "inputGamma");
		if (json_is_number(j))
			shaderParams.inputGamma = (float)json_number_value(j);
		j = json_object_get(shaderJ, "outputGamma");
		if (json_is_number(j))
			shaderParams.outputGamma = (float)json_number_value(j);
		j = json_object_get(shaderJ, "effectScale");
		if (json_is_number(j))
			shaderParams.effectScale = (float)json_number_value(j);
	}
}

void DressMeUp::onReset(const ResetEvent &e)
{
	Module::onReset(e);
	step = 0;
	stepIndex = step;
	for (int i = 0; i < CLOTHTYPE_COUNT; ++i)
		dressDisplay->changeCloth(i, 0);
}

void DressMeUp::onRandomize(const RandomizeEvent &e)
{
	Module::onRandomize(e);

	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, CLOTHTYPE_COUNT - 1);

	// Random value range is [0, CLOTHTYPE_COUNT)
	step = distrib(gen);
	stepIndex = step;
	dressDisplay->highlightWorn(step);

	for (int i = 0; i < CLOTHTYPE_COUNT; ++i)
	{
		int count = clothingManager.getClothCounter(i);

		distrib = std::uniform_int_distribution<>(0, count - 1);

		int value = distrib(gen);
		// clothingManager.changeCloth(i, value);
		dressDisplay->changeCloth(i, value);
	}
}

struct ShaderParamQuantity : rack::Quantity
{
	bool &updateValue;
	float &value;
	float minValue, maxValue;
	std::string label;

	ShaderParamQuantity(
		bool &updateValue,
		float &v,
		float lo,
		float hi,
		const std::string &label)
		: updateValue(updateValue), value(v), minValue(lo), maxValue(hi), label(label) {}
	void setValue(float v) override
	{
		updateValue = true;
		// clamp to range
		value = rack::math::clamp(v, minValue, maxValue);
	}
	float getValue() override { return value; }
	float getMinValue() override { return minValue; }
	float getMaxValue() override { return maxValue; }
	std::string getDisplayValueString() override
	{
		return string::f("%.2f", value);
	}
	std::string getLabel() override
	{
		return label;
	}
};

struct VisualParamsMenuItem : MenuItem
{
	DressMeUp *module;
	Menu *createChildMenu() override;
};

DressMeUpWidget::DressMeUpWidget(DressMeUp *module)
{
	this->module = module;

	setModule(module);
	setPanel(createPanel(
		asset::plugin(pluginInstance, "res/DressMeUp.svg")));

	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(5.491 + 6.185 / 2, 88.874 + 3.089 / 2)), module, DressMeUp::RESET_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(29.265 + 6.185 / 2, 88.874 + 3.089 / 2)), module, DressMeUp::PREV_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(54.284 + 6.185 / 2, 93.529 + 2.032 / 2)), module, DressMeUp::STEPCV_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(29.265 + 6.185 / 2, 100.683 + 3.089 / 2)), module, DressMeUp::CLOCKSTEP_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(44.009 + 6.185 / 2, 116.211 + 6.185 / 2)), module, DressMeUp::VALUECURRENTCV_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(6.791 + 6.185 / 2, 116.211 + 6.185 / 2)), module, DressMeUp::VALUEACV_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(15.425 + 6.185 / 2, 116.211 + 6.185 / 2)), module, DressMeUp::VALUEBCV_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(24.060 + 6.185 / 2, 116.211 + 6.185 / 2)), module, DressMeUp::VALUECCV_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(32.695 + 6.185 / 2, 116.211 + 6.185 / 2)), module, DressMeUp::VALUEDCV_INPUT));

	addOutput(createOutputCentered<PJ301MPort>(
		mm2px(Vec(84.472 + 6.185 / 2, 93)), module, DressMeUp::OUTPUT_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(
		mm2px(Vec(84.472 + 6.185 / 2 - 5, 118)), module, DressMeUp::STEP_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(
		mm2px(Vec(84.472 + 6.185 / 2 + 5, 118)), module, DressMeUp::SUM_OUTPUT));

	// 3 mode switch
	addParam(createParamCentered<rack::componentlibrary::CKSSThree>(
		mm2px(Vec(64, 94.5)),
		module,
		DressMeUp::STEP_CV_RANGE_PARAM));
	addParam(createParamCentered<rack::componentlibrary::CKSSThree>(
		mm2px(Vec(63, 118.127 + 2.032 / 2)), // Position (in mm)
		module,
		DressMeUp::VALUE_CV_RANGE_PARAM));

	addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(
		mm2px(Vec(57.343 + 11.423 / 2, 105)), module,
		DressMeUp::SMOOTH_STEP_PARAM,
		DressMeUp::SMOOTH_STEP_LIGHT));
	addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(
		mm2px(Vec(53.887 + 6.499 / 2, 116.211 + 6.185 / 2)), module,
		DressMeUp::QUANTIZE_VALUES_PARAM,
		DressMeUp::QUANTIZE_VALUES_LIGHT));

	addParam(createParamCentered<RoundBlackKnob>(
		mm2px(Vec(83.814 + 7.500 / 2, 105)),
		module,
		DressMeUp::OUTPUT_CV_RANGE_PARAM));
	auto knobSteps = createParamCentered<RoundBlackKnob>(
		mm2px(Vec(5.491 + 7.500 / 2, 103.5)),
		module,
		DressMeUp::STEPS_COUNT_PARAM);
	knobSteps->minAngle = -0.25 * M_PI;
	knobSteps->maxAngle = 0.25 * M_PI;
	addParam(knobSteps);

	// mm2px(Vec(94.528, 84.528))
	// addChild(createWidget<Widget>(mm2px(Vec(3.536, 3.536))));w

	float cornerSize = 0.8;
	math::Vec pos = mm2px(Vec(
		4.110 + cornerSize,
		6.501 + cornerSize));
	math::Vec size = mm2px(Vec(
		93.381 - 2 * cornerSize,
		76.525 - 2 * cornerSize));
		
	DressMeUpDisplay *dressMeUpDisplay;
	dressMeUpDisplay = createWidget<DressMeUpDisplay>(pos);
	// dressMeUpDisplay = new DressMeUpDisplay();
	dressMeUpDisplay->box.pos = pos;
	dressMeUpDisplay->box.size = size;
	addChild(dressMeUpDisplay);

	if (!module)
		return;

	module->dressDisplay = dressMeUpDisplay;
	dressMeUpDisplay->clothingManager =
		&(module->clothingManager);

	dressMeUpDisplay->init(module, pos, size);
	DressMeUpGLWidget *dmud = dressMeUpDisplay->child;

	if (dmud)
	{
		ImageData &imageData = *dmud->loadImage(
			rack::asset::plugin(pluginInstance, "res/images/Background.png"));
		imageData.attributes.size = size;
		imageData.layer = 0;
	}

	dressMeUpDisplay->loadBody(
		"res/images/Body.png");

	// Load clothes
	std::string path = rack::asset::plugin(pluginInstance, "res/data/ClothingData.json");

	json_error_t error{};
	json_t *root = json_load_file(path.c_str(), 0, &error);
	if (!root)
	{
		WARN("JSON error: %s (line %d)", error.text, error.line);
		return;
	}

	if (!json_is_array(root))
	{
		WARN("Root is not an array");
		json_decref(root);
		return;
	}
	size_t index = 0;
	json_t *category = nullptr;
	json_array_foreach(root, index, category)
	{
		json_t *pos_obj = json_object_get(category, "pos");
		if (!json_is_string(pos_obj))
			continue;
		const char *pos_str = json_string_value(pos_obj);

		t_clothtype clothType;
		if (strcmp(pos_str, "head") == 0)
			clothType = CLOTHTYPE_HEAD;
		else if (strcmp(pos_str, "shirt") == 0)
			clothType = CLOTHTYPE_SHIRT;
		else if (strcmp(pos_str, "pants") == 0)
			clothType = CLOTHTYPE_PANTS;
		else if (strcmp(pos_str, "shoes") == 0)
			clothType = CLOTHTYPE_SHOES;
		else
			continue;

		// Get clothes array
		json_t *clothes = json_object_get(category, "clothes");
		if (!json_is_array(clothes))
			continue;

		// Iterate through clothes
		size_t cloth_index;
		json_t *cloth;
		json_array_foreach(clothes, cloth_index, cloth)
		{
			json_t *name_obj = json_object_get(cloth, "name");
			if (!json_is_string(name_obj))
				continue;
			std::string name = json_string_value(name_obj);

			json_t *center = json_object_get(cloth, "center");
			if (!center)
				continue;

			json_t *connected = json_object_get(cloth, "connected");

			json_t *x_obj = json_object_get(center, "x");
			json_t *y_obj = json_object_get(center, "y");
			if (!json_is_number(x_obj) || !json_is_number(y_obj))
				continue;

			math::Vec centerPos(
				json_number_value(x_obj),
				json_number_value(y_obj));

			json_t *layer_obj = json_object_get(cloth, "layer");
			float layer;
			if (!layer_obj)
				layer = 0.5;
			else
				layer = json_number_value(layer_obj);

			if (connected && json_is_string(connected))
			{
				dressMeUpDisplay->loadCloth(
					name,
					clothType,
					centerPos,
					layer,
					json_string_value(connected));
			}
			else
			{
				dressMeUpDisplay->loadCloth(
					name,
					clothType,
					centerPos,
					layer);
			}
		}
	}

	json_decref(root);

	dressMeUpDisplay->updateClothing();
	dressMeUpDisplay->sortClothing();

	if (dmud)
	{
		dmud->initializeGL(
			rack::asset::plugin(pluginInstance, "res/shaders/texVert.glsl"),
			rack::asset::plugin(pluginInstance, "res/shaders/texFrag.glsl"),
			rack::asset::plugin(pluginInstance, "res/shaders/filterVert.glsl"),
			rack::asset::plugin(pluginInstance, "res/shaders/filterFrag.glsl"));
	}

	dressMeUpDisplay->enable = true;
	
}

void DressMeUpWidget::appendContextMenu(Menu *menu)
{
	DressMeUp *module = dynamic_cast<DressMeUp *>(this->module);
	assert(module);

	menu->addChild(new MenuSeparator());

	VisualParamsMenuItem *visualParamsMenu;
	visualParamsMenu = createMenuItem<VisualParamsMenuItem>("Shader parameters");
	visualParamsMenu->module = dynamic_cast<DressMeUp *>(module);
	menu->addChild(visualParamsMenu);

	menu->addChild(createCheckMenuItem(
		"Disable Shader", "", [=]()
		{ return module->enableShader; }, [=]()
		{ 
			module->enableShader ^= true; 
			module->drawDisplayDirty = true; }));

	menu->addChild(new MenuSeparator());

	menu->addChild(createCheckMenuItem("Enable Output Filter", "", [=]()
									   { return module->enableOutputFilter; }, [=]()
									   { module->enableOutputFilter ^= 1; }));
}

Menu *VisualParamsMenuItem::createChildMenu()
{
	Menu *menu = new Menu;

	auto &shaderParams = module->shaderParams;

	auto addSlider = [&](const std::string &labelText,
						 float &paramValue,
						 float lo, float hi)
	{
		// Slider
		Slider *slider = new Slider();
		// Hook up our custom Quantity
		slider->quantity = new ShaderParamQuantity(
			module->drawDisplayDirty, paramValue, lo, hi, labelText);
		// Size it to taste
		slider->box.size = Vec(200, 20);
		menu->addChild(slider);
	};

	// Spot Width  (0.5 – 1.5)
	addSlider("Spot Width", shaderParams.spotWidth, 0.5f, 1.5f);

	// Spot Height (0.5 – 1.5)
	addSlider("Spot Height", shaderParams.spotHeight, 0.5f, 1.5f);

	// Color Boost (1.0 – 2.0)
	addSlider("Color Boost", shaderParams.colorBoost, 1.0f, 2.0f);

	// Input Gamma  (let’s say 1.0 – 5.0)
	addSlider("Input Gamma", shaderParams.inputGamma, 1.0f, 5.0f);

	// Output Gamma (1.0 – 5.0)
	addSlider("Output Gamma", shaderParams.outputGamma, 1.0f, 5.0f);

	addSlider("Effect Scale", shaderParams.effectScale, 0.25f, 6.0f);

	return menu;
}

void DressMeUpWidget::drawLayer(const DrawArgs &args, int layer)
{
	ModuleWidget::drawLayer(args, layer);
}


Model *modelDressMeUp = createModel<DressMeUp, DressMeUpWidget>("DressMeUp");
