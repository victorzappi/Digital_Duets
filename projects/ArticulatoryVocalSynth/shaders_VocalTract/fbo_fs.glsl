#version 450

in vec2 texture0;

in vec2 texture1;
in vec2 texture2;

in vec2 texture3;
in vec2 texture4;

in vec2 texture5;
in vec2 texture6;

in vec2 texture7;
in vec2 texture8;

in vec2 texture9;


out vec4 color;


const int numOfStates= 4;


uniform sampler2D inOutTexture;

uniform float deltaCoordX;
uniform float deltaCoordY;

// coefficients
uniform float rho_c2_dt_invDs; // rho*c^2*dt/ds
uniform float dt_invDs_invRho; // dt/(ds*rho)

uniform float maxPressure;

// beta and sigma prime dt values
const int cell_excitation = 0;
const int cell_wall       = 1;
const int cell_air        = 2;
const int cell_pml0       = 3;
const int cell_pml1       = 4;
const int cell_pml2       = 5;
const int cell_pml3       = 6;
const int cell_pml4       = 7;
const int cell_dynamic    = 8;
const int cell_dead       = 9;
const int cell_numOFTypes = 10;
uniform vec2 typeValues[cell_numOFTypes]; // 10 = ImplicitFDTD::cell_numOFTypes

// quads to draw
const int quad_0   = 0;
const int quad_1   = 1;
const int quad_2   = 2;
const int quad_3   = 3;
const int quad_excitation_0 = 4;
const int quad_excitation_1 = 5;
const int quad_excitation_2 = 6;
const int quad_excitation_3 = 7;
const int quad_excitation_fu_0 = 8;
const int quad_excitation_fu_1 = 9;
const int quad_excitation_fu_2 = 10;
const int quad_excitation_fu_3 = 11;
// 4 drawing quads not used here
const int quad_drawer0   = 12;
const int quad_drawer1   = 13;
const int quad_drawer2   = 14;
const int quad_drawer3   = 15;
// extended audio quads [4 instead of one to pack info about latest state]
const int quad_audio_0  = 16;
const int quad_audio_1  = 17;
const int quad_audio_2  = 18;
const int quad_audio_3  = 19;
uniform int quad;


// types of excitation
const int ex_noFeedback = 0;
const int ex_2mass_kees = 1;
const int ex_2mass      = 2;
const int ex_body_cover = 3;
const int ex_clarinet_reed = 4;
const int ex_sax_reed   = 5;
uniform int excitationModel;

uniform float excitationInput;
uniform float inv_exCellNum;
uniform vec4 excitationDirection;
uniform vec2 exFragCoord[4];

uniform vec2 feedbFragCoord[4];

uniform vec3 a1Input;


//----------------------------------------------------------------------------------------------------------
// The bottom part of the texture [audio row or audio quad] cotnains the rendered audio for the current audio buffer. 
// Each pixel contains 4 consecutive mono audio frames, as RGBA data. They form the "audio row[s]".
// There might be more than one audio row if needed. Look at FDTD::fillVertices() in FTDT.cpp file for more details

// On the CPU side a "listener" pixel is chosen. When a the audio quad is computed, the FBO copies the 
// pressure value calculated in the listener pixel at the current step into the audio row. 
// This does not necessarily happen at every FBO call, cos in general the FBO needs a higher rate than the audio sample rate
// for matter of stability.

// The listener position in all the quad [all states] is saved here
// two of them are used for crossfade
// A
uniform vec2 listenerAFragCoord[4];
// B
uniform vec2 listenerBFragCoord[4];

// and this is the crossfade control to move between A and B
uniform float crossfade;  

// This vec3 contains the current write position in the audio quad/audio row
// This position is moved forward from the CPU side
uniform vec3 audioWriteCoords;   // (audioWritePixelcoordX, audioWritePixelcoordX, RBGAindex)
// splits as:
// writeXCoord  = audioWriteCoords[0]
// writeYCoord  = audioWriteCoords[1]
// writeChannel = audioWriteCoords[2]

