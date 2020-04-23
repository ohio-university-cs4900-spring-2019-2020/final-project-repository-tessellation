#version 430 core
layout (location = 0) in vec3 VertexPosition;

out vec2 vPos;

void main() {
	//gl_Position = vec4(VertexPosition, 1.0f);
    vPos = VertexPosition.xy;
}