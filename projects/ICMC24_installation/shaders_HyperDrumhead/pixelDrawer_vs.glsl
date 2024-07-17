#version 450

in vec4 in0;

out vec2 tex_norm;

void main() {
	gl_Position = vec4(in0.xy, 0, 1);
	tex_norm    = in0.zw; // drawing texture coord
}