// where:

// writeXCoord[0] = audioWritePixelcoordX 
// writeYCoord[1]  = audioWritePixelcoordY, these coordinates tell what column pixel to write audio to in the current quad
// writeChannel[2] = RBGAindex, this determines what pixel's channel to write to
//----------------------------------------------------------------------------------------------------------




uniform int filter_toggle;
uniform vec2 viscothermal_coefs;
//===============================
// VISCOTHERMAL COEFFICIENTS
//===============================

// deafault Aerophones, not sure how calculated
/*const float visco_a1 = -1.35;
const float visco_a2 = 0.4;
const float visco_b0 = 495.922;
const float visco_b1 = -857.045;
const float visco_b2 = 362.79;*/



// 3*44100 with VT params, between 100 and 10000 Hz, a = 0.005
//Min Error: 0.170409
//Coeffs: a:[-0.7, -0.25], b:[895.246, -1577.38, 683.533]	OK <---
//Min Error: 0.169349
//Coeffs: a:[-0.7, -0.25], b:[866.766, -1577.38, 709.823] 	OK but not with closed shapes
//Min Error: 0.169238
//Coeffs: a:[-0.7, -0.25], b:[838.286, -1577.38, 736.113] 	OK but not with closed shapes
//Min Error: 0.169122
//Coeffs: a:[-0.7, -0.25], b:[810.9, -1551.09, 736.113]   	OK but not with closed shapes
//Min Error: 0.168329
//Coeffs: a:[-0.65, -0.3], b:[781.709, -1577.38, 788.692] 	OK but not with closed shapes

// 4*44100 with VT params, between 100 and 10000 Hz, a = 0.005
//Min Error: 0.299485
//Coeffs: a:[-1.85, 0.85], b:[341.755, -683.533, 341.767]	OK
//Min Error: 0.271268
//Coeffs: a:[-1.3, 0.35], b:[633.642, -1104.17, 473.215]	OK
//Min Error: 0.232975
//Coeffs: a:[-1.15, 0.2], b:[765.921, -1314.49, 552.085] 	OK
//Min Error: 0.221478
//Coeffs: a:[-1.05, 0.1], b:[844.508, -1445.94, 604.664] 	OK <---
//Min Error: 0.201534
//Coeffs: a:[-0.8, -0.15], b:[865.587, -1577.38, 709.823] 	OK but not with closed shapes
//Min Error: 0.190300
//Coeffs: a:[-0.7, -0.25], b:[411.576, -1551.09, 1104.17] 	NO
//Min Error: 0.188472
//Coeffs: a:[-0.5, -0.45], b:[-65.0837, -1577.38, 1577.38] 	NO

// 5*44100 with VT params, between 100 and 10000 Hz, a = 0.005
//Min Error: 0.244172
//Coeffs: a:[-1, 0.05], b:[923.372, -1577.38, 657.243]		OK
//Min Error: 0.240591
//Coeffs: a:[-1, 0.05], b:[894.162, -1577.38, 683.533]		OK <---
//Min Error: 0.237752
//Coeffs: a:[-1, 0.05], b:[864.951, -1577.38, 709.823]		OK but not with closed shapes
//Min Error: 0.235205
//Coeffs: a:[-0.95, 0], b:[836.575, -1577.38, 736.113]		OK but not with closed shapes
//Min Error: 0.231942
//Coeffs: a:[-0.95, 0], b:[807.519, -1577.38, 762.402]		OK but not with closed shapes
//Min Error: 0.231163
//Coeffs: a:[-0.95, 0], b:[778.462, -1577.38, 788.692]		OK but not with closed shapes
//Min Error: 0.224645
//Coeffs: a:[-0.85, -0.1], b:[720.203, -1577.38, 841.272]	NO
//Min Error: 0.215753
//Coeffs: a:[-0.8, -0.15], b:[520.518, -1577.38, 1025.3]	NO
//Min Error: 0.211348
//Coeffs: a:[-0.75, -0.2], b:[351.912, -1551.09, 1156.75]	NO
//Min Error: 0.206239
//Coeffs: a:[-0.6, -0.35], b:[-70.6288, -1577.38, 1577.38]	NO

