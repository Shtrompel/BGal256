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

#ifndef _DIGITAL_TOGGLE
#define _DIGITAL_TOGGLE

#include "plugin.hpp"

struct DigitalToggle : SvgWidget
{
    bool *state = nullptr;
    bool stateInternal = false;
    bool old_state = false;

    std::string svg_on = "res/arpseq/readout/QuantizeOn.svg";
    std::string svg_off = "res/arpseq/readout/QuantizeOff.svg";

    std::function<void(bool)> onPress;

    DigitalToggle(bool *state)
    {
        if (state)
        {
            this->state = state;
            this->old_state = *state;
            this->stateInternal = *state;
        }

        box.size = Vec(10.0, 10.0);

        // Ensure the initial SVG reflects the initial state
        updateSvg();

        onPress = [](bool state) -> void
        {
        };
    }

    void onButton(const event::Button &e) override
    {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS)
        {
            // Toggle the state and update the SVG
            this->toggleState();

            e.consume(this);

            this->onPress(this->stateInternal);
        }
    }

    template <typename Lambda>
    void setOnPress(Lambda&& lambda)
    {
        this->onPress = std::move(lambda);
    }

    bool getState() const
    {
        return stateInternal;
    }

    void setState(bool new_state)
    {
        // Assign the new state through the pointer
        if (state)
            *state = new_state;
        this->stateInternal = new_state;

        // Update the SVG based on the new state
        updateSvg();
    }

    bool toggleState()
    {
        // Toggle and update the state through the pointer
        this->setState(!this->getState());

        return this->getState();
    }

    void updateSvg()
    {
        // Depending on the state, set the SVG
        if (this->getState())
        {
            setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, svg_on)));
        }
        else
        {
            setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, svg_off)));
        }
    }

    void step() override
    {
        // If the state has changed, update the SVG
        if (old_state != stateInternal)
        {
            updateSvg();
            old_state = stateInternal;
        }

        SvgWidget::step();
    }
};

#endif // _DIGITAL_TOGGLE