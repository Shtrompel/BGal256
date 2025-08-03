#include "BufferWidget.hpp"

#include "../BufferSludger.hpp"

BufferDisplayWidget::BufferDisplayWidget()
{
    this->font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LCDM2N__.TTF"));
}

void BufferDisplayWidget::init()
{
}

void BufferDisplayWidget::drawLayer(const DrawArgs &args, int layer)
{
    if (layer != 1)
        return;
    drawScene(args);
}

void BufferDisplayWidget::draw(const DrawArgs &args)
{
    Widget::draw(args);
}

void BufferDisplayWidget::drawScene(const DrawArgs &args)
{
    auto &vg = args.vg;

    NVGcolor bgColor = nvgRGBA(0x38, 0x55, 0x74, 0xFF);
    nvgBeginPath(vg);
    nvgRect(vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
    nvgFillColor(vg, bgColor);
    nvgFill(vg);

    if (this->module)
    {
        if (this->module->visualMode == BUFFER_DISPLAY_DRAW_MODE_SAMPLES)
        {
            if (this->module->isStereo)
            {
                auto boxA = getBox();
                boxA.size.y = boxA.getHeight() / 2;

                auto boxB = boxA;
                boxB.pos.y += boxB.size.y;
                drawSamples(args, boxA, this->module->samples[0]);
                drawSamples(args, boxB, this->module->samples[1]);
            }
            else
            {
                drawSamples(args, getBox(), this->module->samples[0]);
            }
        }
        else if (this->module->visualMode == BUFFER_DISPLAY_DRAW_MODE_DISK)
            drawDisk(args, getBox(), this->module->samples[0]);
    }

    // UI Text
    {
        nvgBeginPath(vg);
        char infoText[64];

        // Time in seconds
        float duration = 0;
        size_t sampleCount = 0;
        if (this->module)
        {
            duration = (float)this->module->samples[0].size() / 44100;
            sampleCount = this->module->samples[0].size();
        }

        snprintf(
            infoText,
            sizeof(infoText),
            "samples: %d (%.3f sec)",
            (int)sampleCount, duration);

        nvgFontSize(vg, 12.0f);
        nvgFontFaceId(vg, font->handle);
        nvgFillColor(vg, nvgRGB(255, 255, 255)); // White text
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgText(
            vg,
            getBox().getLeft() + 5,
            getBox().getTop() + 5,
            infoText,
            nullptr);
    }
}

void BufferDisplayWidget::drawDisk(const DrawArgs &args, Rect box, const std::vector<float> &samples)
{
    float radius = std::min(box.size.x, box.size.y) / 2.0f;

    float outerRadius = radius;
    float innerRadius = radius * 0.3f;

    auto getCircleCentered = [=](float angle, float radius)
    {
        float x = cosf(angle);
        x *= radius;
        x += box.getLeft() + getBox().getWidth() / 2;

        float y = sinf(angle);
        y *= radius;
        y += box.getTop() + getBox().getHeight() / 2;
        return Vec(
            x, y);
    };

    // Set the center of the disk
    Vec center = box.getCenter();

    // Save the current NanoVG state
    nvgSave(args.vg);

    // Apply rotation
    float angle = 0.0f;
    if (this->module)
        angle = this->module->outputIndex % samples.size();
    angle = -(angle / samples.size()) * 2 * M_PI;

    nvgTranslate(args.vg, center.x, center.y);   // Move to the center
    nvgRotate(args.vg, angle);                   // Rotate by the specified angle
    nvgTranslate(args.vg, -center.x, -center.y); // Move back to the original position

    // Number of segments for the gradient
    int segments = 18; // More segments = smoother gradient
    float angleStep = 2 * M_PI / segments;

    // Define gradient colors
    NVGcolor startColor = nvgRGB(255, 0, 0); // Red
    NVGcolor endColor = nvgRGB(0, 0, 255);   // Blue

    // Draw each segment
    for (int i = 0; i < segments; i++)
    {
        // Calculate the start and end angles for the current segment
        float startAngle = i * angleStep;
        float endAngle = (i + 1) * angleStep;

        // Interpolate the color for the current segment
        float t = (float)i / segments;
        NVGcolor segmentColor = nvgLerpRGBA(startColor, endColor, t);

        // Begin drawing the path for the segment
        nvgBeginPath(args.vg);
        nvgArc(args.vg, center.x, center.y, outerRadius, startAngle, endAngle, NVG_CW);
        nvgArc(args.vg, center.x, center.y, innerRadius, endAngle, startAngle, NVG_CCW);
        nvgClosePath(args.vg);
        nvgFillColor(args.vg, segmentColor);
        nvgFill(args.vg);
    }

    // Draw waveform
    nvgBeginPath(args.vg);

    float ir2 = innerRadius * 1.05;
    float or2 = outerRadius * 0.95;
    for (size_t i = 0; i < samples.size(); i += this->module->uiDownsampling)
    {

        float sampleAngle = (float)i / samples.size() * 2 * M_PI;
        float val = (samples.at(i) + 5) / 10.0f;

        Vec pos = getCircleCentered(sampleAngle, val * (or2 - ir2) + ir2);

        if (i == 0)
        {
            nvgMoveTo(args.vg, pos.x, pos.y);
        }
        else
        {
            nvgLineTo(args.vg, pos.x, pos.y);
        }
    }
    nvgStrokeColor(args.vg, nvgRGB(0, 0, 0));
    nvgStrokeWidth(args.vg, 0.5f);
    nvgStroke(args.vg);

    {
        float angle = this->module->recordingIndex % samples.size();
        angle = (angle / samples.size()) * 2 * M_PI;

        Vec a = getCircleCentered(angle, innerRadius);
        Vec b = getCircleCentered(angle, outerRadius);

        nvgBeginPath(args.vg);
        nvgMoveTo(
            args.vg,
            a.x, a.y);
        nvgLineTo(
            args.vg,
            b.x, b.y);
        nvgStrokeColor(args.vg, nvgRGB(0, 0, 0));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);
    }

    // Restore the NanoVG state (undo rotation and translation)
    nvgRestore(args.vg);

    {
        nvgBeginPath(args.vg);
        nvgMoveTo(
            args.vg,
            box.getLeft() + getBox().getWidth() / 2 + innerRadius,
            box.getTop() + getBox().getHeight() / 2);
        nvgLineTo(
            args.vg,
            box.getLeft() + getBox().getWidth() / 2 + outerRadius,
            box.getTop() + getBox().getHeight() / 2);
        nvgStrokeColor(args.vg, nvgRGB(1, 0, 0));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgStroke(args.vg);
    }
}

