#include "plugin.hpp"

struct BPMDisplay : widget::Widget
{
	std::shared_ptr<rack::window::Font> font; // Use rack::window::Font

	int bpm = 120; // Default BPM
	int *bpmPtr = nullptr;

	const std::string FONT_PATH = "res/fonts/LCDM2N__.TTF";

	BPMDisplay()
	{
		font = APP->window->loadFont(asset::plugin(
			pluginInstance, FONT_PATH));
	}

	void drawLayer(const DrawArgs &args, int layer) override
	{

		if (!font || !font->handle || layer != 1)
		{
			if (font)
				DEBUG("%d %d\n", (int)font->handle, (int)layer);
			return;
		}

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

		char bpmText[4];
		if (bpmPtr)
		{
			*bpmPtr = std::min(*bpmPtr, 999);
			std::snprintf(bpmText, 4 * sizeof(char), "%03d", (int)*bpmPtr);
		}
		else
		{
			bpm = std::min(bpm, 999);
			std::snprintf(bpmText, 4 * sizeof(char), "%03d", bpm);
		}

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
#pragma GCC diagnostic pop
#endif

		nvgBeginPath(args.vg);

		nvgFontSize(args.vg, 18);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgTextLetterSpacing(args.vg, 3.0); // Adjust spacing to match font

		float x = box.size.x * 0.15; // Shift left to keep it aligned
		float y = box.size.y / 2;

		nvgFillColor(args.vg, NVGcolor{1, 1, 1, 0.25});
		nvgText(args.vg, x, y, "888", NULL);
		nvgText(args.vg, x, y, "111", NULL);

		nvgFillColor(args.vg, NVGcolor{1, 1, 1, 1});
		nvgText(args.vg, x, y, bpmText, NULL);
	}

	void setBPMPtr(int *bpmPtr)
	{
		this->bpmPtr = bpmPtr;
	}
};