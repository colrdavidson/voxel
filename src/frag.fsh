#version 330 core

in vec2 f_tile_data;
in vec2 f_tex_coords;
in vec3 f_normals;

out vec4 color;
out vec2 click;

uniform sampler2D tex;

void main() {
	vec3 light_dir = vec3(0.1, 1.0, 0.5);
	vec4 light_color = vec4(0.8, 0.8, 0.8, 1.0);
	vec4 ambient = vec4(0.3, 0.3, 0.3, 1.0);

	float diff = max(dot(f_normals, light_dir), 0.0);
	vec4 diffuse = diff * light_color;

	vec2 flipped_tex_coords = vec2(f_tex_coords.x, 1.0 - f_tex_coords.y);
	vec4 tmp_color = (ambient + diffuse) * texture(tex, flipped_tex_coords);

	if (tmp_color.a < 0.5) {
		discard;
	}

	color = tmp_color;
	click = f_tile_data;
}
