#include "plugin.hpp"

#include "SortStep.hpp"

SortStep::SortStep()
{
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	configParam(SCALE_PARAM, 0.1f, 10.f, 1.0f, "Scale");

	configButton(RESET_BUTTON_PARAM, "Reset");
	configButton(SHUFFLE_BUTTON_PARAM, "Shuffle Array");
	configButton(RECALCULATE_BUTTON_PARAM, "Recalculate");

	configInput(STEP_INPUT, "Global Step");
	configInput(RESET_INPUT, "Reset");
	configInput(SHUFFLE_STEP_INPUT, "Shuffle Step");
	configInput(SORT_STEP_INPUT, "Sort Step");
	configInput(TRAVERSE_STEP_INPUT, "Traverse Step");
	configInput(RANDOMIZE_INPUT, "Randomize");
	configInput(RECALCULATE_INPUT, "Recalculate");
	configInput(ALGORITHM_INPUT, "Algorithm Select");
	configInput(ARRAYSIZE_INPUT, "Array Size");

	// configOutput(STEP_OUTPUT, "Step");
	configOutput(CV_OUTPUT, "Index CV");
	configOutput(VALUE_OUTPUT, "Value CV");

	configOutput(END_OUTPUT, "On End Trigger");
	configOutput(SHUFFLE_END_OUTPUT, "On Shuffle End Trigger");
	configOutput(SORT_END_OUTPUT, "On Sort End Trigger");
	configOutput(TRAVERSE_END_OUTPUT, "On Traverse End Trigger");

	triggersMap.insert({STEP_INPUT, dsp::SchmittTrigger{}});
	triggersMap.insert({RESET_INPUT, dsp::SchmittTrigger{}});
	triggersMap.insert({SHUFFLE_STEP_INPUT, dsp::SchmittTrigger{}});
	triggersMap.insert({SORT_STEP_INPUT, dsp::SchmittTrigger{}});
	triggersMap.insert({TRAVERSE_STEP_INPUT, dsp::SchmittTrigger{}});
	triggersMap.insert({RANDOMIZE_INPUT, dsp::SchmittTrigger{}});
	triggersMap.insert({RECALCULATE_INPUT, dsp::SchmittTrigger{}});
}

bool SortStep::checkTrigger(InputId inputId)
{
	return triggersMap
		.at(inputId)
		.process(inputs[inputId].getVoltage());
}

static float mapToScale(
	int value,
	const std::vector<int> &intervals,
	const int offset)
{
	int key = intervals[modTrue(
		value,
		(int)intervals.size())];
	int octave;
	octave = (int)value / (int)intervals.size();

	float x;
	x = key / 12.0f;
	x += octave;
	x += offset / 12.f;
	return x;
}

void SortStep::outputEvent(SorterArrayEvent *event)
{
	int index = 0;
	if (!event)
		return;

	for (SelectedType &element : event->elements)
	{
		auto mapValueOut = [=](int value) -> float
		{
			float out;
			if (sorterArray.enableKeyOutput)
			{
				out = mapToScale(
					value,
					sorterArray.outScale.intervals,
					sorterArray.keyOffset);
			}
			else
			{
				out = value;
				out /= sorterArray.array.size();
				out *= params[ParamId::SCALE_PARAM].getValue();
			}

			out = math::clamp(out, -10.0f, 10.0f);
			return out;
		};

		outputs[CV_OUTPUT].setVoltage(
			mapValueOut(element.index),
			index);

		if (element.index < (int)sorterArray.array.size())
		{
			outputs[VALUE_OUTPUT].setVoltage(
				mapValueOut(sorterArray.array[element.index]),
				index);
		}

		++index;
	}
}

bool loadArrayDataPdArray(std::vector<float> &out, json_t *rootJ)
{
	std::vector<float> buffer;

	// Verify root object exists
	if (!rootJ)
		return false;

	// Check plugin and model
	json_t *pluginJ = json_object_get(rootJ, "plugin");
	json_t *modelJ = json_object_get(rootJ, "model");

	if (!pluginJ || !modelJ)
		return false;
	if (std::string(json_string_value(pluginJ)) != "PdArray")
		return false;
	if (std::string(json_string_value(modelJ)) != "Array")
		return false;

	// Get data section
	json_t *dataJ = json_object_get(rootJ, "data");
	if (!dataJ)
		return false;

	// Extract arrayData
	json_t *arrayDataJ = json_object_get(dataJ, "arrayData");
	if (!arrayDataJ || !json_is_array(arrayDataJ))
		return false;

	// Read array values
	size_t index;
	json_t *value;
	json_array_foreach(arrayDataJ, index, value)
	{
		if (json_is_number(value))
		{
			buffer.push_back(json_number_value(value));
		}
	}

	out = std::move(buffer);
	return true;
}

