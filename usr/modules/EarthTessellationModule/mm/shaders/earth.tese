#version 430 core
layout (quads, fractional_odd_spacing, ccw) in;

in vec2 vTPos[];
out vec2 vTPos2;

void main() {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	vec2 p0 = mix(vTPos[1], vTPos[2], u);
	vec2 p1 = mix(vTPos[0], vTPos[3], u);
	vec2 p = mix(p0, p1, v);

	vTPos2 = p;
}