#version 330 core

layout (location = 0) in vec3 pos;

uniform mat4 projectionMatrix;
uniform mat4 worldMatrix;

void main() {
    gl_Position = projectionMatrix * worldMatrix * vec4(pos, 1.0);
}