bool loadArrayDataDBRackModules(std::vector<float> &out, json_t *rootJ, size_t targetSize)
{
	std::vector<float> output(targetSize, 0.f);

	// Validate JSON structure
	if (!rootJ)
		return false;

	json_t *dataJ = json_object_get(rootJ, "data");
	if (!dataJ)
		return false;

	json_t *pxJ = json_object_get(dataJ, "px");
	json_t *pyJ = json_object_get(dataJ, "py");
	if (!json_is_array(pxJ) || !json_is_array(pyJ))
		return false;

	// Extract points
	std::vector<float> px, py;
	size_t index;
	json_t *value;

	json_array_foreach(pxJ, index, value)
	{
		if (json_is_number(value))
			px.push_back(json_number_value(value));
	}

	json_array_foreach(pyJ, index, value)
	{
		if (json_is_number(value))
			py.push_back(json_number_value(value));
	}

	// Validate point data
	if (px.empty() || px.size() != py.size())
		return false;
	if (px.size() == 1)
	{
		std::fill(output.begin(), output.end(), py[0]);
		return false;
	}

	// Normalize and sort points by X coordinate
	std::vector<std::pair<float, float>> points;
	for (size_t i = 0; i < px.size(); i++)
	{
		points.push_back({px[i], py[i]});
	}
	std::sort(points.begin(), points.end());

	// Resample graph to target size
	for (size_t i = 0; i < targetSize; i++)
	{
		const float x = static_cast<float>(i) / (targetSize - 1);

		// Find segment containing x
		auto it = std::lower_bound(points.begin(), points.end(),
								   std::make_pair(x, -std::numeric_limits<float>::infinity()));

		if (it == points.begin())
		{
			output[i] = points.front().second;
		}
		else if (it == points.end())
		{
			output[i] = points.back().second;
		}
		else
		{
			auto prev = it - 1;
			auto next = it;
			const float segmentStart = prev->first;
			const float segmentEnd = next->first;
			const float t = (x - segmentStart) / (segmentEnd - segmentStart);
			output[i] = map(t, 0.0f, 1.0f, prev->second, next->second);
			output[i] = (output[i] + 5) / 10;
		}
	}

	out = std::move(output);
	return true;
}

static float interpolate(float start, float end, float ctrl, float t)
{
	bool mirror = ctrl > 0.5f;
	if (mirror)
	{
		ctrl = 1.0f - ctrl;
		t = 1.0f - t;
	}
	ctrl *= 2.0f;
	float factor = t * pow(ctrl, 2.0f * (1.0f - t));
	if (mirror)
	{
		factor = 1.0f - factor;
	}
	return start + (end - start) * factor;
}

