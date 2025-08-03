#include "BufferSludger.hpp"

#include "BufferSludgerTransposer.hpp"

#include "Utils.hpp"

JwHorizontalSwitch::JwHorizontalSwitch()
{
	addFrame(APP->window->loadSvg(
		asset::plugin(pluginInstance, "res/components/Switch_Horizontal_0.svg")));
	addFrame(APP->window->loadSvg(
		asset::plugin(pluginInstance, "res/components/Switch_Horizontal_1.svg")));
}

BufferSludger::BufferSludger()
{
	config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	configParam(BPM_PARAM, 1.f, 400.0f, 120.0f, "BPM");
	configParam(MIX_PARAM, 0.0f, 1.f, 1.0f, "Dry/Wet Mix");

	configInput(STEP_INPUT, "Clock Step");
	configInput(RESET_INPUT, "Reset Clock");
	configInput(PHASE_INPUT, "Clock Phase");
	configInput(CLEAR_INPUT, "Clear Buffer");
	configInput(AUDIO_INPUT, "CV Audio Input");
	configInput(AUDIO_RIGHT_INPUT, "CV Audio Right Input");
	configInput(AUTOMATION_INPUT, "Playback Automation");
	configInput(MIX_INPUT, "Dry/Wet Mix");

	configOutput(AUDIO_OUTPUT, "CV Audio Output");
	configOutput(AUDIO_RIGHT_OUTPUT, "CV Audio Right Output");

	configSwitch(EXTERNAL_BPM_PARAM, 0.0f, 1.0f, 0.0f, "External BPM", {"Internal", "External"});
	configParam(MODE_PARAM, 0.0, 1.0, 0.0, "Playback Mode");

	reset();
}

void BufferSludger::resizeBuffer(int sampleRate, bool disableSpeed)
{
	// Allow up to 16 changes a second to the sampleRate
	if (lastResizeFrame < sampleRate / 16 && !firstBeat)
	{
		return;
	}

	long sampleCount = static_cast<long>(sampleRate * masterLength);
	if (sampleCount < 1)
	{
		sampleCount = 1;
	}
	
	float speedRatio = 0;
	if (!samples[0].empty())
	{
		speedRatio = static_cast<float>(sampleCount);
		speedRatio /= samples[0].size();
	}

	if (this->enableSpeedChange && speedRatio != 0 && !disableSpeed)
	{
		resizeBufferSpeed(sampleCount, speedRatio);
	}
	else
	{
		for (int i = 0; i < 2; ++i)
		{
			if (samples[i].size() < samples[i].max_size())
				samples[i].resize(static_cast<size_t>(sampleCount));
		}
	}

	// DEBUG("%d %d", (int)samples[0].size(), (int)samples[1].size());

	lastResizeFrame = 0;
}

void BufferSludger::resizeBufferSpeed(
	int sampleRate,
	float speedRatio)
{
	long targetSampleCount = static_cast<long>(sampleRate * masterLength);

	for (int i = 0; i < 2; ++i)
	{
		if (samples[i].empty())
			continue;

		double currentSize = samples[i].size();
		double speedRatio = static_cast<double>(targetSampleCount);
		speedRatio /= currentSize;

		// Skip if speed change is too small to be noticeable (<0.5%)
		if (std::abs(speedRatio - 1.0) < 0.005)
		{
			continue;
		}

		// Resample with linear interpolation (or better algorithm)
		std::vector<float> resampled(targetSampleCount);
		for (long n = 0; n < targetSampleCount; ++n)
		{
			double oldPos = n / speedRatio;
			size_t left = static_cast<size_t>(oldPos);
			size_t right = std::min(left + 1, samples[i].size() - 1);
			double frac = oldPos - left;
			resampled[n] =
				samples[i][left] * (1.0 - frac) +
				samples[i][right] * frac;
		}

		samples[i] = std::move(resampled);
	}
}

void BufferSludger::reset(bool resetFirstBeat)
{
	timeSinceStep = 0.0f;
	if (resetFirstBeat)
		firstBeat = true;
	lastPhaseIn = 0.f;
	recordingIndex = 0;
	automationPhase = 0;
	diffAdd = 0.0;
}

#define COOLAAAAAA()                           \
	{                                          \
		if (inputs[PHASE_INPUT].isConnected()) \
	}

