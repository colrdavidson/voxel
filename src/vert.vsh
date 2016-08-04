#version 150

in vec3 coords;
in vec3 normals;
in vec2 tex_coords;

out vec2 f_tex_coords;
out vec3 f_normals;

uniform mat4 model;
uniform mat4 view;
uniform mat4 perspective;

void main() {
	mat4 mvp = perspective * view * model;

	gl_Position = mvp * vec4(coords, 1.0);
	f_tex_coords = tex_coords;
	f_normals = mat3(transpose(inverse(mat3(model)))) * normals;
}