bool loadArrayDataShapeMaster(std::vector<float> &out, json_t *rootJ, size_t targetSize)
{
	std::vector<float> output(targetSize, 0.f);

	if (!rootJ)
		return false;

	json_t *dataJ = json_object_get(rootJ, "data");
	if (!dataJ)
		return false;

	json_t *currChanJ = json_object_get(dataJ, "currChan");
	if (!currChanJ || !json_is_integer(currChanJ))
		return false;
	int currChan = json_integer_value(currChanJ);

	json_t *channelsJ = json_object_get(dataJ, "channels");
	if (!channelsJ || !json_is_array(channelsJ))
		return false;

	size_t numChannels = json_array_size(channelsJ);
	if (currChan < 0 || currChan >= static_cast<int>(numChannels))
		return false;

	json_t *channelJ = json_array_get(channelsJ, currChan);
	if (!channelJ)
		return false;

	json_t *shapeJ = json_object_get(channelJ, "shape");
	if (!shapeJ)
		return false;

	std::vector<float> px, py, ctrl;
	json_t *pointsXJ = json_object_get(shapeJ, "pointsX");
	json_t *pointsYJ = json_object_get(shapeJ, "pointsY");
	json_t *ctrlJ = json_object_get(shapeJ, "ctrl");

	if (pointsXJ && json_is_array(pointsXJ))
	{
		size_t index;
		json_t *value;
		json_array_foreach(pointsXJ, index, value)
		{
			if (json_is_number(value))
				px.push_back(json_number_value(value));
		}
	}

	if (pointsYJ && json_is_array(pointsYJ))
	{
		size_t index;
		json_t *value;
		json_array_foreach(pointsYJ, index, value)
		{
			if (json_is_number(value))
				py.push_back(json_number_value(value));
		}
	}

	if (ctrlJ && json_is_array(ctrlJ))
	{
		size_t index;
		json_t *value;
		json_array_foreach(ctrlJ, index, value)
		{
			if (json_is_number(value))
				ctrl.push_back(json_number_value(value));
		}
	}
	else
	{
		// Default to 0.5 if control values missing
		ctrl.resize(px.size(), 0.5f);
	}

	if (px.size() != py.size() || px.empty())
		return false;
	if (ctrl.size() != px.size())
		ctrl.resize(px.size(), 0.5f);
	if (px.size() == 1)
	{
		std::fill(output.begin(), output.end(), py[0]);
		out = output;
		return true;
	}

	std::vector<std::tuple<float, float, float>> points;
	for (size_t i = 0; i < px.size(); i++)
	{
		points.push_back(std::make_tuple(px[i], py[i], ctrl[i]));
	}
	std::sort(
		points.begin(), points.end(),
		[](
			const std::tuple<float, float, float> &a,
			const std::tuple<float, float, float> &b)
		{
			return std::get<0>(a) < std::get<0>(b);
		});

	for (int i = 0; i < (int)targetSize; ++i)
	{
		float x = (float)i / (targetSize - 1); // go from 0 to 1

		// Find which segment x belongs to
		int seg = 0;
		while (seg < (int)px.size() - 2 && x > px[seg + 1])
		{
			++seg;
		}

		float x0 = px[seg];
		float x1 = px[seg + 1];
		float y0 = py[seg];
		float y1 = py[seg + 1];
		float c = ctrl[seg];

		float t = (x - x0) / (x1 - x0); // local segment t

		float y = interpolate(y0, y1, c, t);
		output[i] = y;
	}

	out = output;
	return true;
}

void apply(json_t *rootJ, SorterArray &sorter, SorterArrayWidget *sorterScreen)
{
	if (!sorter.processingFinished)
		return;

	json_t *pluginJ = json_object_get(rootJ, "plugin");
	json_t *modelJ = json_object_get(rootJ, "model");
	if (!pluginJ || !modelJ)
		return;

	std::string pluginStr = std::string(json_string_value(pluginJ));
	std::string modelStr = std::string(json_string_value(modelJ));

	std::vector<float> arrayData;
	if (pluginStr == "PdArray" && modelStr == "Array")
	{
		if (!loadArrayDataPdArray(arrayData, rootJ))
			return;
	}
	else if (pluginStr == "dbRackModules")
	{
		if (!loadArrayDataDBRackModules(
				arrayData,
				rootJ,
				(int)sorter.arraySize))
			return;
	}
	else if (pluginStr == "MindMeldModular" && modelStr == "ShapeMaster")
	{
		if (!loadArrayDataShapeMaster(
				arrayData,
				rootJ,
				(int)sorter.arraySize))
			return;
	}
	else
	{
		return;
	}

	sorter.reset(arrayData.size());
	std::vector<int> arrayInt;
	arrayInt.resize(arrayData.size());
	for (size_t i = 0; i < arrayData.size(); ++i)
	{
		float x = arrayData[i];
		x = math::clamp(x, 0.0f, 1.0f);
		arrayInt[i] = (int)(x * arrayData.size());
	}

	if (sorterScreen)
		sorterScreen->changeArraySize(arrayInt.size());
	sorter.array = arrayInt;
	sorter.calculate();
}