void BufferSludger::process(const ProcessArgs &args)
{

	BufferSludgerTransposer *transposerExtendor = nullptr;
	if (leftExpander.module)
	{
		transposerExtendor = dynamic_cast<BufferSludgerTransposer *>(
			leftExpander.module);
	}
	else if (rightExpander.module && !transposerExtendor)
	{
		transposerExtendor = dynamic_cast<BufferSludgerTransposer *>(
			rightExpander.module);
	}
	else
	{
		transposerExtendor = nullptr;
	}
	automationMode = (int)(params[MODE_PARAM].getValue()) + 1;

	bool externalBpm = params[EXTERNAL_BPM_PARAM].getValue() > 0.f;

	if (lsampleRate != args.sampleRate)
	{
		for (int i = 0; i < 2; ++i)
			outFilter[i].setCoefficients(20000.9, args.sampleRate);
	}
	lsampleRate = args.sampleRate;

	if (inputs[AUDIO_INPUT].isConnected())
	{
		isStereo = inputs[AUDIO_RIGHT_INPUT].isConnected();
	}

	if (clearBufferTrigger.process(inputs[CLEAR_INPUT].getVoltage()))
	{
		for (std::vector<float> &vec : samples)
			std::fill(vec.begin(), vec.end(), 0);
	}

	if (resetTrigger.process(inputs[RESET_INPUT].getVoltage()))
	{
		reset(true);
	}

	if (!externalBpm && params[BPM_PARAM].getValue() != 0)
	{
		this->masterLength = 60 / params[BPM_PARAM].getValue();
		if (this->masterLength != this->lmasterLength)
		{
			resizeBuffer(args.sampleRate);
		}
		if (this->masterLength != 0)
			this->phaseOut += args.sampleTime / this->masterLength;
	}
	else if (
		inputs[STEP_INPUT].isConnected() &&
		this->masterLength != 0)
	{
		if (clockTrigger.process(inputs[STEP_INPUT].getVoltage()))
		{
			if (!firstBeat)
				this->masterLength = timeSinceStep;
			reset();
			resizeBuffer(args.sampleRate);
		}
		firstBeat = false;
		if (this->masterLength != 0)
			this->phaseOut += args.sampleTime / this->masterLength;
	}
	else if (inputs[PHASE_INPUT].isConnected())
	{
		float dif = fabs(lastPhaseIn - inputs[PHASE_INPUT].getVoltage());
		if (dif > 0.5)
		{
			if (!firstBeat)
				this->masterLength = timeSinceStep;
			reset();
			resizeBuffer(args.sampleRate);
		}
		firstBeat = false;
	}
	else
	{
		reset(true);
	}

	bool skipProcessing = false;

	float automationInput = inputs[AUTOMATION_INPUT].getVoltage();
	automationInput += this->externalAutomation;
	automationInput = fmod(fmod(automationInput, 10.0f) + 10.0f, 10.0f);

	if (automationMode == AUTOMATION_MODE_DERIVATIVE)
	{
		automationPhase = recordingIndex;
		automationPhase /= ((float)(samples[0].size()) + GNOME_PLEASING_NUMBER);
		automationPhase += 2 * (automationInput / 10. - 0.5);
		// automationPhase = fmod(fmod(automationPhase, 10.0f) + 10.0f, 10.0f);
	}
	else if (automationMode == AUTOMATION_MODE_LINEAR)
	{
		automationPhase = automationInput / 10.;
	}

	if (automationMode == AUTOMATION_MODE_LINEAR && lastAutomationIn == automationInput)
	{
		// output[0] = output[1] = 0.0;
		// skipProcessing = true;
	}

	float time;
	if (std::isinf(automationPhase) || std::isnan(automationPhase))
		time = 0.0f;
	else
		time = automationPhase * samples[0].size();
 	// For BufferWidget
	this->outputIndex = (size_t)time % samples[0].size();

	// anti clicking filter
	// change "maxSpeed4Filter" with something better
	{
		float indexDif = std::fabs(
			static_cast<long long>(this->lastOutputIndex) -
			static_cast<long long>(this->outputIndex));

		if (antiClickFilter && args.sampleRate != 0 &&
			this->lastOutputIndex != -1 &&
			indexDif > samples[0].size() / args.sampleRate * maxSpeed4Filter)
		{
			fadeGain = 0.0f;
			fadeCounter = (args.sampleRate / 1000) * this->rampSamplesMs;
		}
	}

	for (int i = 0; i < 2; ++i)
	{
		std::vector<float> &vec = samples[i];
		if (!vec.empty() && !skipProcessing)
		{
			if (recordingIndex >= (long long)vec.size())
				recordingIndex = 0;

			int inputId = (i == 0) ? AUDIO_INPUT : AUDIO_RIGHT_INPUT;

			// Record for the buffer
			if (inputs[inputId].isConnected())
				vec[recordingIndex] = inputs[inputId].getVoltage();

			switch (this->interpolationMode)
			{
			case INTERPOLATION_MODE_OPTIMAL_8X:
				output[i] = SampleInterpolation::optimal8X(vec, time);
				break;
			case INTERPOLATION_MODE_OPTIMAL_2X:
				output[i] = SampleInterpolation::optimal2X(vec, time);
				break;
			case INTERPOLATION_MODE_OPTIMAL_32X:
				output[i] = SampleInterpolation::optimal32X(vec, time);
				break;
			case INTERPOLATION_MODE_CUBIC:
				output[i] = SampleInterpolation::cubic(vec, time);
				break;
			case INTERPOLATION_MODE_LINEAR:
				output[i] = SampleInterpolation::linear(vec, time);
				break;
			case INTERPOLATION_MODE_NONE:
				output[i] = SampleInterpolation::none(vec, time);
				break;
			default:
				output[i] = 0;
			}
		}

		if (fadeCounter > 0)
		{
			fadeGain += 1.0f / ((args.sampleRate / 1000) * rampSamplesMs);
			fadeCounter--;
			output[i] = (output[i] * fadeGain);
			output[i] += (lastOutput[i] * (1.0f - fadeGain));
		}
	}

	// Update all Last-X variables
	this->lastPhaseIn = inputs[PHASE_INPUT].getVoltage();
	this->lastAutomationIn = automationInput;
	this->lastOutputIndex = this->outputIndex;
	this->lmasterLength = this->masterLength;

	for (int i = 0; i < 2; ++i)
		this->lastOutput[i] = output[i];

	// Get the dry output value
	float dryOutput[2] = {0, 0};

	for (int i = 0; i < 2; ++i)
	{
		if (samples[i].size())
		{
			float pos =
				recordingIndex / ((float)(samples[i].size()) + GNOME_PLEASING_NUMBER);
			dryOutput[i] = samples[i][(size_t)(pos * samples[i].size()) % samples[0].size()];
		}

		// Mix between dry and wet audio
		float p = params[MIX_PARAM].getValue();
		if (inputs[MIX_INPUT].isConnected())
			p += inputs[MIX_INPUT].getVoltage();

		p = rack::math::clamp(p, 0., 1.);
		output[i] = p * output[i];
		output[i] += (1 - p) * dryOutput[i];
	}

	// Update time/frame
	timeSinceStep += args.sampleTime;
	++lastResizeFrame;
	++recordingIndex;

	if (masterLength != 0)
		bpmValue = 60 / masterLength;

	if (enableOutputFilter)
	{
		outputs[AUDIO_OUTPUT].setVoltage(
			outFilter[0].process(output[0]));
		outputs[AUDIO_RIGHT_OUTPUT].setVoltage(
			outFilter[0].process(output[1]));
	}
	else
	{
		outputs[AUDIO_OUTPUT].setVoltage(output[0]);
		outputs[AUDIO_RIGHT_OUTPUT].setVoltage(output[1]);
	}

	lights[EXTERNAL_BPM_LIGHT].setBrightness(externalBpm ? 10 : 0);
	lights[OUTPUT_LIGHT].setBrightness(output[0] != 0.0 ? 10 : 0);

	externalAutomation = 0.0f;
	if (transposerExtendor)
	{
		externalAutomation = transposerExtendor->outAutomation;
		transposerExtendor->mode = automationMode;
	}
}

