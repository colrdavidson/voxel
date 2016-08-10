#ifndef CUBE_H
#define CUBE_H

GLfloat cube_points[] = {
	// front
	-1.0, -1.0,  1.0,
	1.0, -1.0,  1.0,
	1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,
	// top
	-1.0,  1.0,  1.0,
	1.0,  1.0,  1.0,
	1.0,  1.0, -1.0,
	-1.0,  1.0, -1.0,
	// back
	1.0, -1.0, -1.0,
	-1.0, -1.0, -1.0,
	-1.0,  1.0, -1.0,
	1.0,  1.0, -1.0,
	// bottom
	-1.0, -1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0, -1.0,  1.0,
	-1.0, -1.0,  1.0,
	// left
	-1.0, -1.0, -1.0,
	-1.0, -1.0,  1.0,
	-1.0,  1.0,  1.0,
	-1.0,  1.0, -1.0,
	// right
	1.0, -1.0,  1.0,
	1.0, -1.0, -1.0,
	1.0,  1.0, -1.0,
	1.0,  1.0,  1.0,
};

GLfloat roof_points[] = {
	// front
	-1.0, -1.0,  1.0,
	1.0, -1.0,  1.0,
	1.0,  0.0,  1.0,
	-1.0,  0.0,  1.0,
	// top
	-1.0,  0.0,  1.0,
	1.0,  0.0,  1.0,
	1.0,  0.0, -1.0,
	-1.0,  0.0, -1.0,
	// back
	1.0, -1.0, -1.0,
	-1.0, -1.0, -1.0,
	-1.0,  0.0, -1.0,
	1.0,  0.0, -1.0,
	// bottom
	-1.0, -1.0, -1.0,
	1.0, -1.0, -1.0,
	1.0, -1.0,  1.0,
	-1.0, -1.0,  1.0,
	// left
	-1.0, -1.0, -1.0,
	-1.0, -1.0,  1.0,
	-1.0,  0.0,  1.0,
	-1.0,  0.0, -1.0,
	// right
	1.0, -1.0,  1.0,
	1.0, -1.0, -1.0,
	1.0,  0.0, -1.0,
	1.0,  0.0,  1.0,
};

GLfloat door_points[] = {
	// front
	-1.0, -1.0,  1.0,
	1.0, -1.0,  1.0,
	1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,
	// top
	-1.0,  1.0,  1.0,
	1.0,  1.0,  1.0,
	1.0,  1.0, 0.0,
	-1.0,  1.0, 0.0,
	// back
	1.0, -1.0, 0.0,
	-1.0, -1.0, 0.0,
	-1.0,  1.0, 0.0,
	1.0,  1.0, 0.0,
	// bottom
	-1.0, -1.0, 0.0,
	1.0, -1.0, 0.0,
	1.0, -1.0,  1.0,
	-1.0, -1.0,  1.0,
	// left
	-1.0, -1.0, 0.0,
	-1.0, -1.0,  1.0,
	-1.0,  1.0,  1.0,
	-1.0,  1.0, 0.0,
	// right
	1.0, -1.0,  1.0,
	1.0, -1.0, 0.0,
	1.0,  1.0, 0.0,
	1.0,  1.0,  1.0,
};

GLfloat tree_points[] = {
	//front
	-1.0, -1.0,  0.0,
	1.0, -1.0,  0.0,
	1.0,  4.0,  0.0,
	-1.0,  4.0,  0.0,

	//right
	0.0, -1.0,  1.0,
	0.0, -1.0, -1.0,
	0.0,  4.0, -1.0,
	0.0,  4.0,  1.0,
};

GLfloat tree_normals[] = {
	//front
	0.0, 0.0, 1.0,
	0.0, 0.0, 1.0,
	0.0, 0.0, 1.0,
	0.0, 0.0, 1.0,

	//right
	1.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
};

GLfloat tree_tex_coords[] = {
	// front
	0.0, 0.0,
	1.0, 0.0,
	1.0, 1.0,
	0.0, 1.0,

	// right
	0.0, 0.0,
	1.0, 0.0,
	1.0, 1.0,
	0.0, 1.0,
};

GLushort tree_indices[] = {
	0, 1, 2,
	2, 3, 0,

	4, 5, 6,
	6, 7, 4,
};

GLfloat cube_normals[] = {
	// front
	0.0, 0.0, 1.0,
	0.0, 0.0, 1.0,
	0.0, 0.0, 1.0,
	0.0, 0.0, 1.0,

	// top
	0.0, 1.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 1.0, 0.0,

	// back
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,
	0.0, 0.0, -1.0,

	// bottom
	0.0, -1.0, 0.0,
	0.0, -1.0, 0.0,
	0.0, -1.0, 0.0,
	0.0, -1.0, 0.0,

	// left
	-1.0, 0.0, 0.0,
	-1.0, 0.0, 0.0,
	-1.0, 0.0, 0.0,
	-1.0, 0.0, 0.0,

	// right
	1.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
	1.0, 0.0, 0.0,
};

GLfloat cube_tex_coords[] = {
	// front
	0.0, 0.0,
	0.5, 0.0,
	0.5, 1.0,
	0.0, 1.0,

	// top
	0.5, 0.0,
	1.0, 0.0,
	1.0, 1.0,
	0.5, 1.0,

	// back
	0.0, 0.0,
	0.5, 0.0,
	0.5, 1.0,
	0.0, 1.0,

	// bottom
	0.5, 0.0,
	1.0, 0.0,
	1.0, 1.0,
	0.5, 1.0,

	// left
	0.0, 0.0,
	0.5, 0.0,
	0.5, 1.0,
	0.0, 1.0,

	// right
	0.0, 0.0,
	0.5, 0.0,
	0.5, 1.0,
	0.0, 1.0,
};

GLushort cube_indices[] = {
	0, 1, 2,
	2, 3, 0,

	4, 5, 6,
	6, 7, 4,

	8, 9, 10,
	10, 11, 8,

	12, 13, 14,
	14, 15, 12,

	16, 17, 18,
	18, 19, 16,

	20, 21, 22,
	22, 23, 20,
};

#endif
