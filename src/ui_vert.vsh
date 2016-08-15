#version 330 core

in vec3 coords;
in vec3 normals;
in vec2 tex_coords;
in mat4 model;

flat out int f_tile_data;
out vec2 f_tex_coords;
out vec3 f_normals;

uniform mat4 pv;
uniform int tile_data;

void main() {
	gl_Position = pv * model * vec4(coords, 1.0);
	f_tex_coords = tex_coords;
	f_normals = normals;

	f_tile_data = tile_data << 3;
}