void BufferSludger::onReset()
{
	bpmValue = -1;

	masterLength = 0.5;
	timeSinceStep = 0.0;
	phaseOut = 0.0;

	diffAdd = 0;
	automationPhase = 0.0;

	lmasterLength = -1;
	lastPhaseIn = 0.0;
	lastAutomationIn = 0.0;
	lautomationPhase = 0.0;
	lsampleRate = -1;

	antiClickFilter = true;
	fadeGain = 1.0f;
	fadeCounter = 0;
	this->lastOutputIndex = -1;
	lastOutput[0] = lastOutput[1] = 0;
	rampSamplesMs = 15;

	firstBeat = true;

	visualMode = BUFFER_DISPLAY_DRAW_MODE_SAMPLES;
	automationMode = AUTOMATION_MODE_LINEAR;
	interpolationMode = INTERPOLATION_MODE_OPTIMAL_8X;
	enableOutputFilter = true;

	uiDownsampling = 32;

	recordingIndex = 0;
	this->outputIndex = 0;
}

void loadWavToSamples(
	std::string &filepath,
	std::vector<float> &leftChannel,
	std::vector<float> &rightChannel,
	bool &isStereo)
{
	drwav wav;
	if (!drwav_init_file(&wav, filepath.c_str(), nullptr))
	{
		return;
	}

	size_t numFrames = wav.totalPCMFrameCount;
	size_t numSamples = numFrames * wav.channels; // Total number of float samples

	std::vector<float> buffer(numSamples); // Correct allocation
	if (drwav_read_pcm_frames_f32(&wav, numFrames, buffer.data()) == 0)
	{
		drwav_uninit(&wav);
		return;
	}

	isStereo = false;
	if (wav.channels == 1)
	{
		rightChannel = buffer;
		leftChannel = buffer;
	}
	else if (wav.channels == 2)
	{
		isStereo = true;
		leftChannel.resize(numFrames);
		rightChannel.resize(numFrames);
		for (size_t i = 0; i < numFrames; ++i)
		{
			// Left channel
			leftChannel[i] = std::fmin(buffer[i * 2] * 5, 10);
			// Right channel
			rightChannel[i] = std::fmin(buffer[i * 2 + 1] * 5, 10);
		}
	}

	drwav_uninit(&wav);
}

