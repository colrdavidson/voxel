#version 150
in vec2 coords;

void main() {
	float ratio = 1.33333;
    mat4 scale = mat4(vec4(0.5 / ratio, 0.0, 0.0, 0.0),
					  vec4(0.0, 0.5, 0.0, 0.0),
					  vec4(0.0, 0.0, 1.0, 0.0),
					  vec4(0.0, 0.0, 0.0, 1.0));
	gl_Position = scale * vec4(coords, 0.0, 1.0);
}
