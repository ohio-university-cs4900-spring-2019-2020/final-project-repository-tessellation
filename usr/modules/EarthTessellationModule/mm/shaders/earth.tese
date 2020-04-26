#version 430 core
layout (quads, fractional_odd_spacing, ccw) in;

// Note: As shown above, I'm using fractional_odd_spacing. This produces
//       tessellations in the range [1, 64), which means you can't tessellate
//       as much as fractional_even_spacing which is in the range (1, 64].
//       However, the odd spacing allows no tessellation to be done whereas the
//       even spacing has a minimum tess factor of 2, so some tessellation will
//       always be done.

in vec2 vTPos[];
out vec2 vTPos2;

void main() {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	// calculate tessellated interior vertex from tess coord uv
	vec2 p0 = mix(vTPos[1], vTPos[2], u);
	vec2 p1 = mix(vTPos[0], vTPos[3], u);
	vec2 p = mix(p0, p1, v);

	vTPos2 = p;
}