// 6*44100 with VT params, between 100 and 10000 Hz, a = 0.005
//Min Error: 0.283687
//Coeffs: a:[-1.1, 0.15], b:[981.998, -1577.38, 604.664]	OK
//Min Error: 0.280117
//Coeffs: a:[-1.1, 0.15], b:[922.847, -1577.38, 657.243]	OK <---
//Min Error: 0.279342
//Coeffs: a:[-1.1, 0.15], b:[893.271, -1577.38, 683.533]	OK but not with closed shapes
//Min Error: 0.270081
//Coeffs: a:[-1.05, 0.1], b:[864.842, -1577.38, 709.823]	OK but not with closed shapes
//Min Error: 0.267878
//Coeffs: a:[-1.05, 0.1], b:[835.46, -1577.38, 736.113]		OK but not with closed shapes
//Min Error: 0.266687
//Coeffs: a:[-1.05, 0.1], b:[806.078, -1577.38, 762.402]	OK but not with closed shapes
//Min Error: 0.266635
//Coeffs: a:[-1.05, 0.1], b:[612.771, -1367.07, 736.113]	OK but not with closed shapes
//Min Error: 0.266387
//Coeffs: a:[-1, 0.05], b:[834.022, -1577.38, 736.113]		OK but not with closed shapes
//Min Error: 0.264898
//Coeffs: a:[-0.95, 0], b:[864.144, -1577.38, 709.823]		OK but not with closed shapes
//Min Error: 0.244104
//Coeffs: a:[-0.8, -0.15], b:[548.274, -1577.38, 999.01]	NO
//Min Error: 0.233819
//Coeffs: a:[-0.75, -0.2], b:[235.7, -1577.38, 1288.2]		NO
//Min Error: 0.227667
//Coeffs: a:[-0.65, -0.3], b:[-74.1468, -1577.38, 1577.38]  NO



// 3-4-5-6*44100
const mat4 visco_coeff_a = mat4( vec4(1, -0.7, -0.25, 0), vec4(1, -1.1, 0.15, 0), vec4(1, -1.1, 0.15, 0), vec4(1, -1.1, 0.15, 0) );
const mat4 visco_coeff_b = mat4( vec4(895.246, -1577.38, 683.533, 0), vec4(844.508, -1445.94, 604.664, 0), 
						         vec4(894.162, -1577.38, 683.533, 0), vec4(922.847, -1577.38, 657.243, 0) ); 
uniform int filterIndex;

const float visco_a1 = visco_coeff_a[clamp(filterIndex, 0, 3)][1];
const float visco_a2 = visco_coeff_a[clamp(filterIndex, 0, 3)][2];
const float visco_b0 = visco_coeff_b[clamp(filterIndex, 0, 3)][0];
const float visco_b1 = visco_coeff_b[clamp(filterIndex, 0, 3)][1];
const float visco_b2 = visco_coeff_b[clamp(filterIndex, 0, 3)][2];

    


//===============================
// excitation low pass 
//===============================
uniform int toggle_exLp;
// coefficients [change according to sample rate]
// also used in excitation models
uniform float exLp_a1;
uniform float exLp_a2;
uniform float exLp_b0;
uniform float exLp_b1;
uniform float exLp_b2;


//===============================
// excitation models' declarations
//===============================
vec4 two_mass_excitation_kees();
vec4 two_mass_excitation();
vec4 body_cover_excitation(bool isFollowup);
vec4 reed_excitation(bool isClarinet);

