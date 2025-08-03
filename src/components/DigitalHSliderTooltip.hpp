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

#ifndef _DIGITAL_HSLIDER_TOOLTIP
#define _DIGITAL_HSLIDER_TOOLTIP

#include "plugin.hpp"

#include "VoxglitchTooltip.hpp"

class DigitalHSliderTooltip : public VoxglitchTooltip
{
private:
    float offset_x = -15.0;
    float offset_y = -15.0;

public:

    DigitalHSliderTooltip()
        : VoxglitchTooltip(10.0, -20.0)
    {
    }

    void setAttributes(std::string label, float mouse_x, float mouse_y)
    {
        setLabel(label);

        setX(offset_x + mouse_x);
        setY(offset_y + mouse_y);
    }

    void drawTooltip(const DrawArgs &args)
    {
        VoxglitchTooltip::drawTooltip(args); 
    }
};

#endif // _DIGITAL_HSLIDER_TOOLTIP