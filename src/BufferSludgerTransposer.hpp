#include "plugin.hpp"

struct BufferSludger;

struct BufferSludgerTransposer : Module {
	enum ParamIds {
		REVERSE_PARAM,
		RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		REVERSE_INPUT,
		VOCT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
    enum LightId {
        EXTERNAL_CONNECTED_LIGHT,
        REVERSE_ON_LIGHT,
        LIGHTS_LEN
    };

	std::atomic<bool> deleting{false};

    float time = 0.0f;
    float lastPitch = 0.0f;

    int mode = 0;
    float outAutomation;

	BufferSludgerTransposer();

	~BufferSludgerTransposer();

	void process(const ProcessArgs& args) override;
	
	bool isDeleting() const {
        return deleting.load();
    }
};

// Widget
struct BufferSludgerTransposerWidget : ModuleWidget {

	BufferSludgerTransposerWidget(BufferSludgerTransposer* module);
};