// 2 mass and 2 mass kee
uniform float dampingMod;
uniform float pitchFactor;
uniform float rho;
uniform float mu;
uniform float restArea;
uniform vec2 masses;
uniform vec2 thicknesses;
uniform vec3 springKs;
uniform vec3 rates;
// body cover
uniform float c;
uniform float rho_c;
// reed
uniform float inv_ds;



vec3 sigma_prime_dt;
vec3 beta;



float getExcitation(in int quadIndex) {
	return texture(inOutTexture, exFragCoord[quadIndex])[toggle_exLp*3];
}

float getFeedback(in int quadIndex) {
	return texture(inOutTexture, feedbFragCoord[quadIndex]).x;
}

vec2 getCellStateData(in sampler2D text, in vec2 texCoordinates, out int type){
	type = int(texture(text, texCoordinates).a);
	return typeValues[type]; 	
}



// shader color dbg: helps check that the type of cell is recognized
// CPU color dbg: helps check we put correctly displaced cell's type on texture

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// P(n+1)  = [ P(n) - r*c^2*dt * (Vx(n) - Vx_left(n) + Vy(n) - Vy_down(n) ) / ds ] / [ 1 + ( 1 - b + s) * dt ]
//
// Vx(n+1) = { b*Vx(n) - [ b^2*dt * ( P_right(n+1) - P(n+1) ) / ds*r ] + ( 1 - b + s) * dt*Vxb(n+1) } / [ b + ( 1 - b + s) * dt ]
//
// Vy(n+1) = { b*Vy(n) - [ b^2*dt * ( P_up(n+1)    - P(n+1) ) / ds*r ] + ( 1 - b + s) * dt*Vyb(n+1) } / [ b + ( 1 - b + s) * dt ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FDTD
vec4 computeFDTD() {
 	// intepret attributes passed from vertex shader
 	vec2 tex_c  = texture0;
 	vec2 tex_l  = texture1;
 	vec2 tex_u  = texture2; 
 	vec2 tex_r  = texture3;
 	vec2 tex_d  = texture4; 
 	vec2 tex_lu = texture5;
 	vec2 tex_rd = texture6; 
 	vec2 tex_c_prev = texture7; 
 	vec2 tex_r_prev = texture8; 
 	vec2 tex_u_prev = texture9; 
 	
 	vec3 c, r, u;
	vec3 l, d, lu, rd;
	vec3 c_prev, r_prev, u_prev;
	ivec4 cellTypes;
	
	// pressure, velocity and type
	c = texture(inOutTexture, tex_c).rgb;
	r = texture(inOutTexture, tex_r).rgb;
	u = texture(inOutTexture, tex_u).rgb;
	
	// this cell
	vec2 data = getCellStateData(inOutTexture, tex_c, cellTypes[0]);
	beta[0]           = data[0];
	sigma_prime_dt[0] = data[1];
	
	// right neighbour cell
	data = getCellStateData(inOutTexture, tex_r, cellTypes[1]);
	beta[1]           = data[0];
	sigma_prime_dt[1] = data[1];
	
	// upper neighbour cell
	data = getCellStateData(inOutTexture, tex_u, cellTypes[2]);
	beta[2]           = data[0];
	sigma_prime_dt[2] = data[1];
	
	// velocities needed
	l  = texture(inOutTexture, tex_l).rgb;
	d  = texture(inOutTexture, tex_d).rgb;
	lu = texture(inOutTexture, tex_lu).rgb;
	rd = texture(inOutTexture, tex_rd).rgb;
	
	// pressure update for current, right, and up
	vec3 p  = vec3(c.x, r.x, u.x);
	vec3 vx = vec3(c.y, r.y, u.y);
	vec3 vy = vec3(c.z, r.z, u.z);
	vec3 vx_left = vec3(l.y, c.y, lu.y);
	vec3 vy_down = vec3(d.z, rd.z, c.z);
	
	
	vec2 max_sigma_dt = max(sigma_prime_dt.xx, sigma_prime_dt.yz);
	vec2 min_beta     = min(beta.xx, beta.yz);
	
	vec3 bulk_dt_by_dx = vec3(rho_c2_dt_invDs); 
	
	// update pressure
	vec3 p_next = (p - bulk_dt_by_dx * (vx - vx_left + vy - vy_down)) / (1 + sigma_prime_dt);
	
	vec2 dt_by_rho_by_dx = vec2(dt_invDs_invRho);
	// velocity (x,y) update for current
	vec2 v_next = (min_beta * c.yz - min_beta * min_beta * dt_by_rho_by_dx * (p_next.yz - p_next.xx));
	
	
	//----------------------------
 	// excitations
	//----------------------------
 	bvec3 is_excitation_bool = bvec3(cellTypes.x == cell_excitation, cellTypes.y == cell_excitation, cellTypes.z == cell_excitation);
 	vec3 is_excitation = vec3(is_excitation_bool);
	vec2 excitation_weight = is_excitation.xx * excitationDirection.zw + is_excitation.yz * excitationDirection.xy;
		
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	float excitation_velocity = getExcitation(quad); 
	//excitation_velocity = excitation_input; 
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	
	v_next += excitation_velocity * excitation_weight * max_sigma_dt;	 
	//----------------------------
	
	
	//----------------------------
	// viscothermal losses
	//----------------------------
	// previous values
	c_prev = texture(inOutTexture, tex_c_prev).rgb;
	r_prev = texture(inOutTexture, tex_r_prev).rgb;
	u_prev = texture(inOutTexture, tex_u_prev).rgb;
	
	vec2 visco_velsqr = (c.yz + vec2(1e-8)) * (c.yz + vec2(1e-8));
	vec2 sinsqrtheta = 1.0f / (1.0f + visco_velsqr.xy / visco_velsqr.yx);
	vec2 feedforward_scaling = (viscothermal_coefs.x * sinsqrtheta + viscothermal_coefs.y);
	vec4 feedforward = visco_b0 * p_next.xyxz + visco_b1 * p.xyxz + visco_b2 * vec4(c_prev.x, r_prev.x, c_prev.x, u_prev.x);
	feedforward *= feedforward_scaling.xxyy;
	vec2 feedback = visco_a1 * c.yz + visco_a2 * c_prev.yz;
	
	// beta.x current cell
	// beta.y right neighbour
	// beta.z up neighbour
	// beta = 0 -> wall
	// beta = 1 -> air
	//              air?             wall?
	vec4 xor_term = beta.yxzx * (1 - beta.xyxz);
	
	vec2 are_we_not_excitations = (1 - is_excitation.xx) * (1 - is_excitation.yz);
	vec2 vb_thermal = are_we_not_excitations * (xor_term.xz * (feedforward.yw - feedback.xy) + xor_term.yw * (-feedforward.xz - feedback.xy));
		 
	//if (filter_toggle == 1) {
	//	v_next += vb_thermal * max_sigma_dt;
	//}
	
	//VIC does this work better?
	v_next += filter_toggle * vb_thermal * max_sigma_dt;
	//----------------------------
	
		
	v_next /= (min_beta + max_sigma_dt);
		
	// save colors [P and V] and maintain old alpha [cell type]
	vec4 retColor =  vec4(p_next[0], v_next, cellTypes.x);
	
	//VIC
	//retColor.x = getExcitation(quad)*50;
	//retColor.x = filter_toggle;
	//retColor =  vec4(sinsqrtheta.x, vec2(0), cellTypes.x);

	return retColor;
}


