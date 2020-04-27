#version 430 core
layout (location = 0) in vec3 VertexPosition;

out vec2 vPos;

void main() {
    vPos = VertexPosition.xy; // just forward the x and y
}