#version 450

//TODO simplify this whole shader to the minimum!

/*
*/

in vec2 texture0;

in vec2 texture1;
in vec2 texture2;
in vec2 texture3;
in vec2 texture4;


out vec4 color;


const int numOfStates=2;


uniform sampler2D inOutTexture;

uniform float deltaCoordX;
uniform float deltaCoordY;


// types as written by PBO drawer!
const int cell_wall       = 0;
const int cell_air        = 1;
const int cell_excitation = 2;
const int cell_dead       = 3;
const int cell_numOFTypes = 4;

// quads to draw
const int quad_0 = 0;
const int quad_1 = 1;
// 2 drawing quads not used here
const int quad_drawer0 = 2;
const int quad_drawer1 = 3;
// extended audio quads [2 instead of one to pack info about latest state -> check saveAudio()]
const int quad_audio_0 = 4;
const int quad_audio_1 = 5;
uniform int quad;

uniform float mu;  		 // damping factor, the higher the quicker the damping
uniform float rho; 		 // propagation factor, that combines spatial scale and speed in the medium. must be <= 0.5
uniform float boundGain; // 0 means clamped boundary [wall], 1 means free boundary 
uniform float excitationInput;

uniform vec2 listenerFragCoord[4];

uniform vec3 audioWriteCoords;   // (audioWritePixelcoordX, audioWritePixelcoordX, RBGAindex)
// splits as:
// writeXCoord  = audioWriteCoords[0]
// writeYCoord  = audioWriteCoords[1]
// writeChannel = audioWriteCoords[2]

// where:

// writeXCoord[0]  = audioWritePixelcoordX 
// writeYCoord[1]  = audioWritePixelcoordY, these coordinates tell what pixel to write audio to
// writeChannel[2] = RBGAindex, this determines what pixel's channel to write to
//----------------------------------------------------------------------------------------------------------


// shader color dbg: helps check that the type of cell is recognized
// CPU color dbg: helps check we put correctly displaced cell's type on texture

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FDTD
vec4 computeFDTD() {
 	// intepret attributes passed from vertex shader
 	vec2 tex_c = texture0;
 	vec2 tex_l = texture1;
 	vec2 tex_u = texture2; 
 	vec2 tex_r = texture3;
 	vec2 tex_d = texture4; 
	
	// this cell, needs current pressure, previous pressure, type [later on]
	vec4 frag_c  = texture(inOutTexture, tex_c);
	vec4 p       = frag_c.xxxx; // this is turned into a vector cos used in parallel neighbor computation
	float p_prev = frag_c.y;
	
	// neighbours, need current pressure and beta [saved in vec4 to parallelize computation of 4 neighbors]
	vec4 p_neigh;
	vec4 beta_neigh;
	
	// left
	vec4 frag_l  = texture(inOutTexture, tex_l);
	p_neigh.x    = frag_l.x;
	beta_neigh.x = frag_l.z;
	
	// up
	vec4 frag_u  = texture(inOutTexture, tex_u);
	p_neigh.y    = frag_u.x;
	beta_neigh.y = frag_u.z;
	
	// right
	vec4 frag_r  = texture(inOutTexture, tex_r);
	p_neigh.z    = frag_r.x;
	beta_neigh.z = frag_r.z;

	// down
	vec4 frag_d  = texture(inOutTexture, tex_d);
	p_neigh.w    = frag_d.x;
	beta_neigh.w = frag_d.z;
	
	// parallel computation of wall transition
	vec4 p_neighbours = p_neigh*beta_neigh + p*(1-beta_neigh)*boundGain; // crossfade between full propagation and full reflection [not much sense and to keep continous crossfade beta is handled in a redundant way...cos the same info is in alpha challe as type]
	
	// assemble equation
	float p_next = 2*p.x + (mu-1) * p_prev;
	p_next += (p_neighbours.x+p_neighbours.y+p_neighbours.z+p_neighbours.w - 4*p.x) * rho;
	p_next /= mu+1;
		
	// add excitation if this is excitation cell [piece of cake]
	int is_excitation = int(frag_c.a==cell_excitation);
	p_next += excitationInput * is_excitation;
		
	vec4 color = vec4(p_next,  p.x, frag_c.z, frag_c.w); // pack and return (:
	
	return color;
}


// writes audio into audio quad from listener, in the correct pixel
vec4 saveAudio() {
	// write location
	float writeXCoord  = audioWriteCoords[0];
	float writeYCoord  = audioWriteCoords[1];
	float writeChannel = audioWriteCoords[2];
		 
	// first of all copy all the 4 values stored in previous step 
	vec4 retColor = texture(inOutTexture, texture0);
	
	// then see if this fragment is the one suppposed to save the new audio sample...	
	float diffx = texture0.x-writeXCoord;
	float diffy = texture0.y-writeYCoord;
	
	if( (diffx  < deltaCoordX) && (diffx >= 0) &&
	    (diffy  < deltaCoordY) && (diffy >= 0) ) {
	
		int readState = quad-quad_audio_0; // index of the state we are reading from

		vec2 audioCoord = listenerFragCoord[readState];
		
		// get the audio info from the listener 
		vec4 audioFrag = texture(inOutTexture, audioCoord);
		float audio = audioFrag.r * audioFrag.b; // use beta to silence walls
		
		// put in the correct channel, according to the audio write command sent from cpu	
		retColor[int(writeChannel)] = audio;
		//retColor[int(writeChannel)] = audioFrag.r; //VIC
	}
	return retColor;
}




void main () {
	if(quad <= quad_1) {	
		// regular FDTD calculation
		color = computeFDTD();
	}
	else /*if(quad >= quad_audio_0 && quad <= quad_audio_1)*/ {
		// save audio routine
		color = saveAudio();
	}
};