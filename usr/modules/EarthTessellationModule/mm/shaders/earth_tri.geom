#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec2 vTPos2[];

out vec3 fPos;
out float fLat;

uniform mat4 MVPMat;
uniform float scale;

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

// super-sample elevation texture at UV coordinate
float getElev(vec2 uv) {
	float elev = 0;
	float amntX = 1.0 / 64.0 / 180.0;
	float amntY = amntX / 2.0;

	// take 25 samples and average them
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 5; ++j) {
			vec2 new_uv;
			new_uv.x = uv.x + mix(-amntX, amntX, float(i) / 4.0);
			new_uv.y = uv.y + mix(-amntY, amntY, float(j) / 4.0);

			elev += float(textureLod(elevationTexture, new_uv, 10.0).r);
		}
	}
	elev /= 25.0;

	return elev * 10.0;
}

// convert WGS84 to UV space of elevation texture
vec2 WGS84ToUV(vec2 v) {
	vec2 uv;
	uv.x = (v.y + PI) / (2 * PI);
	uv.y = (PI / 2 - v.x) / PI;
	return uv;
}

void main() {
	// get UV coordinates for each vertex of triangle
	vec2 uv0 = WGS84ToUV(vTPos2[0]);
	vec2 uv1 = WGS84ToUV(vTPos2[1]);
	vec2 uv2 = WGS84ToUV(vTPos2[2]);

	// get ECEF coordinates for each vertex of triangle
	vec3 v0 = WGS84ToECEF(vec3(vTPos2[0], getElev(uv0)));
	vec3 v1 = WGS84ToECEF(vec3(vTPos2[1], getElev(uv1)));
	vec3 v2 = WGS84ToECEF(vec3(vTPos2[2], getElev(uv2)));

	// emit the vertices of the triangle

	fPos = v0;
	fLat = uv0.y;
	gl_Position = MVPMat * vec4(fPos, 1.0);
	EmitVertex();

	fPos = v1;
	fLat = uv1.y;
	gl_Position = MVPMat * vec4(fPos, 1.0);
	EmitVertex();

	fPos = v2;
	fLat = uv2.y;
	gl_Position = MVPMat * vec4(fPos, 1.0);
	EmitVertex();
	EndPrimitive();
}