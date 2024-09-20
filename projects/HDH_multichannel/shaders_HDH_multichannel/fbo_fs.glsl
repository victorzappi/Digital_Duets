#version 450

in vec2 texture0;
in vec2 texture1;
in vec2 texture2;
in vec2 texture3;
in vec2 texture4;
in vec2 texture5;


out vec4 color;

// this must be the same as the respective value defined in HyperDrumhead class or the parent class
const int numOfStates   = 2;

const int maxNumOfAreas = 8;
uniform int numOfAreas;

const int maxNumOfInChannels = 16;


const int maxNumOfOutChannels = 16;
uniform int numOfOutChannels;
uniform float audioWriteChannelOffset;


uniform sampler2D inOutTexture;

uniform float deltaCoordX;
uniform float deltaCoordY;


// types as defined in HyperDrumhead class
const int cell_boundary    = 0;
const int cell_air         = 1;
const int cell_excitation  = 2;
const int cell_excitationA = 3;
const int cell_excitationB = 4;
const int cell_dead        = 5;
const int cell_numOFTypes  = 6;

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

uniform float areaMu[maxNumOfAreas];  		 // damping factor, the higher the quicker the damping
uniform float areaRho[maxNumOfAreas]; 		 // propagation factor, that combines spatial scale and speed in the medium. must be <= 0.5
uniform float excitationInput; // excites all areas
uniform float areaExciteInput[maxNumOfInChannels]; // excite only specific area

// listeners
uniform vec2 listenerFragCoord[numOfStates];
uniform vec2 areaListenerFragCoord[maxNumOfAreas][maxNumOfOutChannels][numOfStates];

uniform vec2 areaListenerBFragCoord[maxNumOfAreas][maxNumOfOutChannels][numOfStates];

uniform float crossfade[maxNumOfAreas][maxNumOfOutChannels];  

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

uniform float exciteCrossfade[maxNumOfInChannels];


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
 	vec2 tex_c_prev = texture5; 
 		
	// this cell, needs current pressure, previous pressure, then area and type [later on]
	vec4 frag_c  = texture(inOutTexture, tex_c);
	vec4 p       = frag_c.xxxx; // this is turned into a vector cos used in parallel neighbor computation
	vec4 frag_c_prev = texture(inOutTexture, tex_c_prev);
	float p_prev     = frag_c_prev.x;
	
	int area = int(frag_c.y); // to access local mu, rho and excitation
	
	// neighbours, need current pressure and beta [saved in vec4 to parallelize computation of 4 neighbors]
	vec4 p_neigh;
	vec4 beta_neigh;  // 0 means boundary, 1 means air
	vec4 gamma_neigh; // 0 means clamped boundary [wall], 1 means free boundary 
	
	// left
	vec4 frag_l  = texture(inOutTexture, tex_l);
	p_neigh.x    = frag_l.x;
	beta_neigh.x = float( (frag_l.w==cell_air)||(frag_l.w==cell_excitation)||(frag_l.w==cell_excitationA)||(frag_l.w==cell_excitationB) );
	gamma_neigh.x = frag_l.z;
	
	// up
	vec4 frag_u  = texture(inOutTexture, tex_u);
	p_neigh.y    = frag_u.x;
	beta_neigh.y = float( (frag_u.w==cell_air)||(frag_u.w==cell_excitation)||(frag_u.w==cell_excitationA)||(frag_u.w==cell_excitationB)  );
	gamma_neigh.y = frag_u.z;
	
	// right
	vec4 frag_r  = texture(inOutTexture, tex_r);
	p_neigh.z    = frag_r.x;
	beta_neigh.z = float( (frag_r.w==cell_air)||(frag_r.w==cell_excitation)||(frag_r.w==cell_excitationA)||(frag_r.w==cell_excitationB)  );
	gamma_neigh.z = frag_r.z;

	// down
	vec4 frag_d  = texture(inOutTexture, tex_d);
	p_neigh.w    = frag_d.x;
	beta_neigh.w = float( (frag_d.w==cell_air)||(frag_d.w==cell_excitation)||(frag_d.w==cell_excitationA)||(frag_d.w==cell_excitationB)  );
	gamma_neigh.w = frag_d.z; 
	
	// parallel computation of wall transition
	vec4 p_neighbours = p_neigh*(beta_neigh) + (1-beta_neigh)*p*(1-gamma_neigh); // air or boundary [in turn clamped or free] 
	
	// assemble equation
	float p_next = 2*p.x + (areaMu[area]-1) * p_prev;
	p_next += (p_neighbours.x+p_neighbours.y+p_neighbours.z+p_neighbours.w - 4*p.x) * areaRho[area];
	p_next /= areaMu[area]+1;
		
	// add excitation if this is an excitation cell
	int is_excitation = int(frag_c.w==cell_excitation);
	int is_excitationA = int(frag_c.w==cell_excitationA);
	int is_excitationB = int(frag_c.w==cell_excitationB); 
	int channel = int(frag_c.z);
	// type excitation is alwahys triggered and takes into account main excitation command  +  crossfade between excitationA and excitationB 
	p_next += (excitationInput+areaExciteInput[channel])*is_excitation + areaExciteInput[channel]*is_excitationA*(1-exciteCrossfade[channel]) + exciteCrossfade[channel]*areaExciteInput[channel]*is_excitationB;
	//p_next += (excitationInput+areaExciteInput[area]) * (is_excitation + is_excitationA + is_excitationB);
	
		
	//             (  p_next, area,     gamma [clamped or free], beta [boundary, air, excitation, dead]  ) -> area serves to access local mu, rho and excitation
	vec4 color = vec4(p_next, frag_c.y, frag_c.z, frag_c.w); // pack and return (:
	
	return color;
}


