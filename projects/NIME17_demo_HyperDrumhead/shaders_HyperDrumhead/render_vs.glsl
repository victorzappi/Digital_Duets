#version 450

in vec4 in0;

out vec2 texture0;


void main () {
	gl_Position = vec4 (in0.xy, 0.0, 1.0); // set position
	texture0    = in0.zw;                  // pass texture coord along to fragment shader
}