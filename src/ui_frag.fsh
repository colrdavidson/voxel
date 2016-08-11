#version 330 core

in vec2 f_tex_coords;
in vec3 f_normals;

out vec4 color;

uniform sampler2D tex;

void main() {
	/*
	vec4 tmp_color = texture(tex, f_tex_coords);

	if (tmp_color.a < 0.5) {
		discard;
	}*/

	color = vec4(1.0, 1.0, 1.0, 1.0);
}
