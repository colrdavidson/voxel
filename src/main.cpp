#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#include "common.h"
#include "point.h"
#include "cube.h"
#include "tga.h"
#include "gl_helper.h"

u32 chunk_width = 16;
u32 chunk_height = 256;
u32 chunk_depth = 16;
u32 chunk_size = chunk_width * chunk_height * chunk_depth;

u32 num_x_chunks = 21;
u32 num_y_chunks = 21;
u32 num_chunks = num_x_chunks * num_y_chunks;

typedef struct Chunk {
	u8 *pre_render_list;
	u8 *real_blocks;

	u32 *mappings;
	glm::vec3 *positions;
	glm::vec3 *colors;
	u8 *ao_bits;

	u64 num_blocks;
	u32 x_off;
	u32 z_off;
} Chunk;

Chunk *generate_chunk(u32 x_off, u32 z_off) {
	Chunk *chunk = (Chunk *)malloc(sizeof(Chunk));

	chunk->positions = (glm::vec3 *)malloc(sizeof(glm::vec3) * chunk_size);
	chunk->colors = (glm::vec3 *)malloc(sizeof(glm::vec3) * chunk_size);
	chunk->mappings = (u32 *)malloc(sizeof(u32) * chunk_size);
	chunk->pre_render_list = (u8 *)malloc(chunk_size);
	chunk->x_off = x_off * chunk_width;
	chunk->z_off = z_off * chunk_depth;

	u8 *height_map = (u8 *)malloc(chunk_width * chunk_depth);
	memset(height_map, 0, sizeof(chunk_width * chunk_depth));

	f32 min_height = chunk_height / 5;
	f32 avg_height = chunk_height / 2;
	for (u32 x = 0; x < chunk_width; x++) {
		for (u32 z = 0; z < chunk_depth; z++) {

			f32 column_height = avg_height;
			for (u8 o = 5; o < 8; o++) {
				f32 scale = (f32)(2 << o) * 1.01f;
				column_height += (f32)(o << 4) * stb_perlin_noise3((f32)(x + chunk->x_off) / scale, (f32)(z + chunk->z_off) / scale, o * 2.0f, 256, 256, 256);
			}

			if (column_height > chunk_height) {
				column_height = chunk_height;
			}

			if (column_height < min_height) {
				column_height = min_height;
			}

			height_map[twod_to_oned(x, z, chunk_width)] = column_height;
		}
	}

	chunk->real_blocks = height_map;
	return chunk;
}


glm::vec3 random_color() {
	f32 r = ((f32)(rand() % 10)) / 10;
	f32 g = ((f32)(rand() % 10)) / 10;
	f32 b = ((f32)(rand() % 10)) / 10;

	glm::vec3 color = glm::vec3(r, g, b);
	return color;
}

bool inside_chunk(u32 x, u32 y, u32 z) {
	if (x < chunk_width && y < chunk_height && z < chunk_depth) {
		return true;
	}

	return false;
}

