#version 430 core
layout (quads, fractional_odd_spacing, ccw) in;

// Note: As shown above, I'm using fractional_odd_spacing. This produces
//       tessellations in the range [1, 64), which means you can't tessellate
//       as much as fractional_even_spacing which is in the range (1, 64].
//       However, the odd spacing allows no tessellation to be done whereas the
//       even spacing has a minimum tess factor of 2, so some tessellation will
//       always be done.

in vec2 vTPos[];
out vec3 vTLPos;
out vec4 vTPos2;
out float vTLat;

uniform mat4 MVPMat;
uniform float scale;
uniform float maxTessellationFactor;

uniform isampler2D elevationTexture;

// constants used in conversion from WGS84
const float EARTH_RADIUS = 6378137.0;
const float EARTH_FLATTENING = 0.00669437999013;
const float PI = 3.14159265358979323846;

// convert from WGS84 to ECEF
vec3 WGS84ToECEF(vec3 v) {
	float latRad = v.x;
	float lonRad = v.y;
	float elev = v.z * scale;

	float sinLatRad = sin(latRad);
	float e2sinLatSq = EARTH_FLATTENING * (sinLatRad * sinLatRad);

	float rn = EARTH_RADIUS * scale / sqrt(1 - e2sinLatSq);
	float R = (rn + elev) * cos(latRad);

	vec3 o;
	o.x = R * cos(lonRad);
	o.y = R * sin(lonRad);
	o.z = (rn * (1 - EARTH_FLATTENING) + elev) * sin(latRad);

	return o;
}

// bilinear interpolation
float biLerp(float a, float b, float c, float d, float s, float t) {
	float x = mix(a, b, s);
	float y = mix(c, d, s);

	return mix(x, y, t);
}

// sample elevation texture at given UV coordinate and level of detail,
// but perform bilinear interpolation between texels
float biLerpTexture(vec2 uv, int level) {
	ivec2 size = textureSize(elevationTexture, level);

	float x = size.x * uv.x;
	float y = size.y * uv.y;

	int lx = int(floor(x));
	int ux = int(ceil(x));

	int ly = int(floor(y));
	int uy = int(ceil(y));

	float e0 = float(texelFetch(elevationTexture, ivec2(lx, ly), level).r);
	float e1 = float(texelFetch(elevationTexture, ivec2(ux, ly), level).r);
	float e2 = float(texelFetch(elevationTexture, ivec2(lx, uy), level).r);
	float e3 = float(texelFetch(elevationTexture, ivec2(ux, uy), level).r);

	return biLerp(e0, e1, e2, e3, x - lx, y - ly);
}

// super-sample elevation texture at UV coordinate
// note: The mipmap level used by this function is calculated based on
//       the maxTessellationFactor. It interpolates between the upper and
//       lower mipmap levels.
float getElev(vec2 uv) {
	// OpenGL max tessellation factor is 64. log2(64) = 6
	// Thus, 6 - log2(64) = 0, so a maxTessellationFactor of 64 uses
	// the base texture for sampling.
	// A maxTessellationFactor of 16 uses mipmap level 2.
	float level = clamp(6.0 - log2(maxTessellationFactor), 0.0, 6.0);
	int lowLevel = int(floor(level));
	int highLevel = int(ceil(level));
	
	float lowElev = biLerpTexture(uv, lowLevel);
	float highElev = biLerpTexture(uv, highLevel);

	float elev = mix(lowElev, highElev, level - lowLevel);

	return elev * 10.0; // exaggerate elevation by one magnitude of 10
}

// convert WGS84 to UV space of elevation texture
vec2 WGS84ToUV(vec2 v) {
	vec2 uv;
	uv.x = (v.y + PI) / (2 * PI);
	uv.y = (PI / 2 - v.x) / PI;
	return uv;
}

void main() {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	// calculate tessellated interior vertex from tess coord uv
	vec2 p0 = mix(vTPos[1], vTPos[2], u);
	vec2 p1 = mix(vTPos[0], vTPos[3], u);
	vec2 wgs = mix(p0, p1, v);

	float level0 = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], v);
	float level1 = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], u);
	float level = max(level0, level1);
	
	vec2 uv = WGS84ToUV(wgs); // get UV coordinate for vertex
	vTLPos = WGS84ToECEF(vec3(wgs, getElev(uv))); // get ECEF coordinate for vertex
	vTPos2 = MVPMat * vec4(vTLPos, 1.0f); // transform into screen space
	vTLat = uv.y; // send out the lattitude in uv space
}