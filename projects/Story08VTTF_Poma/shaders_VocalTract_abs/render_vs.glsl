#version 450

//in vec2 position;
//in vec2 texcoordN;

//out vec2 TexcoordN;

in vec4 in0;

out vec2 texture0;


void main () {
	// pass it along to fragment shader
	//TexcoordN = texcoordN;
	//gl_Position = vec4 (position, 0.0, 1.0);
	
	gl_Position = vec4 (in0.xy, 0.0, 1.0);
	texture0 = in0.zw;
}