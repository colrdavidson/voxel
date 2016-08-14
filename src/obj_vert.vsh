#version 330 core

in vec3 coords;
in vec3 normals;
in vec3 color;

in int tile_data;
in mat4 mvp;

flat out int f_tile_data;
out vec3 f_color;
out vec3 f_normals;

void main() {
	gl_Position = mvp * vec4(coords, 1.0);
	f_color = color;
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