// writes audio into audio column from listener, in the correct pixel
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

		// if yes, move the listener coordinates into the current quad
		// PLUS 
		// always give the possibility to control a crossfade with the previous listener position, 
		// just in case someone changes the position from the CPU
		// in this way we have a smooth transition |:<
		float _crossfade = 1 - crossfade;   

		vec2 audioCoordA = listenerAFragCoord[readState];
		vec2 audioCoordB = listenerBFragCoord[readState];
			
		//float audio = texture(inOutTexture, audioCoordA).r*_crossfade + texture(inOutTexture, audioCoordB).r*crossfade;
		
		// get the audio from the listener 
		
		// check if we are reading directly from excition
		vec4 fragA = texture(inOutTexture, audioCoordA);
		vec4 fragB = texture(inOutTexture, audioCoordB);
		float audioA;
		float audioB;
		
		// this boosts amplitude of original excitation
		float ampFact  = 2000; //TODO: find value that sounds the same no matter how many excitation cells
		 
		
		if(fragA.a != cell_excitation )
			audioA = fragA.r;
		else { // excitation cell
			// hear oiginal input properly amplified
			audioA = excitationInput * ampFact;
			
			// this hsould be the correct one, but gets noisy
			int exIndex = quad-1;
			if(exIndex < 0)
				exIndex = numOfStates-1;
			float excitation = getExcitation(exIndex);
			audioA = excitation * ampFact / (25 * inv_exCellNum);

		}
				
		if(fragB.a != cell_excitation)
			audioB = fragB.r;
		else { // excitation cell 
			// hear original input properly amplified
			audioB = excitationInput * ampFact;
			
			// this should be the correct one, but gets noisy
			int exIndex = quad-1;
			if(exIndex < 0)
				exIndex = numOfStates-1;
			float excitation = getExcitation(exIndex);
			audioB = excitation * ampFact / (25 * inv_exCellNum);
		}
		
		//VIC
		//audioA = fragA.r;
		//audioB = fragB.r;
		
		float audio = audioA*_crossfade + audioB*crossfade;
		
		// put in the correct channel, according to the audio write command sent from cpu	
		retColor[int(writeChannel)] = audio/maxPressure;
		//retColor[int(writeChannel)] = audio; //VIC
	}
	return retColor;
}




