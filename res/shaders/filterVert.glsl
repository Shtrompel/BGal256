#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

uniform mat4 projectionMatrix;
out vec2 fragUV;

void main() {
    fragUV = uv;
    gl_Position = projectionMatrix * vec4(position, 0.0, 1.0);
}