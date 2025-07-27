#version 330 core

in vec2 fragUV;
out vec4 color;
uniform vec3 uColor;
uniform sampler2D uTex;

uniform vec2 uTexSize = vec2(1000.0, 1000.0);
uniform bool uEnableGlow = false;

uniform float uIntensity = 12.0;

void main() 
{
    vec2 texUv = vec2(fragUV.y, 1.0 - fragUV.x);
    vec4 texColor = texture(uTex, texUv);
	
	vec3 glow = vec3(0.0);
	
	if (uEnableGlow)
	{
		vec2 texOffset = uIntensity / uTexSize;
		
		float alphaUp       = texture(uTex, texUv + vec2(0.0, texOffset.y)).a;
		float alphaDown     = texture(uTex, texUv - vec2(0.0, texOffset.y)).a;
		float alphaLeft     = texture(uTex, texUv - vec2(texOffset.x, 0.0)).a;
		float alphaRight    = texture(uTex, texUv + vec2(texOffset.x, 0.0)).a;
		float alphaUpLeft   = texture(uTex, texUv + vec2(-texOffset.x, texOffset.y)).a;
		float alphaUpRight  = texture(uTex, texUv + vec2(texOffset.x, texOffset.y)).a;
		float alphaDownLeft = texture(uTex, texUv + vec2(-texOffset.x, -texOffset.y)).a;
		float alphaDownRight= texture(uTex, texUv + vec2(texOffset.x, -texOffset.y)).a;
		
		float maxAlpha = min(min(max(alphaUp, alphaDown), 
								  min(alphaLeft, alphaRight)), 
							 min(min(alphaUpLeft, alphaUpRight), 
								  min(alphaDownLeft, alphaDownRight)));
		
		float glowIntensity = texColor.a - maxAlpha;
		
		glow = uColor * glowIntensity * 1.0; // Adjust multiplier for intensity
	}
    
    color = vec4(texColor.rgb + glow, texColor.a);
}