void SortStep::process(const ProcessArgs &args)
{
	if (expander != leftExpander.module)
	{
		expander = leftExpander.module;
		if (expander)
		{
			json_t *rootJ = expander->toJson();
			apply(rootJ, sorterArray, sorterScreen);
		}
	}

	if (expanderR != rightExpander.module)
	{
		expanderR = rightExpander.module;
		if (expanderR)
		{
			json_t *rootJ = expanderR->toJson();
			apply(rootJ, sorterArray, sorterScreen);
		}
	}

	if (inputs[InputId::ALGORITHM_INPUT].isConnected())
	{
		float x = inputs[InputId::ALGORITHM_INPUT].getVoltage();
		x /= 10.0f;
		x *= SORTER_ARRAY_EVENTS_COUNT;
		x = math::clamp(x, 0.0f, (float)t_algorithmtype::COUNT - 1);
		t_algorithmtype algorithm = (t_algorithmtype)x;
		if (sorterArray.currentType != algorithm)
		{
			if (sorterScreen)
			{
				sorterScreen->changeAlgorithm(algorithm);
			}
			else
			{
				sorterArray.changeAlgorithm(algorithm);
			}
		}
	}

	if (inputs[InputId::ARRAYSIZE_INPUT].isConnected())
	{
		float x = inputs[InputId::ARRAYSIZE_INPUT].getVoltage();
		x /= 10.0f;
		x *= MAX_ARRAY_SIZE;
		int s = (int)x;
		s = math::clamp(s, 1, (int)MAX_ARRAY_SIZE);
		if ((int)sorterArray.array.size() != s)
		{
			if (sorterScreen)
			{
				sorterScreen->changeArraySize(s);
			}
			else
				sorterArray.reset(s, __LINE__, __FILE__);
		}
	}

	auto processInputButton = [this](
								  ParamId paramId,
								  InputId inputId,
								  bool &btnBool) -> bool
	{
		bool out = false;

		if (this->checkTrigger(inputId))
		{
			out = true;
		}

		if (this->params[paramId].getValue())
		{
			if (!btnBool)
			{
				btnBool = true;
				out = true;
			}
		}
		else
		{
			btnBool = false;
		}

		return out;
	};

	if (processInputButton(
			RESET_BUTTON_PARAM,
			RESET_INPUT,
			buttonResetPressed))
	{
		this->sorterArray.reset();
	}

	if (processInputButton(
			SHUFFLE_BUTTON_PARAM,
			RANDOMIZE_INPUT,
			buttonShufflePressed))
	{
		this->sorterArray.shuffle();
	}

	if (processInputButton(
			RECALCULATE_BUTTON_PARAM,
			RECALCULATE_INPUT,
			buttonRecalculatePressed))
	{
		this->sorterArray.calculate();
	}

	outputs[CV_OUTPUT].setChannels(2);
	outputs[VALUE_OUTPUT].setChannels(2);

	if (checkTrigger(STEP_INPUT))
	{
		if (!sorterArray.isDone())
			outputEvent(sorterArray.step());
	}

	if (checkTrigger(SHUFFLE_STEP_INPUT))
	{
		// if (!sorter.isDoneShuffle())
		outputEvent(sorterArray.stepShuffle());
	}

	if (checkTrigger(SORT_STEP_INPUT))
	{
		// if (!sorter.isDoneSort())
		outputEvent(sorterArray.stepSort());
	}

	if (checkTrigger(TRAVERSE_STEP_INPUT))
	{
		// if (!sorter.isDoneTraverse())
		outputEvent(sorterArray.stepTraverse());
	}

	{
		if (sorterArray.triggerDone())
		{
			pulseDone.trigger(1e-3f);
		}
		float valueDone = pulseDone.process(args.sampleTime) ? 10.f : 0.f;
		outputs[END_OUTPUT].setVoltage(valueDone);

		if (sorterArray.triggerShuffle())
		{
			pulseShuffle.trigger(1e-3f);
		}
		float valueShuffle = pulseShuffle.process(args.sampleTime) ? 10.f : 0.f;
		outputs[SHUFFLE_END_OUTPUT].setVoltage(valueShuffle);

		if (sorterArray.triggerSort())
		{
			pulseSort.trigger(1e-3f);
		}
		float valueSort = pulseSort.process(args.sampleTime) ? 10.f : 0.f;
		outputs[SORT_END_OUTPUT].setVoltage(valueSort);

		if (sorterArray.triggerTraverse())
		{
			pulseTraverse.trigger(1e-3f);
		}
		float valueTraverse = pulseTraverse.process(args.sampleTime) ? 10.f : 0.f;
		outputs[TRAVERSE_END_OUTPUT].setVoltage(valueTraverse);
	}
}

void SortStep::onReset(const ResetEvent &e)
{
	Module::onReset(e);
	sorterArray.reset();
}

void SortStep::onRandomize(const RandomizeEvent &e)
{
	Module::onRandomize(e);
	sorterArray.shuffle();
}

json_t *SortStep::dataToJson()
{
	json_t *rootJ = json_object();

	// Save sorter state
	json_object_set_new(rootJ, "sorter", this->sorterArray.toJson());

	// Save other persistent state
	json_object_set_new(rootJ, "recalcOnChange", json_boolean(this->recalcOnChange));
	json_object_set_new(rootJ, "lastAlgorithm", json_integer(this->lastAlgorithm));

	return rootJ;
}

