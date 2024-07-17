#version 450

in vec4 pos_and_texc;

out vec2 tex_c;


void main () {
	gl_Position = vec4 (pos_and_texc.xy, 0.0, 1.0); // set position
	tex_c       = pos_and_texc.zw;                  // pass texture coord along to fragment shader
}