void BufferSludger::loadWavFile()
{
	static const char FILE_FILTERS[] = "Wave (.wav):wav,WAV";

	osdialog_filters *filters = osdialog_filters_parse(FILE_FILTERS);
	DEFER({ osdialog_filters_free(filters); });

	char *pathC = osdialog_file(OSDIALOG_OPEN, NULL, NULL, filters);
	if (!pathC)
	{
		return;
	}

	std::string path(pathC);

	loadWavToSamples(path, samples[0], samples[1], isStereo);

	reset(true);
	this->lastOutputIndex = -1;
	this->masterLength = (float)samples[0].size() / lsampleRate;
	if (this->masterLength > 0.0f)
		params[BPM_PARAM].setValue(60 / this->masterLength);

	resizeBuffer(lsampleRate, true);
}

// Helper to convert binary data to hex
std::string bin2hex(const void *data, size_t len)
{
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');
	const uint8_t *bytes = static_cast<const uint8_t *>(data);
	for (size_t i = 0; i < len; i++)
	{
		oss << std::setw(2) << static_cast<int>(bytes[i]);
	}
	return oss.str();
}

// Helper to convert hex back to binary
void hex2bin(const std::string &hex, void *buf, size_t len)
{
	const char *src = hex.c_str();
	uint8_t *dst = static_cast<uint8_t *>(buf);
	for (size_t i = 0; i < len; i++)
	{
		sscanf(src + 2 * i, "%2hhx", &dst[i]);
	}
}

static const char hex_table[] = "0123456789abcdef";
std::string bin2hex_fast(const void *data, size_t len)
{
	std::string res(len * 2, '\0');
	const uint8_t *bytes = static_cast<const uint8_t *>(data);
	for (size_t i = 0; i < len; i++)
	{
		res[2 * i] = hex_table[bytes[i] >> 4];
		res[2 * i + 1] = hex_table[bytes[i] & 0x0F];
	}
	return res;
}

