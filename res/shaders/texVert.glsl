#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
uniform vec2 uPosition;
uniform vec2 uSize;
uniform float uLayer;

uniform mat4 projectionMatrix;
out vec2 fragUV;

void main() {
    fragUV = uv;
    vec2 widgetPos = uPosition + uSize * position;
    gl_Position = projectionMatrix * vec4(widgetPos, uLayer, 1.0);
    gl_Position.z = uLayer;
}