void hull_chunk(Chunk **chunks, u32 chunk_idx) {
	Chunk *chunk = chunks[chunk_idx];
	memset(chunk->pre_render_list, 0, chunk_size);

	for (u64 i = 0; i < chunk_width * chunk_depth; i++) {
		Point p = oned_to_twod(i, chunk_width);

		if (p.x > 0 && p.x < (chunk_width - 1) && p.y > 0 && p.y < (chunk_depth - 1)) {
			//B
			if (chunk->real_blocks[i] + 1 < chunk->real_blocks[twod_to_oned(p.x - 1, p.y, chunk_width)]) {
				for (u32 dy = chunk->real_blocks[i] + 1; dy < chunk->real_blocks[twod_to_oned(p.x - 1, p.y, chunk_width)]; dy++) {
					chunk->pre_render_list[threed_to_oned(p.x - 1, dy, p.y, chunk_width, chunk_height)] = 3;
				}
			}
			//GB
			if (chunk->real_blocks[i] + 1 < chunk->real_blocks[twod_to_oned(p.x + 1, p.y, chunk_width)]) {
				//printf("(%d, %d, %d) | %d < (%d, %d, %d) | %d { delta: %d}\n", p.x, p.y, p.z, chunk->real_blocks[i], p.x+1, p.y, p.z, chunk->real_blocks[twod_to_oned(p.x + 1, p.y, chunk_width)], chunk->real_blocks[twod_to_oned(p.x + 1, p.y, chunk_width)] - chunk->real_blocks[i]);
				for (u32 dy = chunk->real_blocks[i] + 1; dy < chunk->real_blocks[twod_to_oned(p.x + 1, p.y, chunk_width)]; dy++) {
					chunk->pre_render_list[threed_to_oned(p.x + 1, dy, p.y, chunk_width, chunk_height)] = 2;
				}
			}
			//RB
			if (chunk->real_blocks[i] + 1 < chunk->real_blocks[twod_to_oned(p.x, p.y + 1, chunk_width)]) {
				for (u32 dy = chunk->real_blocks[i] + 1; dy < chunk->real_blocks[twod_to_oned(p.x, p.y + 1, chunk_width)]; dy++) {
					chunk->pre_render_list[threed_to_oned(p.x, dy, p.y + 1, chunk_width, chunk_height)] = 4;
				}
			}
			//R
			if (chunk->real_blocks[i] + 1 < chunk->real_blocks[twod_to_oned(p.x, p.y - 1, chunk_width)]) {
				for (u32 dy = chunk->real_blocks[i] + 1; dy < chunk->real_blocks[twod_to_oned(p.x, p.y - 1, chunk_width)]; dy++) {
					chunk->pre_render_list[threed_to_oned(p.x, dy, p.y - 1, chunk_width, chunk_height)] = 5;
				}
			}
		} else {
			Point cp = oned_to_twod(chunk_idx, num_x_chunks);
			if (p.x == chunk_width - 1 && cp.x < (num_x_chunks - 1)) {
				Chunk *other_chunk = chunks[threed_to_oned(cp.x + 1, cp.y, cp.z, num_x_chunks, num_y_chunks)];
				if (chunk->real_blocks[i] < other_chunk->real_blocks[twod_to_oned(0, p.y, chunk_width)]) {
					for (u32 dy = chunk->real_blocks[i]; dy < other_chunk->real_blocks[twod_to_oned(0, p.y, chunk_width)]; dy++) {
						chunk->pre_render_list[threed_to_oned(p.x, dy, p.y, chunk_width, chunk_height)] = 6;
					}
				}
			}
			if (p.x == 0 && cp.x > 0) {
				Chunk *other_chunk = chunks[threed_to_oned(cp.x - 1, cp.y, cp.z, num_x_chunks, num_y_chunks)];
				if (chunk->real_blocks[i] + 1 < other_chunk->real_blocks[twod_to_oned(chunk_width - 1, p.y, chunk_width)]) {
					for (u32 dy = chunk->real_blocks[i] + 1; dy < other_chunk->real_blocks[twod_to_oned(chunk_width - 1, p.y, chunk_width)]; dy++) {
						chunk->pre_render_list[threed_to_oned(p.x, dy, p.y, chunk_width, chunk_height)] = 7;
					}
				}
			}
			if (p.y == chunk_width - 1 && cp.y < (num_y_chunks - 1)) {
				Chunk *other_chunk = chunks[threed_to_oned(cp.x, cp.y + 1, cp.z, num_x_chunks, num_y_chunks)];
				if (chunk->real_blocks[i] + 1 < other_chunk->real_blocks[twod_to_oned(p.x, 0, chunk_width)]) {
					for (u32 dy = chunk->real_blocks[i] + 1; dy < other_chunk->real_blocks[twod_to_oned(p.x, 0, chunk_width)]; dy++) {
						chunk->pre_render_list[threed_to_oned(p.x, dy, p.y, chunk_width, chunk_height)] = 8;
					}
				}
			}
			if (p.y == 0 && cp.y > 0) {
				Chunk *other_chunk = chunks[threed_to_oned(cp.x, cp.y - 1, cp.z, num_x_chunks, num_y_chunks)];
				if (chunk->real_blocks[i] + 1 < other_chunk->real_blocks[twod_to_oned(p.x, chunk_depth - 1, chunk_width)]) {
					for (u32 dy = chunk->real_blocks[i] + 1; dy < other_chunk->real_blocks[twod_to_oned(p.x, chunk_depth - 1, chunk_width)]; dy++) {
						chunk->pre_render_list[threed_to_oned(p.x, dy, p.y, chunk_width, chunk_height)] = 9;
					}
				}
			}

		}

		chunk->pre_render_list[threed_to_oned(p.x, chunk->real_blocks[i], p.y, chunk_width, chunk_height)] = 1;
	}
}

