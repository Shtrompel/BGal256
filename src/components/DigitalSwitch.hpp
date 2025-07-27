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

#ifndef _DIGITAL_SWITCH
#define _DIGITAL_SWITCH

#include "plugin.hpp"

struct DigitalSwitch : SvgWidget
{
    bool state;
    bool *statePtr = nullptr;
    bool old_state = false;

    std::function<void(bool)> onPress;

    std::string svg_on = "res/components/DigitalSwitchOn.svg";
    std::string svg_off = "res/components/DigitalSwitchOff.svg";

    // Constructor now accepts a pointer to bool
    DigitalSwitch(bool *statePtr)
    {
        this->statePtr = statePtr;
        if (statePtr)
        {
            this->state = *statePtr;
            this->old_state = *statePtr;
        }

        box.size = Vec(10.0, 10.0);

        // Ensure the initial SVG reflects the initial state
        updateSvg();

        onPress = [](bool state) -> void
        {
        };
    }

    template <typename Lambda>
    void setOnPress(Lambda&& lambda)
    {
        this->onPress = std::move(lambda);
    }

    void onButton(const event::Button &e) override
    {
        if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS)
        {
            // Toggle the state and update the SVG
            this->toggleState();

            e.consume(this);

            this->onPress(this->state);
        }
    }

    bool getState() const
    {
        // Dereference the pointer to get the actual state
        return this->state;
    }

    void setState(bool new_state)
    {
        // Assign the new state through the pointer
        this->state = new_state;
        if (this->statePtr)
            *this->statePtr = new_state;

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

    void draw(const DrawArgs &args) override
    {
        // If the state has changed, update the SVG
        if (this->old_state != this->getState())
        {
            this->updateSvg();
            this->old_state = this->getState();
        }

        SvgWidget::draw(args);
    }

};

#endif // _DIGITAL_SWITCH