void SortStep::dataFromJson(json_t *rootJ)
{
	DEBUG("g4uwg4uwg4w");
	// Load sorter state
	json_t *sorterJ = json_object_get(rootJ, "sorter");
	if (sorterJ)
		this->sorterArray.fromJson(sorterJ);

	// Load other persistent state
	json_t *j;
	if ((j = json_object_get(rootJ, "recalcOnChange")))
		this->recalcOnChange = json_boolean_value(j);
	if ((j = json_object_get(rootJ, "lastAlgorithm")))
		this->lastAlgorithm = json_integer_value(j);
}

SortStepWidget::SortStepWidget(SortStep *module)
{
	setModule(module);
	setPanel(createPanel(asset::plugin(pluginInstance, "res/SortStep.svg")));

	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	float xBase = 0;
	float yBase = 83.026f;

	float rowTopY = yBase + 10;
	float rowCentreY = yBase + 20;
	float rowBottomY = yBase + 30;

	float rowA = 10, rowB = 32.5, rowC = 55, rowD = 77.5, rowE = 100;

	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowA, rowTopY)), module, SortStep::STEP_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowA, rowBottomY)), module, SortStep::RESET_INPUT));

	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowB, rowTopY)), module, SortStep::SHUFFLE_STEP_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowB, rowBottomY)), module, SortStep::RANDOMIZE_INPUT));

	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowC, rowTopY)), module, SortStep::SORT_STEP_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowC, rowBottomY)), module, SortStep::RECALCULATE_INPUT));

	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowD, rowTopY)), module, SortStep::TRAVERSE_STEP_INPUT));

	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowE, rowTopY)), module, SortStep::ALGORITHM_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xBase + rowE, rowCentreY)), module, SortStep::ARRAYSIZE_INPUT));

	float buttonsOffset = 1.f / 3;

	addParam(createParamCentered<VCVButton>(
		mm2px(Vec(xBase + rowA * (1 - buttonsOffset) + rowB * buttonsOffset, rowBottomY)), module,
		SortStep::RESET_BUTTON_PARAM));
	addParam(createParamCentered<VCVButton>(
		mm2px(Vec(xBase + rowB * (1 - buttonsOffset) + rowC * buttonsOffset, rowBottomY)), module,
		SortStep::SHUFFLE_BUTTON_PARAM));
	addParam(createParamCentered<VCVButton>(
		mm2px(Vec(xBase + rowC * (1 - buttonsOffset) + (rowC + 22.5) * buttonsOffset, rowBottomY)), module,
		SortStep::RECALCULATE_BUTTON_PARAM));

	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xBase + rowA, rowCentreY)), module, SortStep::END_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xBase + rowB, rowCentreY)), module, SortStep::SHUFFLE_END_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xBase + rowC, rowCentreY)), module, SortStep::SORT_END_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xBase + rowD, rowCentreY)), module, SortStep::TRAVERSE_END_OUTPUT));

	// addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xBase + 65, rowBottomY)), module, SortStep::STEP_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xBase + rowD * 0.8 + rowE * 0.2, rowBottomY)), module, SortStep::VALUE_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xBase + rowD * 0.2 + rowE * 0.8, rowBottomY)), module, SortStep::CV_OUTPUT));

	// Add scale knob
	RoundSmallBlackKnob *knob = createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(xBase + rowE + 7.5, rowBottomY)), module, SortStep::SCALE_PARAM);
	addParam(knob);

	// mm2px(Vec(94.528, 84.528))
	addChild(createWidget<Widget>(mm2px(Vec(3.536, 3.536))));

	if (module)
	{
		this->module = module;
		if (!module->sorterArray.isFromJson)
		{
			module->sorterArray.reset(24);
		}

		SorterArray *sorterArray;
		sorterArray = &module->sorterArray;

		SorterArrayWidget *widget = new SorterArrayWidget(
			sorterArray, Vec(0, 0), mm2px(Vec(114.918, 76.525)));
		widget->box.pos = mm2px(Vec(4.110, 6.501));
		addChild(widget);

		// widget->updateUIFromState();
		module->sorterScreen = widget;
	}
	else
	{
		SorterArrayWidget *widget = new SorterArrayWidget(
			nullptr, Vec(0, 0), mm2px(Vec(114.918, 76.525)));
		widget->box.pos = mm2px(Vec(4.110, 6.501));
		addChild(widget);
	}
}

Model *modelSortStep = createModel<SortStep, SortStepWidget>("SortStep");