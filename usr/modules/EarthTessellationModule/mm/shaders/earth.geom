#version 430 core
layout (triangles) in;
layout (line_strip, max_vertices = 4) out;

in vec3 vTLPos[];
in vec4 vTPos2[];
in float vTLat[];

out vec3 fPos;
out float fLat;

void main() {
	// emit the vertices of the triangle to form the line
	// strip from v0 -> v1 -> v2 -> v0

	fPos = vTLPos[0];
	fLat = vTLat[0];
	gl_Position = vTPos2[0];
	EmitVertex();

	fPos = vTLPos[1];
	fLat = vTLat[1];
	gl_Position = vTPos2[1];
	EmitVertex();

	fPos = vTLPos[2];
	fLat = vTLat[2];
	gl_Position = vTPos2[2];
	EmitVertex();
	EndPrimitive();

	fPos = vTLPos[0];
	fLat = vTLat[0];
	gl_Position = vTPos2[0];
	EmitVertex();
}