void update_chunk(Chunk **chunks, u32 chunk_idx) {
	Chunk *chunk = chunks[chunk_idx];

	u32 tile_index = 0;
	for (u64 i = 0; i < chunk_size; i++) {
		u32 tile_id = chunk->pre_render_list[i];
		if (tile_id != 0) {
			Point p = oned_to_threed(i, chunk_width, chunk_height);

			switch (chunk->pre_render_list[i]) {
				case 1: {
					//green
					chunk->colors[tile_index] = glm::vec3(0.0, 1.0, 0.0);
				} break;
				case 2: {
					//blue
					chunk->colors[tile_index] = glm::vec3(0.0, 0.0, 1.0);
				} break;
				case 3: {
					//GB
					chunk->colors[tile_index] = glm::vec3(1.0, 0.645, 0.0);
				} break;
				case 4: {
					//RB
					chunk->colors[tile_index] = glm::vec3(1.0, 0.0, 1.0);
				} break;
				case 5: {
					//red
					chunk->colors[tile_index] = glm::vec3(1.0, 0.0, 0.0);
				} break;
				case 6: {
					//red
					chunk->colors[tile_index] = glm::vec3(0.2, 0.5, 0.7);
				} break;
				case 7: {
					//red
					chunk->colors[tile_index] = glm::vec3(0.5, 0.5, 0.5);
				} break;
				case 8: {
					//red
					chunk->colors[tile_index] = glm::vec3(1.0, 1.0, 1.0);
				} break;
				case 9: {
					//red
					chunk->colors[tile_index] = glm::vec3(0.9, 0.2, 0.5);
				} break;
			}

			glm::vec3 m = glm::vec3(p.x + chunk->x_off, p.y, p.z + chunk->z_off);
			chunk->positions[tile_index] = m;
			chunk->mappings[i] = tile_index;

			tile_index++;
		}
	}

	chunk->num_blocks = tile_index;
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	i32 screen_width = 640;
	i32 screen_height = 480;

	SDL_Window *window = SDL_CreateWindow("Voxel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
    SDL_GL_GetDrawableSize(window, &screen_width, &screen_height);

	printf("GL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	srand(time(NULL));

	GLuint obj_shader_program = load_and_build_program("src/obj_vert.vsh", "src/obj_frag.fsh");

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo_cube_points;
	glGenBuffers(1, &vbo_cube_points);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_points);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_points), cube_points, GL_STATIC_DRAW);

	GLuint ibo_cube_indices;
	glGenBuffers(1, &ibo_cube_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

	GLuint points_attr = glGetAttribLocation(obj_shader_program, "coords");
	GLuint tile_color_attr = glGetAttribLocation(obj_shader_program, "color");
	GLuint model_attr = glGetAttribLocation(obj_shader_program, "model");

	GLuint vbo_rect_points;
	glGenBuffers(1, &vbo_rect_points);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_rect_points);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rect_points), rect_points, GL_STATIC_DRAW);

	GLuint ibo_rect_indices;
	glGenBuffers(1, &ibo_rect_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_rect_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rect_indices), rect_indices, GL_STATIC_DRAW);

	GLuint pv_uniform = glGetUniformLocation(obj_shader_program, "pv");

	glViewport(0, 0, screen_width, screen_height);

	u32 start_time = SDL_GetTicks();

	Chunk **chunks = (Chunk **)malloc(sizeof(Chunk *) * num_chunks);
	for (u32 x = 0; x < num_x_chunks; x++) {
		for (u32 y = 0; y < num_y_chunks; y++) {
			Chunk *chunk = generate_chunk(x, y);
			chunks[twod_to_oned(x, y, num_x_chunks)] = chunk;
		}
	}

	u32 block_load = 0;
	for (u32 i = 0; i < num_chunks; i++) {
		hull_chunk(chunks, i);
		update_chunk(chunks, i);
		block_load += chunks[i]->num_blocks;
	}

	u32 end_time = SDL_GetTicks();
	printf("%u blocks in %u ms, %f bps\n", block_load, end_time - start_time, (f64)block_load / (f64)((end_time - start_time) / 1000.0f));

	Point hovered = new_point(0, 0, 0);

	GLuint vbo_model;
	glGenBuffers(1, &vbo_model);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_model);

	GLuint vbo_tile_color;
	glGenBuffers(1, &vbo_tile_color);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color);

	f32 current_time = (f32)SDL_GetTicks() / 60.0;
	f32 t = 0.0;

	glm::vec3 camera_pos = glm::vec3(chunk_width / 2, chunk_height + 3.0, chunk_depth / 2);
	glm::vec3 camera_front = glm::vec3(0.0, 0.0, 1.0);
	glm::vec3 camera_up = glm::vec3(0.0, 1.0, 0.0);

	f32 yaw = 0.0f;
	f32 pitch = 0.0f;

	glEnable(GL_CULL_FACE);

	u8 warped = false;
	u8 warp = false;
	bool clicked = false;

	u8 running = true;
	while (running) {
		SDL_Event event;

		f32 new_time = (f32)SDL_GetTicks() / 60.0;
		f32 dt = new_time - current_time;
		current_time = new_time;
		t += dt;

		f32 saved_y = camera_pos.y;
		f32 cam_speed = 1.5;
		SDL_PumpEvents();
		const u8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_W]) {
			camera_pos += cam_speed * camera_front * dt;
		}
		if (state[SDL_SCANCODE_S]) {
			camera_pos -= cam_speed * camera_front * dt;
		}
		if (state[SDL_SCANCODE_A]) {
			camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * cam_speed * dt;
		}
		if (state[SDL_SCANCODE_D]) {
			camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * cam_speed * dt;
		}

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE: {
							warp = false;
							SDL_SetRelativeMouseMode(SDL_FALSE);
						} break;
					}
				} break;
				case SDL_MOUSEMOTION: {
					if (!warped) {
						i32 mouse_x, mouse_y;
						SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

						if (warp) {
							SDL_WarpMouseInWindow(window, screen_width / 2, screen_height / 2);
							warped = true;
						} else {
							continue;
						}

						f32 x_off = mouse_x;
						f32 y_off = -mouse_y;

						f32 mouse_speed = 0.20;
						x_off *= mouse_speed;
						y_off *= mouse_speed;

						yaw += x_off;
						pitch += y_off;

						if (pitch > 89.0f)
							pitch = 89.0f;
						if (pitch < -89.0f)
							pitch = -89.0f;

						glm::vec3 front = glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)), sin(glm::radians(yaw)) * cos(glm::radians(pitch)));

						clicked = true;
						camera_front = front;
					} else {
						warped = false;
					}
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					i32 mouse_x, mouse_y;
					u32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
					SDL_SetRelativeMouseMode(SDL_TRUE);
					warp = true;

					if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
					} else if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
					}
				} break;
				case SDL_QUIT: {
					running = 0;
				} break;
			}
		}

		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glUseProgram(obj_shader_program);

		glEnableVertexAttribArray(points_attr);
		glEnableVertexAttribArray(tile_color_attr);
		glEnableVertexAttribArray(model_attr);

		i32 size;

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_points));
		GL_CHECK(glVertexAttribPointer(points_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));

		GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices));
		GL_CHECK(glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color));
		GL_CHECK(glVertexAttribPointer(tile_color_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));
		GL_CHECK(glVertexAttribDivisor(tile_color_attr, 1));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_model));
		GL_CHECK(glVertexAttribPointer(model_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));
		GL_CHECK(glVertexAttribDivisor(model_attr, 1));

		glm::mat4 perspective;
		perspective = glm::perspective(glm::radians(45.0f), (f32)screen_width / (f32)screen_height, 0.1f, 5000.0f);
		glm::mat4 view;
		view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
		glm::mat4 pv = perspective * view;
		glUniformMatrix4fv(pv_uniform, 1, GL_FALSE, &pv[0][0]);

		for (u32 i = 0; i < num_chunks; i++) {
			glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * chunks[i]->num_blocks, chunks[i]->colors, GL_STREAM_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * chunks[i]->num_blocks, chunks[i]->positions, GL_STREAM_DRAW);

			GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0, chunks[i]->num_blocks));
		}

		glDisable(GL_DEPTH_TEST);

        chunks[0]->colors[0] = glm::vec3(1.0, 1.0, 1.0);
		pv = glm::ortho(-66.5f, 66.5f, -37.6f, 37.6f, -1.0f, 1.0f);

		chunks[0]->positions[0] = glm::vec3(0.1, 0.0, 0.0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), chunks[0]->colors, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), chunks[0]->positions, GL_STREAM_DRAW);

		glUniformMatrix4fv(pv_uniform, 1, GL_FALSE, &pv[0][0]);
		GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0, 1));

		SDL_GL_SwapWindow(window);
	}

	SDL_Quit();

	return 0;
}
