#version 420

in vec2 tex_norm;

out vec4 color;

uniform usampler2D drawingTexture;
uniform int resetState;

void main() {
	// update domain pixels [for mains quads]
	if (resetState == 0) {
		int c = int(texture(drawingTexture, tex_norm).x);
		color = vec4(0, 0, 0, float(c));
	}
	// wipe everything out  [for excitation quads]
	else if(resetState == 1) {
		color = vec4(0);
	}
}