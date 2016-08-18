#version 330 core

in vec3 coords;
in vec3 color;

in vec3 model;

uniform mat4 pv;

out vec3 f_color;

void main() {
	mat4 m = mat4(vec4(1.0, 0.0, 0.0, 0.0),
				  vec4(0.0, 1.0, 0.0, 0.0),
				  vec4(0.0, 0.0, 1.0, 0.0),
				  vec4(model, 1.0));

	gl_Position = pv * m * vec4(coords, 1.0);
	f_color = color;
}
