#version 450

in vec2 texture0;

out vec4 frag_color;

// set once
uniform vec2 deltaCoord_by2;
uniform vec2 deltaCoord_times3_by2;


uniform float maxPressure;

uniform sampler2D inputTexture;

uniform vec2 listenerFragCoord;
uniform vec2 feedbFragCoord;

uniform int excitationModel;
const int EX_NOFEEDBACK = 0;
const int EX_2MASS      = 1;

uniform vec4 excitationDirection;
uniform int exCellNum;

const int cell_wall       = 0;
const int cell_air        = 1;
const int cell_excitation = 2;
const int cell_pml0       = 3;
const int cell_pml1       = 4;
const int cell_pml2       = 5;
const int cell_pml3       = 6;
const int cell_pml4       = 7;
const int cell_pml5       = 8;
const int cell_dynamic    = 9;
const int cell_dead       = 10;
const int cell_nopressure = 11;
const int cell_numOFTypes = 12;

uniform vec2 mouseFragCoord;


const float log_min = 40.0f;
const float p_max = 5000.0f;
const float p_min = p_max / (pow(6.0f, log_min / 20.0f));

const vec4 color_white     = vec4(1);
const vec4 color_black     = vec4(0);
const vec4 color_red       = vec4(0.9, 0, 0, 1);
const vec4 color_green     = vec4(0, 1, 0, 1);
const vec4 color_blue      = vec4(0, 0, 1, 1);
const vec4 color_yellow    = vec4(1, 1, 0, 1);
const vec4 color_grey      = vec4(0.2, 0.2, 0.2, 1);
const vec4 color_lightBlue = vec4(0.0, 0.5, 1, 1);
const vec4 color_pink 	   = vec4(1, 0.0, 1, 1);
const vec4 color_purple    = vec4(0.6, 0.0, 1, 1);
const vec4 color_orange    = vec4(1, 0.4, 0.3, 1);

vec2 coord;

bool colDebug = false;

vec4 get_color_from_pressure(in float p)
{
	if(p > maxPressure)
		return color_red;
	else if (p<-maxPressure)
		return color_purple;
		
	float slider = abs(p / p_max);
	float log_scale = (20.0f * log(slider + /*1e-8*/0.03) / log(6.0f) + log_min) / log_min;
	float clamp_scale = clamp(log_scale, 0, 1);

	if (p > 0)
		return mix(vec4(0), color_white, clamp_scale);
	return mix(vec4(0), color_blue, clamp_scale);
}

void main () {
	coord.x = texture0.x;
	coord.y = texture0.y;
	frag_color = texture(inputTexture, coord);
	
	float cellType = frag_color.a;
	
	
	// color cells
	
	// air, the most likely case
	if( (cellType == cell_air) || (cellType >= cell_pml0 && cellType <= cell_pml5) || (cellType == cell_dead) /*|| (cellType == cell_nopressure)*/ ) {
		float p = frag_color.r;
		frag_color = get_color_from_pressure(p);
		
		if(cellType > cell_air)
			frag_color = mix(frag_color, color_grey, 0.5);
				
		if(colDebug) {
			if(cellType == cell_air)
				frag_color = color_black;
			else if(cellType != cell_dead) {
				int numPMLlayers = 6;
				float color = float(cellType-2)/numPMLlayers;
				frag_color.r = color;
				frag_color.g = 0;
				frag_color.b = 0;
			}
			else
				frag_color = color_blue;
		}
	}
	// walls
	else if(cellType == cell_wall) { 
		frag_color = color_green;
	}
	// excitation
	else if(cellType == cell_excitation) {
		//float p = frag_color.r/(maxPressure);
		float p;
		if(excitationDirection.x != 0)
			p = frag_color.g/excitationDirection.x;
		else
			p = frag_color.b/excitationDirection.y;
					
		p = p*exCellNum;
		
		
		p = frag_color.r;
		
		if(p >= 0) {
			frag_color.r = p;
			frag_color.g = p;
			frag_color.b = 0;
		}
		else {
			frag_color.r = 0;
			frag_color.g = -p;
			frag_color.b = -p;
		}
		
		if(colDebug)
			frag_color = color_yellow;
	}
	// soft wall, whose cell type is (0, 1)
	else if(cellType > cell_wall && cellType < cell_air) { 
		float p = frag_color.r;
		vec4 pressure_color = get_color_from_pressure(p);
		frag_color = mix(color_green, pressure_color, cellType);
		
		if(colDebug)
			frag_color = color_orange;
	}
	else if(cellType == cell_nopressure)
		frag_color = color_blue;
	
	if( (abs(coord.x - mouseFragCoord.x) <= deltaCoord_by2.x) || 
	    (abs(coord.y - mouseFragCoord.y) <= deltaCoord_by2.y) )
	    frag_color = mix(frag_color, color_white, 0.1);
	

	
	// feedback and listener rely on this weird fractional deltas because FBO shader needs 0.5 increment on coordinates to correctly pick frags' positions on texutre...
	// this is not true here, so we cope with this additional 0.5 with fractional deltas
	
	//feedback
	vec2 posDiff;
	
	if(excitationModel != EX_NOFEEDBACK) {
		posDiff = vec2(coord.x - feedbFragCoord.x, coord.y - feedbFragCoord.y);
		vec2 absDiff = vec2(abs(posDiff.x), abs(posDiff.y));
		// 4 corner dots
		if(absDiff.x>deltaCoord_by2.x) {
			if(absDiff.x<deltaCoord_times3_by2.x && absDiff.y >= deltaCoord_by2.y && absDiff.y < deltaCoord_times3_by2.y)
				frag_color = color_lightBlue;
		}
	}

	// listener
	posDiff = vec2(coord.x - listenerFragCoord.x, coord.y - listenerFragCoord.y);
	vec2 absDiff = vec2(abs(posDiff.x), abs(posDiff.y));
	// a 4 dot cross
	if(absDiff.x>deltaCoord_by2.x) {
		if(absDiff.x<deltaCoord_times3_by2.x && absDiff.y < deltaCoord_by2.y) 
			frag_color = color_pink;
	}
	else if(absDiff.y>deltaCoord_by2.y) {
		if(absDiff.y<=deltaCoord_times3_by2.y && absDiff.x < deltaCoord_by2.x) 
			frag_color = color_pink;
	}	
};
