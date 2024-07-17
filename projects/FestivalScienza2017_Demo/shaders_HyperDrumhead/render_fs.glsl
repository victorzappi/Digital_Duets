#version 450

in vec2 texture0;

out vec4 frag_color;

// set once
uniform vec2 deltaCoord_by2;
uniform vec2 deltaCoord_times3_by2;

float maxVol = 20;

const int maxNumOfAreas = 10;
uniform int numOfAreas;


uniform sampler2D inputTexture;

uniform vec2 listenerFragCoord;

uniform vec2 areaListenerFragCoord[maxNumOfAreas];

const int cell_boundary   = 0;
const int cell_air        = 1;
const int cell_excitation = 2;
const int cell_dead       = 3;
const int cell_numOFTypes = 4;

uniform vec2 mouseFragCoord;

uniform int showAreas;
uniform int selectedArea;


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
const vec4 color_dark_purple = vec4(0.5, 0.0, 1, 1);
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

vec4 get_color_from_pressure(in float p, vec4 col)
{
	if(p > maxVol)
		return color_red;
	else if (p<-maxVol)
		return color_purple;
		
	float slider = abs(p / maxVol);
	float log_scale = (log(slider + 1e-8) + log_min) / log_min;
	float clamp_scale = clamp(log_scale, 0, 1);

	if (p > 0)
		return mix(col, color_white, clamp_scale);
	return mix(col, color_blue, clamp_scale);
}

void main() {
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
		else if(showAreas == 1) {
			float area = texture(inputTexture, coord).g;
			frag_color = mix(color_black, color_white, area/numOfAreas);
			if(area == selectedArea)
				frag_color = mix(frag_color, color_orange, 0.5);				
		}
	}
	// boundaries
	else if(cellType == cell_boundary) { 
		frag_color = mix(color_green, color_dark_purple, frag_color.b); // boundary gain determines boundary color
	
		//float p = frag_color.r;
		//vec4 frag_color_p = get_color_from_pressure(p, color_dark_purple);
			
		//frag_color = mix(color_green, frag_color_p, frag_color.b); // boundary gain determines boundary color
	}
	// excitation
	else if(cellType == cell_excitation) {
		float p = frag_color.r;
		vec4 color = get_color_from_pressure(p);
		
		frag_color = mix(color, color_yellow/*(1-chupa[0].x)*/, 0.2);
		
		if(colDebug)
			frag_color = color_yellow;
	}
	else if(cellType == cell_dead) {
		frag_color = mix(color_green, color_dark_purple, frag_color.b); // boundary gain determines boundary color
	
		//frag_color = color_green;
		
		//float p = frag_color.r;
		//vec4 frag_color_p = get_color_from_pressure(p, color_dark_purple);
			
		//frag_color = mix(color_green, frag_color_p, frag_color.b); // boundary gain determines boundary color
		
		if(colDebug)
			frag_color = color_blue;
	}
	
	
	// mouse lines
	vec4 mix_color = color_white;
	float mixVal = 0.1;
	if(showAreas == 1) {
		mix_color = color_blue;
		mixVal = 0.4;
	}
		
	if( (abs(coord.x - mouseFragCoord.x) <= deltaCoord_by2.x) || 
	    (abs(coord.y - mouseFragCoord.y) <= deltaCoord_by2.y) )
	    frag_color = mix(frag_color, mix_color, mixVal);
	
	// listener relies on this weird fractional deltas because FBO shader needs 0.5 increment on coordinates to correctly pick frags' positions on texutre...
	// this is not true here, so we cope with this additional 0.5 with fractional deltas
	
	
	vec2 posDiff;
	vec2 absDiff;
	
	// listener
	/*posDiff = vec2(coord.x - listenerFragCoord.x, coord.y - listenerFragCoord.y);
	absDiff = vec2(abs(posDiff.x), abs(posDiff.y));
	
	// a 4 dot cross
	if(absDiff.x>deltaCoord_by2.x) {
		if(absDiff.x<deltaCoord_times3_by2.x && absDiff.y < deltaCoord_by2.y) 
			frag_color = mix(frag_color, color_pink, 0.5);
	}
	else if(absDiff.y>deltaCoord_by2.y) {
		if(absDiff.y<=deltaCoord_times3_by2.y && absDiff.x < deltaCoord_by2.x) 
			frag_color = mix(frag_color, color_pink, 0.5);
	}	*/
	
	
	// area listeners
	for(int i=0; i<numOfAreas; i++) {
		posDiff = vec2(coord.x - areaListenerFragCoord[i].x, coord.y - areaListenerFragCoord[i].y);
		absDiff = vec2(abs(posDiff.x), abs(posDiff.y));
		// a 4 dot cross
		if(absDiff.x>deltaCoord_by2.x) {
			if(absDiff.x<deltaCoord_times3_by2.x && absDiff.y < deltaCoord_by2.y) 
				frag_color =  mix(frag_color, color_lightBlue, 0.5);
		}
		else if(absDiff.y>deltaCoord_by2.y) {
			if(absDiff.y<=deltaCoord_times3_by2.y && absDiff.x < deltaCoord_by2.x) 
				frag_color = mix(frag_color, color_lightBlue, 0.5);
		}	
	}

	
};
