#version 420

in vec2 tex_norm;

out vec4 color;
uniform sampler2D drawingTextureA;
uniform sampler2D drawingTextureB;
uniform int operation; // 0 = update, 1 = crossfade update, 2 = reset 

uniform float crossfade; 
uniform int deck; // which deck we are using or which one we are transitioning to

const int cell_air        = 1;
const int cell_excitation = 2;

const int oparation_oneShot      = 0;
const int oparation_crossfade    = 1;
const int oparation_formerExcite = 2;
const int operation_reset        = 3;


void main() {
	// update domain pixels [for mains quads]
	if (operation == oparation_oneShot) {
		float alpha = (1-deck) * texture(drawingTextureA, tex_norm).x + deck * texture(drawingTextureB, tex_norm).x;
		color = vec4(0, 0, 0, alpha); 
		// the zeros on pressure and velocity are masked out when we are not modifying excitation cells!
		// this means pressure and velocity are left the same, so there is less disruption in the simulation when single cells are modified
	}
	//TODO consider moving this to main FBO shader, simply using mix() between the 2 textures...
	// crossfade for smooth transition between the 2 domains saved in the 2 textures [ignores cells that turn from exciatations to something else, i.e., former excitations]
	else if(operation == oparation_crossfade) {
		float alphaA = texture(drawingTextureA, tex_norm).x;
		float alphaB = texture(drawingTextureB, tex_norm).x;
		
		// if we are at last iteration [in either directions], we updated all but former excitations
		if( crossfade==0 && !(alphaA == cell_air && alphaB == cell_excitation) )
			color = vec4(0, 0, 0, alphaA);
		else if( crossfade==1  && !(alphaA == cell_excitation && alphaB == cell_air) ) //else if(crossfade==1  && alphaB != cell_air)
			color = vec4(0, 0, 0, alphaB);
		// otherwise we update everything but excitations
		else if( (alphaA != cell_excitation) && (alphaB != cell_excitation) ) {
			float alpha = (1-crossfade)*alphaA + crossfade*alphaB ;
			color = vec4(0, 0, 0, alpha);
		}
		// indeed excitations are discarded
		else
			discard; // do not change excitation cells

	}
	// update former excitation cells [that turn from excitations to something else]
	else if(operation == oparation_formerExcite) {
		float alphaA = texture(drawingTextureA, tex_norm).x;
		float alphaB = texture(drawingTextureB, tex_norm).x;
		// check in the 2 directions
		if( (crossfade==0) && (alphaA == cell_air) && (alphaB == cell_excitation) )
			color = vec4(0, 0, 0, alphaA);
		else if( (crossfade==1)  && (alphaB == cell_air) && (alphaA == cell_excitation) )
			color = vec4(0, 0, 0, alphaB);
		else
			discard;
	}
	// wipe everything out [for excitation quads]
	else if(operation == operation_reset) {
		color = vec4(0);
	}
}