json_t *BufferSludger::toJson()
{
	json_t *rootJ = Module::toJson();

	// Save float values
	json_object_set_new(rootJ, "masterLength", json_real(masterLength));
	json_object_set_new(rootJ, "lmasterLength", json_real(lmasterLength));
	json_object_set_new(rootJ, "timeSinceStep", json_real(timeSinceStep));
	json_object_set_new(rootJ, "phaseOut", json_real(phaseOut));
	json_object_set_new(rootJ, "lastPhaseIn", json_real(lastPhaseIn));
	json_object_set_new(rootJ, "diffAdd", json_real(diffAdd));
	json_object_set_new(rootJ, "lastAutomationIn", json_real(lastAutomationIn));
	json_object_set_new(rootJ, "automationPhase", json_real(automationPhase));
	json_object_set_new(rootJ, "lautomationPhase", json_real(lautomationPhase));
	json_object_set_new(rootJ, "fadeGain", json_real(fadeGain));
	json_object_set_new(rootJ, "lastOutputL", json_real(lastOutput[0]));
	json_object_set_new(rootJ, "lastOutputR", json_real(lastOutput[1]));
	json_object_set_new(rootJ, "rampSamplesMs", json_real(rampSamplesMs));

	// Save int values
	json_object_set_new(rootJ, "automationMode", json_integer(automationMode));
	json_object_set_new(rootJ, "interpolationMode", json_integer(interpolationMode));
	json_object_set_new(rootJ, "lastResizeFrame", json_integer(lastResizeFrame));
	json_object_set_new(rootJ, "visualMode", json_integer(visualMode));
	json_object_set_new(rootJ, "automationMode", json_integer(automationMode));
	json_object_set_new(rootJ, "fadeCounter", json_integer(fadeCounter));

	// Save size_t values (cast to int since JSON doesn't support size_t)
	json_object_set_new(rootJ, "recordingIndex", json_integer((int)recordingIndex));
	json_object_set_new(rootJ, "outputIndex", json_integer((int)outputIndex));
	json_object_set_new(rootJ, "lastOutputIndex", json_integer(lastOutputIndex));

	// Save boolean values
	json_object_set_new(rootJ, "firstBeat", json_boolean(firstBeat));
	json_object_set_new(rootJ, "enableOutputFilter", json_boolean(enableOutputFilter));
	json_object_set_new(rootJ, "enableSpeedChange", json_boolean(enableSpeedChange));
	json_object_set_new(rootJ, "antiClickFilter", json_boolean(antiClickFilter));
	json_object_set_new(rootJ, "isStereo", json_boolean(isStereo));

	auto saveSamples = [=](std::string title, const std::vector<float> &samples)
	{
		std::string hex = bin2hex_fast(samples.data(), samples.size() * sizeof(float));
		json_object_set_new(rootJ, title.c_str(), json_string(hex.c_str()));
	};

	saveSamples("samples", samples[0]);
	saveSamples("samplesR", samples[1]);

	return rootJ;
}