float sampleAudio(vec2 audioCoord) {	
	// get the audio info from the listener 
	vec4 audioFrag = texture(inOutTexture, audioCoord);
	 // silence boundaries, dead cells and fragments outside of texture [hidden listeners]
	return audioFrag.r * int(audioFrag.a==cell_air || audioFrag.a==cell_excitation || audioFrag.a==cell_excitationA || audioFrag.a==cell_excitationB) * int(audioFrag.x>-1); 
}

// writes audio into audio quad from listener, in the correct pixel
vec4 saveAudio() {
	// write location
	float writeXCoord  = audioWriteCoords[0];
	float writeYCoord  = audioWriteCoords[1];
	float writeChannel = audioWriteCoords[2];
		
	// first of all copy all the 4 values stored in previous step 
	vec4 retColor = texture(inOutTexture, texture0);
	
	// then cycle the audio regions of all channels, which are stacked vertically
	for(int k=0; k<numOfOutChannels; k++) {
		
		// and see if this fragment is the one suppposed to save the new audio sample in this channel...	
		float diffx = texture0.x-writeXCoord;
		float diffy = texture0.y-writeYCoord;
		
		if( (diffx  < deltaCoordX) && (diffx >= 0) &&
			(diffy  < deltaCoordY) && (diffy >= 0) ) {
		
			int readState = quad-quad_audio_0; // index of the state we are reading from
			int channel = int(writeChannel);
			retColor[channel] = 0;   // start with silence
			
			// primary listener [generally silenced, out of viewport]
			//retColor[channel] += sampleAudio(listenerFragCoord[readState]);
			
			float audioA = 0;
			float audioB = 0;
			// area listeners
			for(int i=0; i<numOfAreas; i++) {
				audioA = sampleAudio(areaListenerFragCoord[i][k][readState]);
				audioB = sampleAudio(areaListenerBFragCoord[i][k][readState]);
				
				retColor[channel] += audioA*(1-crossfade[i][k]) + audioB*crossfade[i][k];
				
				//retColor[channel] += sampleAudio(areaListenerFragCoord[i][readState]);
			}
		}
		writeYCoord += audioWriteChannelOffset; // move up to the next channel
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