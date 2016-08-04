#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

void get_shader_err(GLuint shader) {
	GLint err_log_max_length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &err_log_max_length);
	char *err_log = (char *)malloc(err_log_max_length);

	GLsizei err_log_length = 0;
	glGetShaderInfoLog(shader, err_log_max_length, &err_log_length, err_log);
	printf("%s\n", err_log);
}

GLint build_shader(const char *file_string, GLenum shader_type) {
	GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &file_string, NULL);

	glCompileShader(shader);

	GLint compile_success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_success);

	if (!compile_success && shader_type == GL_VERTEX_SHADER) {
		printf("Vertex Shader %d compile error!\n", shader);
		get_shader_err(shader);
		return 0;
	} else if (!compile_success && shader_type == GL_FRAGMENT_SHADER) {
		printf("Fragment Shader %d compile error!\n", shader);
		get_shader_err(shader);
		return 0;
	}

	return shader;
}

GLuint load_and_build_program(char *vert_filename, char *frag_filename) {
	GLuint shader_program = glCreateProgram();

	char *vert_file = file_to_string(vert_filename);
	char *frag_file = file_to_string(frag_filename);

	GLint vert_shader = build_shader(vert_file, GL_VERTEX_SHADER);
	GLint frag_shader = build_shader(frag_file, GL_FRAGMENT_SHADER);

	free(vert_file);
	free(frag_file);

	glAttachShader(shader_program, vert_shader);
	glAttachShader(shader_program, frag_shader);

	glLinkProgram(shader_program);

	return shader_program;
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);

	SDL_Window *window = SDL_CreateWindow("Voxel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	GLuint shader_program = load_and_build_program("src/vert.vsh", "src/frag.fsh");

	GLuint wood_tex = 0;
	SDL_Surface *wood_surf = IMG_Load("wood.png");
	glGenTextures(1, &wood_tex);
	glBindTexture(GL_TEXTURE_2D, wood_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wood_surf->w, wood_surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, wood_surf->pixels);
	SDL_FreeSurface(wood_surf);

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	f32 ratio = 640.0 / 480.0;

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

	GLuint vbo_cube_points = 0;
	GLuint vbo_cube_tex_coords = 0;
	GLuint vbo_cube_normals = 0;
	GLuint ibo_cube_indices = 0;

	glGenBuffers(1, &vbo_cube_points);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_points);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_points), cube_points, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_cube_tex_coords);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_tex_coords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex_coords), cube_tex_coords, GL_STATIC_DRAW);

	glGenBuffers(1, &vbo_cube_normals);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);

	glGenBuffers(1, &ibo_cube_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

	GLuint points_attr = glGetAttribLocation(shader_program, "coords");
	GLuint texpos_attr = glGetAttribLocation(shader_program, "tex_coords");
	GLuint normals_attr = glGetAttribLocation(shader_program, "normals");

	GLuint cube_model_uniform = glGetUniformLocation(shader_program, "model");
	GLuint view_uniform = glGetUniformLocation(shader_program, "view");
	GLuint perspective_uniform = glGetUniformLocation(shader_program, "perspective");
	GLint tex_uniform = glGetUniformLocation(shader_program, "tex");

	printf("GL version: %s\n", glGetString(GL_VERSION));
    printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glViewport(0, 0, 640, 480);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	glm::vec3 cam_pos = glm::vec3(0.0, 0.0, 0.0);
	glm::vec3 cam_front = glm::vec3(0.0, 0.0, -1.0);
	glm::vec3 cam_up = glm::vec3(0.0, 1.0, 0.0);

	u8 running = 1;
	while (running) {
    	SDL_Event event;

		f32 cam_speed = 0.05;
		SDL_PumpEvents();
		const u8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_UP]) {
			cam_pos += cam_speed * cam_front;
		}
		if (state[SDL_SCANCODE_DOWN]) {
			cam_pos -= cam_speed * cam_front;
		}
		if (state[SDL_SCANCODE_LEFT]) {
			cam_pos -= glm::normalize(glm::cross(cam_front, cam_up)) * cam_speed;
		}
		if (state[SDL_SCANCODE_RIGHT]) {
			cam_pos += glm::normalize(glm::cross(cam_front, cam_up)) * cam_speed;
		}

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
				} break;
				case SDL_QUIT: {
					running = 0;
				} break;
			}
		}

#define PERSPECTIVE 0
#if PERSPECTIVE
		glm::mat4 perspective = glm::perspective(glm::radians(35.264f), ratio, 0.1f, 100.0f);
		double dist = sqrt(480.0 / 3.0);
		glm::mat4 view = glm::lookAt(glm::vec3(dist, dist, dist), cam_pos + cam_front, cam_up);
#else
		glm::mat4 perspective = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, -100.0f, 100.0f);

		glm::mat4 view = glm::mat4(1.0);
		view = glm::translate(view, glm::vec3(cam_pos.x, cam_pos.z, cam_pos.y));
		view = glm::scale(view, glm::vec3(1.0f / 14.0f, 1.0f / 14.0f, 1.0 / 14.0f));
		view = glm::rotate(view, glm::radians(40.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		view = glm::rotate(view, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
#endif

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shader_program);

		glUniformMatrix4fv(view_uniform, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(perspective_uniform, 1, GL_FALSE, &perspective[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glUniform1i(tex_uniform, 0);
		glBindTexture(GL_TEXTURE_2D, wood_tex);

		glEnableVertexAttribArray(points_attr);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_points);
		glVertexAttribPointer(points_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(normals_attr);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_normals);
		glVertexAttribPointer(normals_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(texpos_attr);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_tex_coords);
		glVertexAttribPointer(texpos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
		i32 size;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

		glm::mat4 start_model = glm::mat4(1.0);
		for (u32 x = 0; x < 20; x++) {
			for (u32 y = 0; y < 20; y++) {
				glm::mat4 model = glm::translate(start_model, glm::vec3((f32)x * 2.0f, 0.0f, (f32)y * 2.0));
				glUniformMatrix4fv(cube_model_uniform, 1, GL_FALSE, &model[0][0]);
				glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
			}
		}

		glDisableVertexAttribArray(points_attr);
		glDisableVertexAttribArray(normals_attr);
		glDisableVertexAttribArray(texpos_attr);

		SDL_GL_SwapWindow(window);
	}

	SDL_Quit();

	return 0;
}
