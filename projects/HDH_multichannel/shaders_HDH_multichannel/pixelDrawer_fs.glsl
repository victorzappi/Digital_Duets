#version 420

in vec2 tex_norm;

out vec4 color;
uniform sampler2D drawingTexture;
uniform int operation; // 0 = update, 1 = crossfade update, 2 = reset 

const int oparation_write = 0;
const int operation_reset = 1;

// beta and sigma values
const int cell_boundary   = 0;
const int cell_air        = 1;
const int cell_excitation = 2;
const int cell_dead       = 3;
const int cell_numOFTypes = 4;




void main() {
	// update domain pixels [for mains quads]
	if (operation == oparation_write) {
		float area  = texture(drawingTexture, tex_norm).x;
		float bgain = texture(drawingTexture, tex_norm).y;
		float type  = texture(drawingTexture, tex_norm).z;
		color = vec4(0, area, bgain, type); 
		// the zero on pressure is masked out, by means of a paraemter on the external draw commmand
		// this means pressure is left the same
	}
	// wipe everything out [for excitation quads it wipes all 4 channels, otherwise only the first two are wiped, again by means of an external command]
	else if(operation == operation_reset) {
		color = vec4(0);
	}
}