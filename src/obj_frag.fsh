#version 330 core

flat in int f_tile_data;
in vec3 f_color;
in vec3 f_normals;

out vec4 color;
out int click;

void main() {
	vec3 light_dir = vec3(0.2, 1.0, 0.6);
	vec4 light_color = vec4(0.8, 0.8, 0.8, 1.0);
	vec4 ambient = vec4(0.5, 0.5, 0.5, 1.0);

	float diff = max(dot(f_normals, light_dir), 0.0);
	vec4 diffuse = diff * light_color;

	vec4 tmp_color = (ambient + diffuse) * vec4(f_color, 1.0);

	if (tmp_color.a < 0.5) {
		discard;
	}

	color = tmp_color;
	click = f_tile_data;
}