// calculates new excitation value - excited cells will read from here through exFragCoord[state]
vec4 computeExcitation() {
	if(excitationModel == ex_noFeedback) {
		float excite = excitationInput * 25 *inv_exCellNum; // *25, like in Aerophones, cos sin excitation is filtered through an anti aliasing FIR with a gain of 5 in Aerophones and then in shader everything multiplied by 5 again!
		return vec4(excite, 0, 0, excite); // simple pass-thru, put also in alpha channel to comply with how filtered feedback excitation models work [look at fbo_excitation... files and in this file at getExcitation(in int quadIndex)]
		// excitation values for no feedback case already come with no aliasing [filtered on CPU side]  
	}
	else if(excitationModel == ex_2mass_kees) {
		return two_mass_excitation_kees();
	}
	else if(excitationModel == ex_2mass) {
		return two_mass_excitation();
	}
	else if(excitationModel == ex_body_cover) {
		return body_cover_excitation(false);
	}
	else if(excitationModel == ex_clarinet_reed) {
		return reed_excitation(true);
	}
	else if(excitationModel == ex_sax_reed) {
		return reed_excitation(false);
	}
}

// calculate excitation followups [only where needed]
vec4 computeExcitationFollowup() {
	
	if(excitationModel == ex_body_cover)
		return body_cover_excitation(true);
	else
		discard;
}





void main () {

	if(quad >= quad_0 && quad <= quad_3) {	
		// regular FDTD calculation
		color = computeFDTD();
	}
	else if(quad >= quad_audio_0 && quad <= quad_audio_3) {
		// save audio routine
		color = saveAudio();
	}
	else if(quad >= quad_excitation_0 && quad <= quad_excitation_3) {
		// compute excitation
		color = computeExcitation();
	} 	
	else if(quad >= quad_excitation_fu_0 && quad <= quad_excitation_fu_3) {
		// compute excitation follow up
		color = computeExcitationFollowup();
	}
	// for color debugging with no computations---> to be activated together with TextureFitler::colored field
	//color = texture(inOutTexture, texture0);			
};