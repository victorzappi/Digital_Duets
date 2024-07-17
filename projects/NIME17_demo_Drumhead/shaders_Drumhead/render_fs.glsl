#version 450

in vec2 texture0;

out vec4 frag_color;

// set once
uniform vec2 deltaCoord_by2;
uniform vec2 deltaCoord_times3_by2;


float maxVol = 20;

uniform sampler2D inputTexture;

uniform vec2 listenerFragCoord;

const int cell_wall       = 0;
const int cell_air        = 1;
const int cell_excitation = 2;
const int cell_dead       = 3;
const int cell_numOFTypes = 4;

uniform vec2 mouseFragCoord;


const float log_min = 10.0f;

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
	if(p > maxVol)
		return color_red;
	else if (p<-maxVol)
		return color_purple;
		
	float slider = abs(p / maxVol);
	float log_scale = (log(slider + 1e-8) + log_min) / log_min;
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
	if(cellType == cell_air) {
		float p = frag_color.r;
		frag_color = get_color_from_pressure(p);
				
		if(colDebug)
			frag_color = color_black;
	}
	// walls
	else if(cellType == cell_wall) { 
		frag_color = mix(color_green, get_color_from_pressure(frag_color.r), frag_color.b);
	}
	// excitation
	else if(cellType == cell_excitation) {
		float p = frag_color.r;
		vec4 color = get_color_from_pressure(p);
		
		frag_color = mix(color, color_yellow, 0.5);
		
		if(colDebug)
			frag_color = color_yellow;
	}
	else if(cellType == cell_dead) {
		frag_color = color_green;
		if(colDebug)
			frag_color = color_blue;
	}
	
	if( (abs(coord.x - mouseFragCoord.x) <= deltaCoord_by2.x) || 
	    (abs(coord.y - mouseFragCoord.y) <= deltaCoord_by2.y) )
	    frag_color = mix(frag_color, color_white, 0.1);
	
	
	
	// listener relies on this weird fractional deltas because FBO shader needs 0.5 increment on coordinates to correctly pick frags' positions on texutre...
	// this is not true here, so we cope with this additional 0.5 with fractional deltas
	
	// listener
	vec2 posDiff = vec2(coord.x - listenerFragCoord.x, coord.y - listenerFragCoord.y);
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