void BufferSludger::fromJson(json_t *rootJ)
{
	Module::fromJson(rootJ);

	json_t *j;

	// Load float values
	j = json_object_get(rootJ, "masterLength");
	if (j)
		masterLength = json_real_value(j);

	j = json_object_get(rootJ, "lmasterLength");
	if (j)
		lmasterLength = json_real_value(j);

	j = json_object_get(rootJ, "timeSinceStep");
	if (j)
		timeSinceStep = json_real_value(j);

	j = json_object_get(rootJ, "phaseOut");
	if (j)
		phaseOut = json_real_value(j);

	j = json_object_get(rootJ, "lastPhaseIn");
	if (j)
		lastPhaseIn = json_real_value(j);

	j = json_object_get(rootJ, "diffAdd");
	if (j)
		diffAdd = json_real_value(j);

	j = json_object_get(rootJ, "lastAutomationIn");
	if (j)
		lastAutomationIn = json_real_value(j);

	j = json_object_get(rootJ, "automationPhase");
	if (j)
		automationPhase = json_real_value(j);

	j = json_object_get(rootJ, "lautomationPhase");
	if (j)
		lautomationPhase = json_real_value(j);

	j = json_object_get(rootJ, "antiClickFilter");
	if (j)
		antiClickFilter = json_boolean_value(j);

	j = json_object_get(rootJ, "fadeGain");
	if (j)
		fadeGain = json_real_value(j);

	j = json_object_get(rootJ, "fadeCounter");
	if (j)
		fadeCounter = json_integer_value(j);

	j = json_object_get(rootJ, "lastOutputIndex");
	if (j)
		lastOutputIndex = json_integer_value(j);

	j = json_object_get(rootJ, "lastOutputL");
	if (j)
		lastOutput[0] = json_real_value(j);

	j = json_object_get(rootJ, "lastOutputR");
	if (j)
		lastOutput[1] = json_real_value(j);

	j = json_object_get(rootJ, "rampSamplesMs");
	if (j)
		rampSamplesMs = json_real_value(j);

	j = json_object_get(rootJ, "isStereo");
	if (j)
		isStereo = json_boolean_value(j);

	// Load int values
	j = json_object_get(rootJ, "automationMode");
	if (j)
		automationMode = json_integer_value(j);

	j = json_object_get(rootJ, "interpolationMode");
	if (j)
		interpolationMode = (int8_t)json_integer_value(j);

	j = json_object_get(rootJ, "lastResizeFrame");
	if (j)
		lastResizeFrame = json_integer_value(j);

	j = json_object_get(rootJ, "recordingIndex");
	if (j)
		recordingIndex = (size_t)json_integer_value(j);

	j = json_object_get(rootJ, "outputIndex");
	if (j)
		outputIndex = (size_t)json_integer_value(j);

	j = json_object_get(rootJ, "firstBeat");
	if (j)
		firstBeat = json_boolean_value(j);

	j = json_object_get(rootJ, "visualMode");
	if (j)
		visualMode = json_integer_value(j);

	j = json_object_get(rootJ, "automationMode");
	if (j)
		automationMode = json_integer_value(j);

	j = json_object_get(rootJ, "enableOutputFilter");
	if (j)
		enableOutputFilter = json_boolean_value(j);

	j = json_object_get(rootJ, "enableSpeedChange");
	if (j)
		enableSpeedChange = json_boolean_value(j);

	auto loadSamples = [=](json_t *j, const std::string &title, std::vector<float> &samples)
	{
		j = json_object_get(rootJ, title.c_str());
		if (j && json_is_string(j))
		{
			const char *hex = json_string_value(j);
			size_t size = strlen(hex) / (2 * sizeof(float));
			samples.resize(size);
			hex2bin(hex, samples.data(), size * sizeof(float));
		}
	};

	loadSamples(j, "samples", samples[0]);
	loadSamples(j, "samplesR", samples[1]);
}

struct BFInterpolationModeItem : MenuItem
{

	int8_t *ptrInterpolationMode = nullptr;

	Menu *createChildMenu() override
	{
		Menu *menu = new Menu;

		menu->addChild(createCheckMenuItem("None", "", [=]()
										   { return *ptrInterpolationMode == BufferSludger::INTERPOLATION_MODE_NONE; }, [=]()
										   { *ptrInterpolationMode = BufferSludger::INTERPOLATION_MODE_NONE; }));
		menu->addChild(createCheckMenuItem("Linear", "", [=]()
										   { return *ptrInterpolationMode == BufferSludger::INTERPOLATION_MODE_LINEAR; }, [=]()
										   { *ptrInterpolationMode = BufferSludger::INTERPOLATION_MODE_LINEAR; }));
		menu->addChild(createCheckMenuItem("Cubic", "", [=]()
										   { return *ptrInterpolationMode == BufferSludger::INTERPOLATION_MODE_CUBIC; }, [=]()
										   { *ptrInterpolationMode = BufferSludger::INTERPOLATION_MODE_CUBIC; }));
		menu->addChild(createCheckMenuItem("Optimal 2x", "", [=]()
										   { return *ptrInterpolationMode == BufferSludger::INTERPOLATION_MODE_OPTIMAL_2X; }, [=]()
										   { *ptrInterpolationMode = BufferSludger::INTERPOLATION_MODE_OPTIMAL_2X; }));
		menu->addChild(createCheckMenuItem("Optimal 8x", "", [=]()
										   { return *ptrInterpolationMode == BufferSludger::INTERPOLATION_MODE_OPTIMAL_8X; }, [=]()
										   { *ptrInterpolationMode = BufferSludger::INTERPOLATION_MODE_OPTIMAL_8X; }));
		menu->addChild(createCheckMenuItem("Optimal 32x", "", [=]()
										   { return *ptrInterpolationMode == BufferSludger::INTERPOLATION_MODE_OPTIMAL_32X; }, [=]()
										   { *ptrInterpolationMode = BufferSludger::INTERPOLATION_MODE_OPTIMAL_32X; }));

		return menu;
	}
};

struct BFVisualModeItem : MenuItem
{

	int *ptrVisualMode = nullptr;

