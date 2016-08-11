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
#include "tga.h"

#define RELEASE 0
#if RELEASE
	#define GL_CHECK(x) x
#else
	#define GL_CHECK(x) do { x; GLenum err = glGetError(); assert(err == GL_NO_ERROR); } while(0)
#endif

typedef struct Attribs {
	GLuint points_attr;
	GLuint texpos_attr;
	GLuint normals_attr;
} Attribs;

Attribs *init_attribs(GLuint shader_program, const char *s_points_attr, const char *s_texpos_attr, const char *s_normals_attr) {
	Attribs *a = (Attribs *)malloc(sizeof(Attribs));

	a->points_attr = glGetAttribLocation(shader_program, s_points_attr);
	a->texpos_attr = glGetAttribLocation(shader_program, s_texpos_attr);
	a->normals_attr = glGetAttribLocation(shader_program, s_normals_attr);

	return a;
}

void enable_attribs(Attribs *attribs) {
	glEnableVertexAttribArray(attribs->points_attr);
	glEnableVertexAttribArray(attribs->texpos_attr);
	glEnableVertexAttribArray(attribs->normals_attr);
}

void disable_attribs(Attribs *attribs) {
	glDisableVertexAttribArray(attribs->points_attr);
	glDisableVertexAttribArray(attribs->normals_attr);
	glDisableVertexAttribArray(attribs->texpos_attr);
}

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

GLuint load_and_build_program(const char *vert_filename, const char *frag_filename) {
	GLuint shader_program = glCreateProgram();

	char *vert_file = file_to_string(vert_filename);
	char *frag_file = file_to_string(frag_filename);

	GLint vert_shader = build_shader(vert_file, GL_VERTEX_SHADER);
	GLint frag_shader = build_shader(frag_file, GL_FRAGMENT_SHADER);

	free(vert_file);
	free(frag_file);

	GL_CHECK(glAttachShader(shader_program, vert_shader));
	GL_CHECK(glAttachShader(shader_program, frag_shader));

	GL_CHECK(glLinkProgram(shader_program));

	return shader_program;
}

u8 *load_map(const char *map_file) {
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

GLuint build_texture(const char *tex_filename) {
	GLuint tex;

	SDL_Surface *surf = IMG_Load(tex_filename);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);

	SDL_FreeSurface(surf);
	return tex;
}

GLuint build_vbo(GLfloat *arr, u64 size) {
	GLuint vbo = 0;

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size, arr, GL_STATIC_DRAW);

	return vbo;
}

GLuint build_ibo(GLushort *arr, u64 size) {
	GLuint ibo = 0;

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, arr, GL_STATIC_DRAW);

	return ibo;
}