void BufferDisplayWidget::drawSamples(const DrawArgs &args, Rect box, const std::vector<float> &samples)
{
    if (samples.empty())
        return;

    const float PADDING = 0.1;

    nvgBeginPath(args.vg);
    for (size_t i = 0; i < samples.size(); i += this->module->uiDownsampling)
    {
        float x = (float)i / samples.size();
        x *= box.getWidth();
        x += box.getLeft();

        float y = (1.0f - (samples.at(i) + 5) / 10.0f);
        y = rack::math::clamp(y, 0.0f, 1.0f);
        y += PADDING / 2.f;
        y = box.getTop() + (1.f - PADDING) * box.getHeight() * y;

        if (i == 0)
        {
            nvgMoveTo(args.vg, x, y);
        }
        else
        {
            nvgLineTo(args.vg, x, y);
        }
    }
    nvgStrokeColor(args.vg, nvgRGB(0, 0, 0));
    nvgStrokeWidth(args.vg, 1.5f);
    nvgStroke(args.vg);

    // Red - Recording Index
    drawBar(
        args,
        samples.size(),
        this->module->recordingIndex % samples.size(),
        nvgRGBA(255, 0, 0, 255),
        box);

    // Yellow - Output Index
    drawBarFlt(
        args,
        this->module->automationPhase,
        nvgRGBA(255, 255, 0, 255),
        box);
}

void BufferDisplayWidget::drawBar(
    const DrawArgs &args,
    size_t sampleCount,
    size_t index,
    const NVGcolor color,
    const Rect &rect)
{
    auto &vg = args.vg;

    if (index < sampleCount)
    {
        float barX = (float)index / sampleCount;
        barX *= rect.getWidth();
        barX += rect.getLeft();

        nvgBeginPath(vg);
        nvgMoveTo(vg, barX, rect.getTop());
        nvgLineTo(vg, barX, rect.getTop() + rect.getHeight());
        nvgStrokeColor(vg, color); // Red bar
        nvgStrokeWidth(vg, 2.0f);
        nvgStroke(vg);
    }
}

void BufferDisplayWidget::drawBarFlt(
    const DrawArgs &args,
    float phase,
    const NVGcolor color,
    const Rect &rect)
{
    auto &vg = args.vg;

    //if (index < sampleCount)
    {
        float barX =  fmod(fmod(phase, 1.0f) + 1.0f, 1.0f);
        barX *= rect.getWidth();
        barX += rect.getLeft();

        nvgBeginPath(vg);
        nvgMoveTo(vg, barX, rect.getTop());
        nvgLineTo(vg, barX, rect.getTop() + rect.getHeight());
        nvgStrokeColor(vg, color); // Red bar
        nvgStrokeWidth(vg, 2.0f);
        nvgStroke(vg);
    }
}