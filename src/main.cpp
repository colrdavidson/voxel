#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "point.h"
#include "cube.h"

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

u8 *load_map(char *map_file) {
	FILE *fp = fopen("assets/house_map", "r");
	char *line = (char *)malloc(256);

	fgets(line, 256, fp);

	u32 map_width = atoi(strtok(line, " "));
	u32 map_height = atoi(strtok(NULL, " "));
	u32 map_depth = atoi(strtok(NULL, " "));
	u64 map_size = map_width * map_height * map_depth;

	printf("map_size: %lu\n", map_size);
	u8 *map = (u8 *)malloc(map_size * sizeof(u8));
	memset(map, 0, map_size);

	fgets(line, 256, fp);
	fgets(line, 256, fp);

	u32 tile_entries = atoi(line) + 3;
	for (u32 i = 1; i < tile_entries - 2; i++) {
		fgets(line, 256, fp);
	}

	fgets(line, 256, fp);
	fgets(line, 256, fp);

	char *line_start = line;
	char *rest;
	for (u32 z = 0; z < map_depth; z++) {
		for (u32 y = 0; y < map_height; y++) {
			bool new_line = true;
			for (u32 x = 0; x < map_width; x++) {
				char *bit;

				if (new_line) {
					bit = strtok_r(line_start, " ", &rest);
					new_line = false;
				} else {
					bit = strtok_r(NULL, " ", &rest);
				}

				u32 tile_id = atoi(bit);
				map[threed_to_oned(x, y, z, map_width, map_height)] = tile_id;
			}
			fgets(line, 256, fp);
		}
		fgets(line, 256, fp);
	}

	/*for (u32 z = 0; z < map_depth; z++) {
		for (u32 y = 0; y < map_height; y++) {
			for (u32 x = 0; x < map_width; x++) {
				printf("%d ", map[threed_to_oned(x, y, z, map_width, map_height)]);
			}
			printf("\n");
		}
		printf("\n");
	}*/

	fclose(fp);
	return map;
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

	GLuint wall_tex = 0;
	GLuint wood_tex = 0;
	GLuint grass_tex = 0;

	SDL_Surface *wall_surf = IMG_Load("assets/wall.png");
	SDL_Surface *wood_surf = IMG_Load("assets/wood.png");
	SDL_Surface *grass_surf = IMG_Load("assets/grass.png");

	glGenTextures(1, &wall_tex);
	glBindTexture(GL_TEXTURE_2D, wall_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wall_surf->w, wall_surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, wall_surf->pixels);

	glGenTextures(1, &wood_tex);
	glBindTexture(GL_TEXTURE_2D, wood_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wood_surf->w, wood_surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, wood_surf->pixels);

	glGenTextures(1, &grass_tex);
	glBindTexture(GL_TEXTURE_2D, grass_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, grass_surf->w, grass_surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, grass_surf->pixels);

	SDL_FreeSurface(wall_surf);
	SDL_FreeSurface(wood_surf);
	SDL_FreeSurface(grass_surf);

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	f32 ratio = 640.0 / 480.0;

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

	puts("here");
	u8 *map = load_map("assets/house_map");
	Direction direction = NORTH;

	u8 running = 1;
	while (running) {
    	SDL_Event event;

		f32 cam_speed = 0.15;
		SDL_PumpEvents();
		const u8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_UP]) {
			cam_pos += cam_speed * cam_front;
		}
		if (state[SDL_SCANCODE_DOWN]) {
			cam_pos -= cam_speed * cam_front;
		}
		if (state[SDL_SCANCODE_LEFT]) {
			cam_pos -= glm::cross(cam_front, cam_up) * cam_speed;
		}
		if (state[SDL_SCANCODE_RIGHT]) {
			cam_pos += glm::cross(cam_front, cam_up) * cam_speed;
		}

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_e: {
							direction = cycle_right(direction);
						} break;
						case SDLK_q: {
							direction = cycle_left(direction);
						} break;
					}
				} break;
				case SDL_QUIT: {
					running = 0;
				} break;
			}
		}

#define PERSPECTIVE 0
#if PERSPECTIVE
		glm::mat4 perspective = glm::perspective(glm::radians(35.264f), ratio, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(glm::vec3(-10.0, 10.0, 0.0), cam_pos + cam_front, cam_up);
#else
		glm::mat4 perspective = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, -100.0f, 100.0f);

		glm::mat4 view = glm::mat4(1.0);
		view = glm::rotate(view, glm::radians(40.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		switch (direction) {
			case NORTH: {
				view = glm::rotate(view, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	  		} break;
			case SOUTH: {
				view = glm::rotate(view, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	  		} break;
			case WEST: {
				view = glm::rotate(view, glm::radians(-135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	  		} break;
			case EAST: {
				view = glm::rotate(view, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	  		} break;
			default: {
				view = glm::rotate(view, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			}
		}

		view = glm::translate(view, glm::vec3(cam_pos.x, cam_pos.y, cam_pos.z));
		view = glm::scale(view, glm::vec3(1.0f / 16.0f, 1.0f / 16.0f, 1.0 / 16.0f));

#endif

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shader_program);

		glUniformMatrix4fv(view_uniform, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(perspective_uniform, 1, GL_FALSE, &perspective[0][0]);

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

		u8 map_width = 20;
		u8 map_height = 20;
		u8 map_depth = 4;

		for (u32 i = 0; i < map_width * map_height * map_depth; i++) {
			u32 tile_id = map[i];
			if (tile_id != 0) {
				glActiveTexture(GL_TEXTURE0);
				glUniform1i(tex_uniform, 0);

				switch (tile_id) {
					case 1: {
						glBindTexture(GL_TEXTURE_2D, wall_tex);
					} break;
					case 2: {
						glBindTexture(GL_TEXTURE_2D, grass_tex);
					} break;
					case 4: {
						glBindTexture(GL_TEXTURE_2D, wood_tex);
					} break;
					default: {
						glBindTexture(GL_TEXTURE_2D, wall_tex);
					} break;
				}

				Point p = oned_to_threed(i, map_width, map_height);

				glm::mat4 model = glm::translate(start_model, glm::vec3((f32)p.x * 2.0f, (f32)p.z * 2.0f, (f32)p.y * 2.0f));
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