	Menu *createChildMenu() override
	{
		Menu *menu = new Menu;

		menu->addChild(createCheckMenuItem("Disable", "", [=]()
										   { return *ptrVisualMode == BUFFER_DISPLAY_DRAW_MODE_DISABLE; }, [=]()
										   { *ptrVisualMode = BUFFER_DISPLAY_DRAW_MODE_DISABLE; }));
		menu->addChild(createCheckMenuItem("Samples", "", [=]()
										   { return *ptrVisualMode == BUFFER_DISPLAY_DRAW_MODE_SAMPLES; }, [=]()
										   { *ptrVisualMode = BUFFER_DISPLAY_DRAW_MODE_SAMPLES; }));
		menu->addChild(createCheckMenuItem("Disk", "", [=]()
										   { return *ptrVisualMode == BUFFER_DISPLAY_DRAW_MODE_DISK; }, [=]()
										   { *ptrVisualMode = BUFFER_DISPLAY_DRAW_MODE_DISK; }));

		return menu;
	}
};

struct BFUiDownsamplingQuantity : Quantity
{
	float value = 0;
	int *uiDownsampling = nullptr;

	BFUiDownsamplingQuantity(int *uiDownsampling)
	{
		this->uiDownsampling = uiDownsampling;
		value = *uiDownsampling;
	}

	void setValue(float value) override
	{
		this->value = math::clamp(value, getMinValue(), getMaxValue());
		*uiDownsampling = (int)this->value;
	}

	float getValue() override
	{
		return value;
	}

	float getMinValue() override { return 1; }
	float getMaxValue() override { return 64; }
	float getDefaultValue() override { return 32; }

	float getDisplayValue() override
	{
		return value;
	}

	std::string getDisplayValueString() override
	{
		return string::f("%d", *uiDownsampling);
	}

	void setDisplayValue(float displayValue) override
	{
		setValue(displayValue);
	}

	std::string getLabel() override { return "Ui Downsampling"; }
	std::string getUnit() override { return " samples"; }
};

struct BFUiDownsamplingSlider : ui::Slider
{
	BFUiDownsamplingSlider(int *uiDownsampling)
	{
		quantity = new BFUiDownsamplingQuantity(uiDownsampling);
	}
	~BFUiDownsamplingSlider()
	{
		delete quantity;
	}
};

struct BFUiItem : MenuItem
{
	BufferSludger *module;

	Menu *createChildMenu() override
	{
		Menu *menu = new Menu;

		return menu;
	}
};

struct BFLoadWavItem : MenuItem
{
	BufferSludger *module;
	void onAction(const event::Action &e) override
	{
		if (module)
		{
			module->loadWavFile();
		}
	}
};

