#version 450

in vec4 pos_and_texc;
in vec4 texl_and_texu;
in vec4 texr_and_texd;

out vec2 tex_c;
out vec2 tex_l;
out vec2 tex_u;
out vec2 tex_r;
out vec2 tex_d;

void main () {	
	gl_Position = vec4 (pos_and_texc.xy, 0.0, 1.0);
	tex_c = pos_and_texc.zw;
	
	tex_l = texl_and_texu.xy;
	tex_u = texl_and_texu.zw;
	tex_r = texr_and_texd.xy;
	tex_d = texr_and_texd.zw;
}