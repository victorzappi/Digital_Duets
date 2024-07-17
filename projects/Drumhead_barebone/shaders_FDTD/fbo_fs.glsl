#version 450

/*
This code is distributed under the 2-Clause BSD License

Copyright 2017 Victor Zappi
*/

/* fragment shader: FDTD solver running on all the fragments of the texture [grid points]. Also saves audio. Result is rendered to the taxture, via FBO */

in vec2 tex_c;
in vec2 tex_l;
in vec2 tex_u;
in vec2 tex_r;
in vec2 tex_d;

out vec4 frag_color;

//----------------------------------------------------------------------------------------------------------
// quads to draw
const int state0 = 0; // draw right quad 
const int state1 = 1; // read audio from left quad [cos right might not be ready yet]
const int state2 = 2; // draw left quad 
const int state3 = 3; // read audio from right quad [cos left might not be ready yet]

//----------------------------------------------------------------------------------------------------------
uniform sampler2D inOutTexture;
uniform int state;
uniform float excitation;
uniform vec2 wrCoord;   // [x coord of audioWrite pixel, RBGA index]
uniform vec2 listenerFragCoord[4];
uniform float deltaCoordX; // width of each fragment

//----------------------------------------------------------------------------------------------------------
// can modify this simulate different materials and types of boundaries
//----------------------------------------------------------------------------------------------------------
float mu = 0.001; // damping factor, the higher the quicker the damping. generally way below 1
float rho = 0.5;  // propagation factor, that combines spatial scale and speed in the medium. must be <= 0.5
float gamma = 0;  // 0 means clamped boundary [wall], 1 means free boundary 
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------



// FDTD
vec4 computeFDTD() {
	// current point needs current pressure, previous pressure, type [checked later on]
	vec4 frag_color  = texture(inOutTexture, tex_c);
	vec4 p       = frag_color.rrrr; // p_n, this is turned into a vector cos used in parallel neighbor computation
	float p_prev = frag_color.g; // p_n-1
	
	// neighbours [pl_n, pr_n, pu_n, pd_n] need current pressure and type [only b...is boundary?]
	vec4 p_neigh;
	vec4 b_neigh;
	
	// left
	vec4 frag_l = texture(inOutTexture, tex_l);
	p_neigh.r   = frag_l.r;
	b_neigh.r   = frag_l.b;
	
	// up
	vec4 frag_u = texture(inOutTexture, tex_u);
	p_neigh.g   = frag_u.r;
	b_neigh.g   = frag_u.b;
	
	// right
	vec4 frag_r = texture(inOutTexture, tex_r);
	p_neigh.b   = frag_r.r;
	b_neigh.b   = frag_r.b;

	// down
	vec4 frag_d = texture(inOutTexture, tex_d);
	p_neigh.a   = frag_d.r;
	b_neigh.a   = frag_d.b;
	
	// parallel computation of pLRUD
	vec4 pLRUD = p_neigh*b_neigh + p*(1-b_neigh)*gamma; // gamma to simulate clamped or free edge
	
	// assemble equation
	float p_next = 2*p.r + (mu-1) * p_prev;
	p_next += (pLRUD.x+pLRUD.y+pLRUD.z+pLRUD.w - 4*p.r) * rho;
	p_next /= mu+1;
			
	// add excitation if this is excitation point [piece of cake]
	int is_excitation = int(frag_color.a);
	p_next += excitation * is_excitation;
		
	//          p_n+1    p_n  boundary? excitation?
	return vec4(p_next,  p.r, frag_color.b, frag_color.a); // pack and return (: [we maintain channels B and A intact]
}


// writes audio into audio quad from listener, in the correct pixel
vec4 saveAudio() {
	// write location
	float writeXCoord  = wrCoord[0]; // this helps chose what pixel to write audio to
	float writeChannel = wrCoord[1]; // this determines what pixel's channel to write to
		 
	// first of all copy all the 4 values stored in previous step 
	vec4 color = texture(inOutTexture, tex_c);
	
	// then see if this fragment is the one supposed to save the new audio sample...	
	float diffx = tex_c.x-writeXCoord;
	
	// is this the next available fragment?
	if( (diffx  < deltaCoordX) && (diffx >= 0) ) {
		// sample chosen grid point
		int readState = 1-(state/2); // index of the state we are reading from
		vec2 audioCoord = listenerFragCoord[readState]; 
		vec4 audioFrag = texture(inOutTexture, audioCoord); // get the audio info from the listener
		
		// silence boundaries, using b
		float audio = audioFrag.r * audioFrag.b;
		
		// put in the correct channel, according to the audio write command sent from cpu	
		color[int(writeChannel)] = audio;
		//color[int(writeChannel)] = audioFrag.r; //VIC
		//color[3] = 1;
		//color = vec4(1, 2, 3, 4);
	}	 
	return color;
}




void main() {

	if( (state == state0) || (state == state2) ) {	
		// regular FDTD calculation
		frag_color = computeFDTD();
	}
	else {
		// save audio routine
		frag_color = saveAudio();
	}
};