BufferSludgerWidget::BufferSludgerWidget(BufferSludger *module)
{
	setModule(module);
	setPanel(createPanel(asset::plugin(pluginInstance, "res/BufferSludger.svg")));

	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
	addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

	addParam(createParamCentered<RoundBlackKnob>(
		mm2px(Vec(4.881 + 10.500 / 2, 11.154 + 10.500 / 2)), module, BufferSludger::BPM_PARAM));
	addParam(createParamCentered<RoundBlackKnob>(
		mm2px(Vec(80.000 + 10.500 / 2, 29.807 + 10.500 / 2)), module, BufferSludger::MIX_PARAM));
	addParam(createParamCentered<JwHorizontalSwitch>(
		mm2px(Vec(70.934 + 5.470 / 2, 15.981 + 3.699 / 2)), module, BufferSludger::MODE_PARAM));

	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.032 + 6.191 / 2, 31.357 + 6.191 / 2)), module, BufferSludger::STEP_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(23.260 + 6.191 / 2, 31.357 + 6.191 / 2)), module, BufferSludger::RESET_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(39.703 + 6.191 / 2, 31.357 + 6.191 / 2)), module, BufferSludger::PHASE_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(44.904 + 6.191 / 2, 47.259 + 6.191 / 2)), module, BufferSludger::CLEAR_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(13.470 + 6.191 / 2, 113.831 + 6.191 / 2)), module, BufferSludger::AUDIO_INPUT));
	addInput(createInputCentered<PJ301MPort>(
		mm2px(Vec(22.690 + 6.191 / 2, 113.831 + 6.191 / 2)), module, BufferSludger::AUDIO_RIGHT_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.989, 116.503)), module, BufferSludger::AUTOMATION_INPUT));
	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.134 + 6.191 / 2, 31.357 + 6.191 / 2)), module, BufferSludger::MIX_INPUT));

	addOutput(createOutputCentered<PJ301MPort>(
		mm2px(Vec(68.134 + 6.191 / 2, 113.831 + 6.191 / 2)), module, BufferSludger::AUDIO_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(
		mm2px(Vec(76.952 + 6.191 / 2, 113.831 + 6.191 / 2)), module, BufferSludger::AUDIO_RIGHT_OUTPUT));

	addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(
		mm2px(Vec(15.772 + 17.057 + 3 + 7, 11.313 + 9.854 / 2)),
		module,
		BufferSludger::EXTERNAL_BPM_PARAM,
		BufferSludger::EXTERNAL_BPM_LIGHT));

	addChild(createLightCentered<MediumLight<RedLight>>(
		mm2px(Vec(65.814 + 19.475 / 2, 111)), module, BufferSludger::OUTPUT_LIGHT));

	BPMDisplay *bpmDisplay = createWidget<BPMDisplay>(
		mm2px(Vec(15.772, 11.313)));
	bpmDisplay->box.size = mm2px(Vec(17.057, 9.854));
	if (module)
		bpmDisplay->setBPMPtr(&module->bpmValue);
	addChild(bpmDisplay);

	if (module)
	{
		BufferDisplayWidget *bufferDisplayWidget =
			createWidget<BufferDisplayWidget>(
				mm2px(Vec(3.120 / 2, 56.723 / 2)));
		bufferDisplayWidget->box.size = mm2px(Vec(89.760, 48.160));
		bufferDisplayWidget->init();

		bufferDisplayWidget->module = module;

		addChild(bufferDisplayWidget);
	}

	// mm2px(Vec(9.609, 4.101))
	// addChild(createWidget<Widget>(mm2px(Vec(66.881, 8.281))));
	// mm2px(Vec(19.218, 8.202))
	// addChild(createWidget<Widget>(mm2px(Vec(16.303, 8.643))));
	// addChild(createWidgetCentered<Widget>(mm2px(Vec(23.017, 23.076))));
}

void BufferSludgerWidget::appendContextMenu(Menu *menu)
{
	BufferSludger *module = dynamic_cast<BufferSludger *>(this->module);
	assert(module);

	menu->addChild(new MenuSeparator());

	BFLoadWavItem *loadWav = new BFLoadWavItem;
	loadWav->text = "Load WAV File";
	loadWav->module = dynamic_cast<BufferSludger *>(module);
	menu->addChild(loadWav);

	menu->addChild(new MenuSeparator());

	BFInterpolationModeItem *intrModeItm = nullptr;
	intrModeItm = createMenuItem<BFInterpolationModeItem>("Interpolation Mode", RIGHT_ARROW);
	intrModeItm->ptrInterpolationMode = &(module->interpolationMode);
	menu->addChild(intrModeItm);

	menu->addChild(createCheckMenuItem("Disable Output Filter", "", [=]()
									   { return !module->enableOutputFilter; }, [=]()
									   { module->enableOutputFilter ^= 1; }));

	menu->addChild(createCheckMenuItem("Disable Anti Clicking Filter", "", [=]()
									   { return !module->antiClickFilter; }, [=]()
									   { module->antiClickFilter ^= 1; }));

	menu->addChild(createCheckMenuItem("Enable speed change on bpm change", "", [=]()
									   { return module->enableSpeedChange; }, [=]()
									   { module->enableSpeedChange ^= 1; }));

	menu->addChild(new MenuSeparator());

	BFVisualModeItem *visualModeItm = nullptr;
	visualModeItm = createMenuItem<BFVisualModeItem>("Visual Mode", RIGHT_ARROW);
	visualModeItm->ptrVisualMode = &(module->visualMode);
	menu->addChild(visualModeItm);

	ui::Slider *uiDownsamplingSlider = nullptr;
	uiDownsamplingSlider = new BFUiDownsamplingSlider(&module->uiDownsampling);
	uiDownsamplingSlider->box.size.x = 200.0f;
	menu->addChild(uiDownsamplingSlider);
}

Model *modelBufferSludger = createModel<BufferSludger, BufferSludgerWidget>("BufferSludger");
