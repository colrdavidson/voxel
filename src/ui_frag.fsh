#version 330 core

flat in int f_tile_data;
in vec2 f_tex_coords;
in vec3 f_normals;

out vec4 color;
out int click;

uniform sampler2D tex;

void main() {
	vec4 tmp_color = texture(tex, f_tex_coords);

	if (tmp_color.a < 0.5) {
		discard;
	}

	color = tmp_color;
	click = f_tile_data;
}
