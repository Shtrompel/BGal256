#include "BufferSludgerTransposer.hpp"

#include "BufferSludger.hpp"


BufferSludgerTransposer::BufferSludgerTransposer() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, LIGHTS_LEN);
	configSwitch(RESET_PARAM, 0.f, 1.f, 0.f, "Reset mode", {"Add", "Reset"});
	configSwitch(REVERSE_PARAM, 0.f, 1.f, 0.f, "Reverse mode", {"Disable", "Enable"});
	configInput(REVERSE_INPUT, "Reset");
	configInput(VOCT_INPUT, "V/Oct");
	configLight(EXTERNAL_CONNECTED_LIGHT, "External Connected");
	configLight(REVERSE_ON_LIGHT, "Reverse Playback");
}

BufferSludgerTransposer::~BufferSludgerTransposer()
{
	deleting.store(true);
}

void setModule()
{

}

void BufferSludgerTransposer::process(const ProcessArgs& args) 
{
	BufferSludger* bufferExpander = nullptr;

	if (isDeleting())
		return;
	
	if (leftExpander.module) {
		bufferExpander = dynamic_cast<BufferSludger*>(
			leftExpander.module);
	}
	if (rightExpander.module && !bufferExpander) {
		bufferExpander = dynamic_cast<BufferSludger*>(
			rightExpander.module);
	}

	lights[EXTERNAL_CONNECTED_LIGHT].setBrightness(
		bufferExpander ? 10 : 0);

	if (!bufferExpander) 
		return;
	
	if (lastPitch != inputs[VOCT_INPUT].getVoltage() && 
		params[RESET_PARAM].getValue() == 1.0f) 
	{
		time = 0.0f;
		lastPitch = inputs[VOCT_INPUT].getVoltage();
	}


	bool isReverse = false;
	if (inputs[REVERSE_INPUT].isConnected())
	{
		isReverse = inputs[REVERSE_INPUT].getVoltage() >= 5.0f;
	}
	else
	{
		isReverse = params[REVERSE_PARAM].getValue() == 1.0f;
	}
	lights[REVERSE_ON_LIGHT].setBrightness(isReverse ? 10 : 0);
	
	float toAdd = 0.0f;
	if (mode == BufferSludger::AUTOMATION_MODE_LINEAR) { // Linear
		toAdd = powf(2, inputs[VOCT_INPUT].getVoltage());
		if (isReverse)
		{
			toAdd *= -1.0f;
		}
	}
	else if (mode == BufferSludger::AUTOMATION_MODE_DERIVATIVE) { // Wrap
		toAdd = powf(2, inputs[VOCT_INPUT].getVoltage() - 1); 
		if (isReverse)
		{
			toAdd *= -1.0f;
		}
		toAdd -= 0.5f;
	}
	toAdd *= 1.0f / args.sampleRate;
	toAdd *= 1.0f / bufferExpander->masterLength;
	toAdd *= 10.0f;


	time += toAdd;
	time = std::fmod (std::fmod (time, 10.0f) + 10.0f, 10.0f);
	
	float out = time;
	out = std::fmod (std::fmod (out, 10.0f) + 10.0f, 10.0f);
    this->outAutomation = out;
}

BufferSludgerTransposerWidget::BufferSludgerTransposerWidget(BufferSludgerTransposer* module) {
	setModule(module);
	setPanel(createPanel(asset::plugin(pluginInstance, "res/BufferSludgerTransposer.svg")));

	addChild(createLightCentered<MediumLight<RedLight>>(
		mm2px(Vec(10, 20)), 
		module, 
		BufferSludgerTransposer::LightId::EXTERNAL_CONNECTED_LIGHT));

	addChild(createLightCentered<MediumLight<RedLight>>(
		mm2px(Vec(5, 40.5)), 
		module, 
		BufferSludgerTransposer::LightId::REVERSE_ON_LIGHT));
	addParam(
        createParamCentered<CKSS>(mm2px(Vec(10.0, 40.0)), 
        module, 
        BufferSludgerTransposer::REVERSE_PARAM));
	addInput(
        createInputCentered<PJ301MPort>(
            mm2px(Vec(10.0, 55.0)), 
            module, 
            BufferSludgerTransposer::REVERSE_INPUT));

	addParam(
        createParamCentered<CKSS>(mm2px(Vec(10.0, 75.0)), 
        module, 
        BufferSludgerTransposer::RESET_PARAM));
    
	addInput(
        createInputCentered<PJ301MPort>(
            mm2px(Vec(10.0, 100.0)), 
            module, 
            BufferSludgerTransposer::VOCT_INPUT));
}

Model* modelBufferSludgerTransposer = createModel<BufferSludgerTransposer, BufferSludgerTransposerWidget>("BufferSludgerTransposer");