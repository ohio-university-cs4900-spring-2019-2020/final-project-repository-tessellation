#version 330 core
in vec3 fPos;
in vec2 fLat;

out vec4 fragColor;

uniform isampler2D elevationTexture;
uniform sampler2D imageryTexture;

const float PI = 3.14159265358979323846;

void main() {
	// The longitude cannot be simply passed as an interpolated variable into this shader
	// stage from the geometry shader due to it breaking down at the poles. However, it
	// can easily be recalculated from the localized ECEF position of this fragment.
	// From: tan(lon) = y / x
	float fLon = atan(fPos.y, fPos.x);
	fLon = (fLon / PI + 1.0) / 2.0; // convert longitude to UV coordinate

	fragColor = vec4(texture(imageryTexture, vec2(fLon, fLat)).rgb, 1.0);
}