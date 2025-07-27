#version 330 core

// Based on
// https://docs.libretro.com/shader/crt/#crt-caligari
// https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-caligari.glsl

/*
    Phosphor shader - Copyright (C) 2011 caligari.

    Ported by Hyllian.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

in vec2 fragUV;
out vec4 color;
uniform sampler2D uTexture;

uniform vec2 uResolution; // Rendering buffer size
uniform float uZoom;       // Screen zoom factor
uniform float uEffectScale = 1.0;

// CRT Effect Parameters
uniform float uSpotWidth = 1.2;     // (0.5-1.5, default 0.9)
uniform float uSpotHeight = 0.65;    // (0.5-1.5, default 0.65)
uniform float uColorBoost = 1.45;    // (1.0-2.0, default 1.45)
uniform float uInputGamma = 2.4;     // (default 2.4)
uniform float uOutputGamma = 2.2;    // (default 2.2)

uniform float uRoomBrightness = 1.0;

#define GAMMA_IN(color) pow(color, vec4(uInputGamma))
#define GAMMA_OUT(color) pow(color, vec4(1.0 / uOutputGamma))

#define WEIGHT(w) \
    if(w > 1.0) w = 1.0; \
    w = 1.0 - w * w; \
    w = w * w;

void main() {
    vec2 fixedSize = uResolution / 6.0 / uZoom * uEffectScale;
    vec2 onex = vec2(1.0 / fixedSize.x, 0.0);
    vec2 oney = vec2(0.0, 1.0 / fixedSize.y);
    
    vec2 coords = fragUV * fixedSize;
    vec2 pixel_center = floor(coords) + 0.5;
    vec2 tex_coords = pixel_center / fixedSize;
    
    // Horizontal blending
    float dx = coords.x - pixel_center.x;
    float h_weight_00 = dx / uSpotWidth;
    WEIGHT(h_weight_00);
    
    vec4 col = GAMMA_IN(texture(uTexture, tex_coords)) * h_weight_00;
    
    vec2 coords01;
    if(dx > 0.0) {
        coords01 = onex;
        dx = 1.0 - dx;
    } else {
        coords01 = -onex;
        dx = 1.0 + dx;
    }
    
    float h_weight_01 = dx / uSpotWidth;
    WEIGHT(h_weight_01);
    col += GAMMA_IN(texture(uTexture, tex_coords + coords01)) * h_weight_01;
    
    // Vertical blending
    float dy = coords.y - pixel_center.y;
    float v_weight_00 = dy / uSpotHeight;
    WEIGHT(v_weight_00);
    col *= v_weight_00;
    
    vec2 coords10;
    if(dy > 0.0) {
        coords10 = oney;
        dy = 1.0 - dy;
    } else {
        coords10 = -oney;
        dy = 1.0 + dy;
    }
    
    float v_weight_10 = dy / uSpotHeight;
    WEIGHT(v_weight_10);
    
    // Vertical neighbor contribution
    vec4 colorNB = GAMMA_IN(texture(uTexture, tex_coords + coords10));
    col += colorNB * (v_weight_10 * h_weight_00);
    
    // Diagonal neighbor contribution
    colorNB = GAMMA_IN(texture(uTexture, tex_coords + coords01 + coords10));
    col += colorNB * (v_weight_10 * h_weight_01);
    
    // Apply color boost and gamma correction
    col *= uColorBoost;

    float d = min(
        min(fragUV.x, 1.0 - fragUV.x),
        min(fragUV.y, 1.0 - fragUV.y));
    float vignette = smoothstep(0.0, 0.01, d); // adjust 0.6 for softness
    col.rgb *= vignette;

	// col.rgb /= max(uRoomBrightness, 0.0001);
    
    color = clamp(GAMMA_OUT(col), 0.0, 1.0);
}