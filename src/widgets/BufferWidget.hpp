#pragma once
#include "plugin.hpp"

#include <vector>

struct BufferSludger;

constexpr int BUFFER_DISPLAY_DRAW_MODE_DISABLE = 1;
constexpr int BUFFER_DISPLAY_DRAW_MODE_SAMPLES = 2;
constexpr int BUFFER_DISPLAY_DRAW_MODE_DISK = 3;

struct BufferDisplayWidget : Widget  {

    BufferSludger* module = nullptr;
    std::shared_ptr<rack::window::Font> font; 
    int downSample = 32;

    BufferDisplayWidget();

    void init();

    void drawLayer(const DrawArgs& args, int layer) override;

    void draw(const DrawArgs& args) override;

    void drawScene(const DrawArgs& args);

    void drawSamples(const DrawArgs& args, Rect box, const std::vector<float>& samples);

    void drawDisk(const DrawArgs& args, Rect box, const std::vector<float>& samples);

    void drawBar(
        const DrawArgs& args, 
        size_t sampleCount, 
        size_t index, 
        const NVGcolor color,
        const Rect& rect);

    void drawBarFlt(
        const DrawArgs& args, 
        float phase, 
        const NVGcolor color,
        const Rect& rect);

};