void setup_object(Attribs *attribs, GLuint points, GLuint tex, GLuint normals, GLuint tex_coords, GLuint indices, i32 *size) {
	glBindBuffer(GL_ARRAY_BUFFER, points);
	glVertexAttribPointer(attribs->points_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindTexture(GL_TEXTURE_2D, tex);

	glBindBuffer(GL_ARRAY_BUFFER, normals);
	glVertexAttribPointer(attribs->normals_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, tex_coords);
	glVertexAttribPointer(attribs->texpos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, size);
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_Window *window = SDL_CreateWindow("Voxel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	printf("GL version: %s\n", glGetString(GL_VERSION));
    printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	GLuint shader_program = load_and_build_program("src/vert.vsh", "src/frag.fsh");

    GLuint frame_buffer = 0;
    GLuint depth_buffer = 0;
	GLuint render_buffer = 0;
	GLuint click_buffer = 0;

	GL_CHECK(glGenFramebuffers(1, &frame_buffer));

	GL_CHECK(glGenRenderbuffers(1, &depth_buffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer));
	GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 640, 480));

	GL_CHECK(glGenRenderbuffers(1, &render_buffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, render_buffer));
	GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 640, 480));

	GL_CHECK(glGenRenderbuffers(1, &click_buffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, click_buffer));
	GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_R32I, 640, 480));

	GLint color_index = glGetFragDataLocation(shader_program, "color");
	GLint click_index = glGetFragDataLocation(shader_program, "click");

	GLenum buffers[2];
	buffers[color_index] = GL_COLOR_ATTACHMENT0;
	buffers[click_index] = GL_COLOR_ATTACHMENT1;

	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer));
	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buffer));
	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, click_buffer));
	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer));
	GL_CHECK(glDrawBuffers(2, buffers));

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		return 1;
	}

	GLuint wall_tex = build_texture("assets/wall.png");
	GLuint roof_tex = build_texture("assets/roof.png");
	GLuint ladder_tex = build_texture("assets/ladder.png");
	GLuint door_tex = build_texture("assets/door.png");
	GLuint tree_tex = build_texture("assets/tree.png");
	GLuint wood_tex = build_texture("assets/wood.png");
	GLuint grass_tex = build_texture("assets/grass.png");

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	f32 ratio = 640.0 / 480.0;

	GLuint vbo_cube_points = build_vbo(cube_points, sizeof(cube_points));
	GLuint vbo_roof_points = build_vbo(roof_points, sizeof(roof_points));
	GLuint vbo_door_points = build_vbo(door_points, sizeof(door_points));
	GLuint vbo_tree_points = build_vbo(tree_points, sizeof(tree_points));

	GLuint vbo_cube_tex_coords = build_vbo(cube_tex_coords, sizeof(cube_tex_coords));
	GLuint vbo_tree_tex_coords = build_vbo(tree_tex_coords, sizeof(tree_tex_coords));

	GLuint vbo_cube_normals = build_vbo(cube_normals, sizeof(cube_normals));
	GLuint vbo_tree_normals = build_vbo(tree_normals, sizeof(tree_normals));

	GLuint ibo_cube_indices = build_ibo(cube_indices, sizeof(cube_indices));
	GLuint ibo_tree_indices = build_ibo(tree_indices, sizeof(tree_indices));

	Attribs *attribs = init_attribs(shader_program, "coords", "tex_coords", "normals");

	GLuint mvp_uniform = glGetUniformLocation(shader_program, "mvp");
	GLuint tile_data_uniform = glGetUniformLocation(shader_program, "tile_data");
	GLint tex_uniform = glGetUniformLocation(shader_program, "tex");

	glViewport(0, 0, 640, 480);
	glEnable(GL_DEPTH_TEST);

	glm::vec3 cam_pos = glm::vec3(0.0, 0.0, -50.0);
	glm::vec3 cam_front = glm::vec3(0.0, 0.0, -1.0);
	glm::vec3 cam_up = glm::vec3(0.0, 1.0, 0.0);

	u8 *map = load_map("assets/house_map");
	u8 map_width = 20;
	u8 map_height = 20;
	u8 map_depth = 4;
	Direction direction = NORTH;

	f32 scale = 25.0f;
	u8 persp = true;
	u8 running = true;
	while (running) {
    	SDL_Event event;

		f32 cam_speed = 0.15;
		SDL_PumpEvents();
		const u8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_W]) {
			cam_pos.y += cam_speed;
		}
		if (state[SDL_SCANCODE_S]) {
			cam_pos.y -= cam_speed;
		}
		if (state[SDL_SCANCODE_A]) {
			cam_pos.x -= cam_speed;
		}
		if (state[SDL_SCANCODE_D]) {
			cam_pos.x += cam_speed;
		}
		if (state[SDL_SCANCODE_LSHIFT]) {
			cam_pos.z += cam_speed;
			scale += cam_speed;
		}
		if (state[SDL_SCANCODE_LCTRL]) {
			cam_pos.z -= cam_speed;
			scale -= cam_speed;
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
						case SDLK_x: {
                    		persp = !persp;
						} break;
					}
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					i32 mouse_x, mouse_y;
					u32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

					if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                        i32 data = 0;

						glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
						glReadBuffer(GL_COLOR_ATTACHMENT1);
						glReadPixels(mouse_x, 480 - mouse_y, 1, 1, GL_RED_INTEGER, GL_INT, &data);

						Point p = oned_to_threed(data, map_width, map_height);
						printf("(%d, %d) -> (%d, %d, %d)\n", mouse_x, mouse_y, p.x, p.y, p.z);
					} else {
					}
				} break;
				case SDL_QUIT: {
					running = 0;
				} break;
			}
		}

		f32 screen_width = 640.0f;
		f32 screen_height = 480.0f;

 		glm::mat4 perspective;
		if (persp) {
			perspective = glm::ortho(-screen_width / scale, screen_width / scale, -screen_height / scale, screen_height / scale, -200.0f, 200.0f);
		} else {
			perspective = glm::perspective(glm::radians(40.0f), screen_width / screen_height, 0.1f, 500.0f);
		}

		glm::mat4 view;
		view = glm::translate(view, glm::vec3(cam_pos.x, cam_pos.y, cam_pos.z));
		view = glm::rotate(view, glm::radians(35.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		switch (direction) {
			case NORTH: {
				view = glm::rotate(view, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	  		} break;
			case SOUTH: {
				view = glm::rotate(view, glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader_program);

		enable_attribs(attribs);

		i32 size;
		glm::mat4 start_model = glm::mat4(1.0);

		for (u32 i = 0; i < map_width * map_height * map_depth; i++) {
			u32 tile_id = map[i];
			if (tile_id != 0) {
				glActiveTexture(GL_TEXTURE0);
				glUniform1i(tex_uniform, 0);

				Point p = oned_to_threed(i, map_width, map_height);
				glm::mat4 model = glm::translate(start_model, glm::vec3(p.x * 2.0f - map_width, p.z * 2.0f, p.y * 2.0f - map_height));

				switch (tile_id) {
					case 1: {
						setup_object(attribs, vbo_cube_points, wall_tex, vbo_cube_normals, vbo_cube_tex_coords, ibo_cube_indices, &size);
					} break;
					case 2: {
						setup_object(attribs, vbo_cube_points, grass_tex, vbo_cube_normals, vbo_cube_tex_coords, ibo_cube_indices, &size);
					} break;
					case 4: {
						setup_object(attribs, vbo_cube_points, wood_tex, vbo_cube_normals, vbo_cube_tex_coords, ibo_cube_indices, &size);
					} break;
					case 5: {
						setup_object(attribs, vbo_door_points, door_tex, vbo_cube_normals, vbo_cube_tex_coords, ibo_cube_indices, &size);
					} break;
					case 6: {
						setup_object(attribs, vbo_roof_points, roof_tex, vbo_cube_normals, vbo_cube_tex_coords, ibo_cube_indices, &size);
					} break;
					case 7: {
						setup_object(attribs, vbo_cube_points, ladder_tex, vbo_cube_normals, vbo_cube_tex_coords, ibo_cube_indices, &size);
					} break;
					case 8: {
						setup_object(attribs, vbo_tree_points, tree_tex, vbo_tree_normals, vbo_tree_tex_coords, ibo_tree_indices, &size);
					} break;
					default: {
						continue;
					} break;
				}

				glm::mat4 mvp = perspective * view * model;
				glUniformMatrix4fv(mvp_uniform, 1, GL_FALSE, &mvp[0][0]);
				glUniform1i(tile_data_uniform, i);
				glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
			}
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, 640, 480, 0, 0, 640, 480, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		disable_attribs(attribs);

		SDL_GL_SwapWindow(window);
	}

	SDL_Quit();

	return 0;
}
