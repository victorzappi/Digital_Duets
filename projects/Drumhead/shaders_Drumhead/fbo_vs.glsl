#version 450

in vec4 in0;
in vec4 in1;
in vec4 in2;

out vec2 texture0;

out vec2 texture1;
out vec2 texture2;
out vec2 texture3;
out vec2 texture4;




void main () {	
	gl_Position = vec4 (in0.xy, 0.0, 1.0);
	texture0 = in0.zw;
	
	texture1 = in1.xy;
	texture2 = in1.zw;
	texture3 = in2.xy;
	texture4 = in2.zw;
}