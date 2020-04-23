#version 430 core
layout (vertices = 4) out;

in vec2 vPos[];
out vec2 vTPos[];

uniform float tessellationFactor;
uniform float scale;

uniform mat4 MVPMat;

uniform isampler2D elevationTexture;

layout ( binding = 0, std140 ) uniform CameraTransforms
{
   mat4 View;
   mat4 Projection;
   mat4 Shadow; //for shadow mapping
   // A Value of 0 = Render w/ No shadows
   // A Value of 1 = Generate depth map only
   // A Value of 2 = Render w/ Shadow mapping
   int ShadowMapShadingState;
} Cam;

const float EARTH_RADIUS = 6378137.0;
const float EARTH_FLATTENING = 0.00669437999013;
const float PI = 3.14159265358979323846;

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

float getElev(vec2 uv) {
	return float(texture(elevationTexture, uv).r) * 50.0;
}

vec2 WGS84ToUV(vec2 v) {
	vec2 uv;
	uv.x = (v.y + PI) / (2 * PI);
	uv.y = (PI / 2 - v.x) / PI;

	return uv;
}

float tessLevel(vec3 a, vec3 b) {
	float diameter = distance(a, b);
	vec3 center = (a + b) / 2.0;
	vec4 screenPos = MVPMat * vec4(center, 1.0);

	return abs(diameter * Cam.Projection[1][1] / screenPos.w) * tessellationFactor;
}

float clamp_factor(float f) {
	return clamp(f, 1.0, 64.0);
}

void main() {
	vTPos[gl_InvocationID] = vPos[gl_InvocationID];

	if (gl_InvocationID == 0) {
		vec2 uv0 = WGS84ToUV(vPos[0]);
		vec2 uv1 = WGS84ToUV(vPos[1]);
		vec2 uv2 = WGS84ToUV(vPos[2]);
		vec2 uv3 = WGS84ToUV(vPos[3]);

		vec3 v0 = WGS84ToECEF(vec3(vPos[0], getElev(uv0)));
		vec3 v1 = WGS84ToECEF(vec3(vPos[1], getElev(uv1)));
		vec3 v2 = WGS84ToECEF(vec3(vPos[2], getElev(uv2)));
		vec3 v3 = WGS84ToECEF(vec3(vPos[3], getElev(uv3)));

		float e0 = tessLevel(v0, v1);
		float e1 = tessLevel(v1, v2);
		float e2 = tessLevel(v2, v3);
		float e3 = tessLevel(v3, v0);

		gl_TessLevelOuter[0] = clamp_factor(e0);
		gl_TessLevelOuter[1] = clamp_factor(e1);
		gl_TessLevelOuter[2] = clamp_factor(e2);
		gl_TessLevelOuter[3] = clamp_factor(e3);

        gl_TessLevelInner[0] = clamp_factor((e1 + e3) / 2.0);
        gl_TessLevelInner[1] = clamp_factor((e0 + e2) / 2.0);
	}
}