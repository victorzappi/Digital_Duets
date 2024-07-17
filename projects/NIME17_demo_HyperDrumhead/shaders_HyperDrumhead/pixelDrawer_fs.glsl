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
		// type includes beta when between wall and air. all the other cases have beta = full air
		color = vec4(0, area, bgain, type); 
		// the zeros on pressure and velocity are masked out when we are not modifying excitation cells!
		// this means pressure and velocity are left the same, so there is less disruption in the simulation when single cells are modified
	}
	// wipe everything out [for excitation quads wipes all 4 channels, eitherwise only the first two are wiped, with an external command]
	else if(operation == operation_reset) {
		color = vec4(0);
	}
}