/*
 * This file contains code derived from Voxglitch
 * https://github.com/clone45/voxglitch
 *
 * Original copyright (c) Bret Truchan
 * Licensed under the GNU General Public License v3.0 (GPL-3.0)
 *
 * Modifications by BGAL256
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _DIGITAL_HORIZONTAL_SLIDER
#define _DIGITAL_HORIZONTAL_SLIDER

#include "plugin.hpp"

struct DigitalHorizontalSlider : Widget
{
    float WIDTH = 74.0f;
    float HEIGHT = 10.0f;

    float *value; // Represents the slider's current value as a fraction of total width
    float internalValue = 0.0;

    // Dimensions and positions
    Rect track_rectangle;
    Rect thumb_rectangle;

    // Flag to check if the thumb is being dragged
    bool is_thumb_dragged = false;

    Vec drag_position;

    float fineTuneMultiplier = 0.02f;
    bool ctrlPressedPrev = false;
    bool ctrlPressed = false;

    std::function<void(float)> onRelease;

    DigitalHorizontalSlider(float *value = nullptr)
    {
        this->value = value;
        if (value)
            internalValue = *value;

        // Initialize the track's dimensions and positions based on WIDTH
        track_rectangle.size.x = WIDTH;
        track_rectangle.size.y = HEIGHT;
        track_rectangle.pos = Vec(0, (HEIGHT - track_rectangle.size.y) / 2);

        // Initialize the thumb's dimensions and position
        thumb_rectangle.size.x = 4;
        thumb_rectangle.size.y = HEIGHT;
        thumb_rectangle.pos.x = track_rectangle.pos.x + (internalValue * WIDTH);

        this->box.size.x = WIDTH;
        this->box.size.y = HEIGHT;

        onRelease = [](float value) -> void {};
    }

    void updatePos()
    {
        //thumb_rectangle.pos.x = rand() % 1000;// track_rectangle.pos.x + (*value * WIDTH);
    }

    template<typename Lambda>
    void setOnRelease(Lambda&& lambda)
    {
        this->onRelease = lambda;
    }

    void draw(const DrawArgs &args) override
    {
        thumb_rectangle.pos.x = getValue() * (WIDTH - thumb_rectangle.size.x);

        // Drawing the track
        nvgBeginPath(args.vg);
        nvgRect(args.vg, track_rectangle.pos.x, track_rectangle.pos.y, track_rectangle.size.x, track_rectangle.size.y);
        nvgFillColor(args.vg, nvgRGB(94, 78, 7));
        nvgFill(args.vg);

        // Drawing the thumb
        nvgBeginPath(args.vg);
        nvgRect(args.vg, thumb_rectangle.pos.x, thumb_rectangle.pos.y, thumb_rectangle.size.x, thumb_rectangle.size.y);
        nvgFillColor(args.vg, nvgRGB(255, 215, 20));
        nvgFill(args.vg);
    }

    void onButton(const event::Button &e) override
    {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS)
        {
            e.consume(this);

            drag_position = e.pos;

            if (thumb_rectangle.contains(e.pos))
            {
                is_thumb_dragged = true;
            }
            else if (e.pos.x >= 0 && e.pos.x <= WIDTH)
            {
                float x = e.pos.x;
                x -= thumb_rectangle.size.x / 2;
                x /= WIDTH;
                setValue(x);
                thumb_rectangle.pos.x = x;
                is_thumb_dragged = true;
            }
        }
    }

    float lastValue = -1;

    void onDragMove(const event::DragMove &e) override
    {
        float zoom = getAbsoluteZoom();

        Vec v = e.mouseDelta.div(zoom);
        if (is_thumb_dragged)
        {
            float newValue = v.x;
            newValue /= (WIDTH - thumb_rectangle.size.x);
            if (ctrlPressed)
                newValue *= fineTuneMultiplier;
            newValue += getValue();
            newValue = clamp(
                newValue, 0.f, 1.0f);
            setValue(newValue);
        }

        Widget::onDragMove(e);
    }

    void step() override
    {
        int currentMods = APP->window->getMods();
        bool ctrlPressedNow = (currentMods & GLFW_MOD_CONTROL);

        if (!ctrlPressedPrev && ctrlPressedNow)
        {
            this->ctrlPressed = true;
        }
        else if (ctrlPressedPrev && !ctrlPressedNow)
        {
            this->ctrlPressed = false;
        }

        this->ctrlPressedPrev = this->ctrlPressed;

        Widget::step();
    }

    void setValue(float val)
    {
        internalValue = val;
        if (value)
            *value = val;
    }

    float getValue()
    {
        return internalValue;
    }

    void onDragEnd(const event::DragEnd &e) override
    {
        is_thumb_dragged = false;
        if (value)
        {
            lastValue = internalValue;
            onRelease(internalValue);
        }
    }
};

#endif // _DIGITAL_HORIZONTAL_SLIDER