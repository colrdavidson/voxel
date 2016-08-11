#version 330 core

in vec3 coords;
in vec3 normals;
in vec2 tex_coords;

flat out int f_tile_data;
out vec2 f_tex_coords;
out vec3 f_normals;

uniform mat4 mvp;
uniform int tile_data;

void main() {
	gl_Position = mvp * vec4(coords, 1.0);
	f_tex_coords = tex_coords;
	f_normals = normals;

	int x = int(normals.x) * 4;
	int y = int(normals.y) * 2;
	int z = int(normals.z);

	int side = x + y + z;

	switch (side) {
		// front
		case 1: {
			side = 0;
		} break;
		// top
		case 2: {
			side = 1;
		} break;
		// back
		case -1: {
			side = 2;
		} break;
		// bottom
		case -2: {
			side = 3;
		} break;
		// left
		case -4: {
			side = 4;
		} break;
		// right
		case 4: {
			side = 5;
		} break;
		// error
		default: {
			side = 6;
		} break;
	}

	f_tile_data = tile_data << 3 | side;
}
