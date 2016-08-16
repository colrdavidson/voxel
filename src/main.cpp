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

u8 *generate_map(u32 map_width, u32 map_height, u32 map_depth) {
    u64 map_size = map_width * map_height * map_depth;

	u8 *map = (u8 *)malloc(map_size);
	memset(map, 0, map_size);

	u8 *height_map = (u8 *)malloc(map_width * map_depth);

	f32 min_height = map_height / 5;
	f32 avg_height = map_height / 2;
	for (u32 x = 0; x < map_width; x++) {
		for (u32 z = 0; z < map_depth; z++) {

			f32 column_height = avg_height;
			for (u8 o = 5; o < 8; o++) {
				f32 scale = (f32)(2 << o) * 1.01f;
				column_height += (f32)(o << 3) * stb_perlin_noise3((f32)x / scale, (f32)z / scale, o * 2.0f, 256, 256, 256);
			}

			if (column_height > map_height) {
				column_height = map_height;
			}

			if (column_height < min_height) {
				column_height = min_height;
			}

			height_map[twod_to_oned(x, z, map_width)] = column_height;
		}
	}

	/*Image img;
	img.width = map_width;
	img.height = map_depth;
	img.data = height_map;
	write_tga_bitmap("test.tga", &img);*/

	for (u64 x = 0; x < map_width; x++) {
		for (u64 z = 0; z < map_depth; z++) {
			for (u64 y = 0; y < height_map[twod_to_oned(x, z, map_width)]; y++) {
				if (y > (u64)((f32)map_height * 0.55)) {
					map[threed_to_oned(x, y, z, map_width, map_height)] = 1;
				} else {
					map[threed_to_oned(x, y, z, map_width, map_height)] = 2;
				}
			}
		}
	}

	return map;
}

typedef struct RenderableMap {
	u8 *blocks;
	u8 *ao_map;
	u64 num_blocks;
} RenderableMap;

glm::vec3 random_color() {
	f32 r = ((f32)(rand() % 10)) / 10;
	f32 g = ((f32)(rand() % 10)) / 10;
	f32 b = ((f32)(rand() % 10)) / 10;

	glm::vec3 color = glm::vec3(r, g, b);
	return color;
}

bool inside_map(u32 map_width, u32 map_height, u32 map_depth, u32 x, u32 y, u32 z) {
	if (x < map_width && y < map_height && z < map_depth) {
		return true;
	}

	return false;
}

bool adjacent_to_air(u8 *map, u32 map_width, u32 map_height, u32 map_depth, u32 i) {
	Point p = oned_to_threed(i, map_width, map_height);

	if (!(p.x == 0 || p.x == (map_width - 1))) {
		if (map[threed_to_oned(p.x + 1, p.y, p.z, map_width, map_height)] == 0 || map[threed_to_oned(p.x - 1, p.y, p.z, map_width, map_height)] == 0) {
			return true;
		}
	}


	if (!(p.y == 0 || p.y == (map_height - 1))) {
		if (map[threed_to_oned(p.x, p.y + 1, p.z, map_width, map_height)] == 0 || map[threed_to_oned(p.x, p.y - 1, p.z, map_width, map_height)] == 0) {
			return true;
		}
	}


	if (!(p.z == 0 || p.z == (map_depth - 1))) {
		if (map[threed_to_oned(p.x, p.y, p.z + 1, map_width, map_height)] == 0 || map[threed_to_oned(p.x, p.y, p.z - 1, map_width, map_height)] == 0) {
			return true;
		}
	}

	return false;
}



RenderableMap *hull_map(u8 *map, RenderableMap *r_map, u32 map_width, u32 map_height, u32 map_depth) {
	u32 start_time = SDL_GetTicks();
	u64 map_size = map_height * map_width * map_depth;

	if (r_map == NULL) {
		r_map = (RenderableMap *)malloc(sizeof(RenderableMap));
		r_map->blocks = (u8 *)malloc(map_size);
	}

	memset(r_map->blocks, 0, map_size);

	u64 block_count = 0;
	for (u64 i = 0; i < map_size; i++) {
        if (map[i] != 0 && adjacent_to_air(map, map_width, map_height, map_depth, i)) {
			r_map->blocks[i] = map[i];
			block_count += 1;
		}

	}

	r_map->num_blocks = block_count;

	u32 end_time = SDL_GetTicks();
	printf("%lu blocks in %u ms, %f bps\n", map_size, end_time - start_time, (f64)map_size / (f64)((end_time - start_time) / 1000.0f));
	return r_map;
}

void update_map(glm::vec3 *model, i32 *tile_data, glm::vec3 *colors, u32 *mappings, u32 map_width, u32 map_height, u32 map_depth, RenderableMap *r_map, u8 *map) {
	u64 map_size = map_height * map_width * map_depth;
	u32 tile_index = 0;
	for (u64 i = 0; i < map_size; i++) {
		u32 tile_id = r_map->blocks[i];
		if (tile_id != 0) {
			Point p = oned_to_threed(i, map_width, map_height);

			glm::vec3 m = glm::vec3(p.x, p.y, p.z);

			model[tile_index] = m;
			tile_data[tile_index] = i;

            if (map[i] == 1) {
				colors[tile_index] = glm::vec3(0.2, 0.5, 0.2);
			} else {
				colors[tile_index] = glm::vec3(0.0, 0.3, 0.0);
			}


			mappings[i] = tile_index;

			tile_index++;
		}
	}
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	i32 screen_width = 1280;
	i32 screen_height = 720;

	SDL_Window *window = SDL_CreateWindow("Voxel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);
    SDL_GL_GetDrawableSize(window, &screen_width, &screen_height);

	printf("GL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	srand(time(NULL));

	GLuint obj_shader_program = load_and_build_program("src/obj_vert.vsh", "src/obj_frag.fsh");

	GLuint frame_buffer = 0;
	GLuint depth_buffer = 0;
	GLuint render_buffer = 0;
	GLuint click_buffer = 0;

	GL_CHECK(glGenFramebuffers(1, &frame_buffer));

	GL_CHECK(glGenRenderbuffers(1, &depth_buffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer));
	GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screen_width, screen_height));

	GL_CHECK(glGenRenderbuffers(1, &render_buffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, render_buffer));
	GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, screen_width, screen_height));

	GL_CHECK(glGenRenderbuffers(1, &click_buffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, click_buffer));
	GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_R32I, screen_width, screen_height));

	GLint obj_color_index = glGetFragDataLocation(obj_shader_program, "color");
	GLint obj_click_index = glGetFragDataLocation(obj_shader_program, "click");

	GLenum obj_buffers[2];
	obj_buffers[obj_color_index] = GL_COLOR_ATTACHMENT0;
	obj_buffers[obj_click_index] = GL_COLOR_ATTACHMENT1;

	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer));
	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buffer));
	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, click_buffer));
	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer));

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		return 1;
	}

	GLuint vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo_cube_points;
	glGenBuffers(1, &vbo_cube_points);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_points);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_points), cube_points, GL_STATIC_DRAW);

	GLuint vbo_cube_normals;
	glGenBuffers(1, &vbo_cube_normals);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);

	GLuint ibo_cube_indices;
	glGenBuffers(1, &ibo_cube_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

	GLuint points_attr = glGetAttribLocation(obj_shader_program, "coords");
	GLuint normals_attr = glGetAttribLocation(obj_shader_program, "normals");
	GLuint tile_data_attr = glGetAttribLocation(obj_shader_program, "tile_data");
	GLuint tile_color_attr = glGetAttribLocation(obj_shader_program, "color");
	GLuint model_attr = glGetAttribLocation(obj_shader_program, "model");

	GLuint vbo_rect_points;
	glGenBuffers(1, &vbo_rect_points);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_rect_points);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rect_points), rect_points, GL_STATIC_DRAW);

	GLuint vbo_rect_normals;
	glGenBuffers(1, &vbo_rect_normals);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_rect_normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rect_normals), rect_normals, GL_STATIC_DRAW);

	GLuint ibo_rect_indices;
	glGenBuffers(1, &ibo_rect_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_rect_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rect_indices), rect_indices, GL_STATIC_DRAW);

	GLuint pv_uniform = glGetUniformLocation(obj_shader_program, "pv");

	glViewport(0, 0, screen_width, screen_height);

	u32 map_width = 16 * 21;
	u32 map_height = 256;
	u32 map_depth = 16 * 21;
	u64 map_size = map_width * map_height * map_depth;
	u8 *map = generate_map(map_width, map_height, map_depth);

	RenderableMap *r_map = hull_map(map, NULL, map_width, map_height, map_depth);

	glm::vec3 *model = (glm::vec3 *)malloc(sizeof(glm::vec3) * map_size);
	glm::vec3 *colors = (glm::vec3 *)malloc(sizeof(glm::vec3) * map_size);
	i32 *tile_data = (i32 *)malloc(sizeof(i32) * map_size);
	u32 *mappings = (u32 *)malloc(sizeof(u32) * map_size);

	Point hovered = new_point(0, 0, 0);
	update_map(model, tile_data, colors, mappings, map_width, map_height, map_depth, r_map, map);

	GLuint vbo_model;
	glGenBuffers(1, &vbo_model);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_model);

	GLuint vbo_tile_data;
	glGenBuffers(1, &vbo_tile_data);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_data);

	GLuint vbo_tile_color;
	glGenBuffers(1, &vbo_tile_color);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color);

	f32 current_time = (f32)SDL_GetTicks() / 60.0;
	f32 t = 0.0;

	glm::vec3 camera_pos = glm::vec3(map_width / 2, map_height + 3.0, map_depth / 2);
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
						if (inside_map(map_width, map_height, map_depth, hovered.x, hovered.y, hovered.z)) {
							map[point_to_oned(hovered, map_width, map_height)] = 0;
							memset(model, 0, sizeof(glm::vec3) * r_map->num_blocks);
							memset(tile_data, 0, sizeof(i32) * r_map->num_blocks);
							memset(colors, 0, sizeof(glm::vec3) * r_map->num_blocks);
							memset(mappings, 0, sizeof(u32) * map_size);
							r_map = hull_map(map, r_map, map_width, map_height, map_depth);
							update_map(model, tile_data, colors, mappings, map_width, map_height, map_depth, r_map, map);

							clicked = true;
						} else {
							printf("%u, %u, %u\n", hovered.x, hovered.y, hovered.z);
						}
					} else if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
						i32 data = 0;
						glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
						glReadBuffer(GL_COLOR_ATTACHMENT1);
						glReadPixels(screen_width / 2, screen_height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &data);

						u32 pos = (u32)data >> 3;
						u8 side = ((u32)data << 29) >> 29;
						Point p = oned_to_threed(pos, map_width, map_height);

						i8 x_adj = 0;
						i8 y_adj = 0;
						i8 z_adj = 0;

						switch (side) {
							case 0: {
								z_adj = 1;
							} break;
							case 1: {
								y_adj = 1;
							} break;
							case 2: {
								z_adj = -1;
							} break;
							case 3: {
								y_adj = -1;
							} break;
							case 4: {
								x_adj = -1;
							} break;
							case 5: {
								x_adj = 1;
							} break;
							default: {
								puts("not a valid side");
								return 1;
							}
						}

						p.x += x_adj;
						p.y += y_adj;
						p.z += z_adj;
                        if (inside_map(map_width, map_height, map_depth, p.x, p.y, p.z)) {
							map[threed_to_oned(p.x, p.y, p.z, map_width, map_height)] = 1;
							memset(model, 0, sizeof(glm::vec3) * r_map->num_blocks);
							memset(tile_data, 0, sizeof(i32) * r_map->num_blocks);
							memset(colors, 0, sizeof(glm::vec3) * r_map->num_blocks);
							memset(mappings, 0, sizeof(u32) * map_size);
							r_map = hull_map(map, r_map, map_width, map_height, map_depth);
							update_map(model, tile_data, colors, mappings, map_width, map_height, map_depth, r_map, map);
							clicked = true;
						}
					}
				} break;
				case SDL_QUIT: {
					running = 0;
				} break;
			}
		}

		if (clicked) {
			i32 data = 0;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glReadPixels(screen_width / 2, screen_height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &data);

			u32 pos = (u32)data >> 3;
			Point p = oned_to_threed(pos, map_width, map_height);

			colors[mappings[point_to_oned(hovered, map_width, map_height)]] -= glm::vec3(0.1, 0.1, 0.1);
			hovered = p;
			colors[mappings[pos]] += glm::vec3(0.1, 0.1, 0.1);
		}

		glEnable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GL_CHECK(glDrawBuffers(2, obj_buffers));
		glUseProgram(obj_shader_program);

		glEnableVertexAttribArray(points_attr);
		glEnableVertexAttribArray(tile_color_attr);
		glEnableVertexAttribArray(normals_attr);
		glEnableVertexAttribArray(tile_data_attr);
		glEnableVertexAttribArray(model_attr);

		i32 size;

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_points));
		GL_CHECK(glVertexAttribPointer(points_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_normals));
		GL_CHECK(glVertexAttribPointer(normals_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));

		GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices));
		GL_CHECK(glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color));
		GL_CHECK(glVertexAttribPointer(tile_color_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));
		GL_CHECK(glVertexAttribDivisor(tile_color_attr, 1));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_data));
		GL_CHECK(glVertexAttribIPointer(tile_data_attr, 1, GL_INT, 0, NULL));
		GL_CHECK(glVertexAttribDivisor(tile_data_attr, 1));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_model));
		GL_CHECK(glVertexAttribPointer(model_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));
		GL_CHECK(glVertexAttribDivisor(model_attr, 1));


		glm::mat4 perspective;
		perspective = glm::perspective(glm::radians(45.0f), (f32)screen_width / (f32)screen_height, 0.1f, 500.0f);
		glm::mat4 view;
		view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
		glm::mat4 pv = perspective * view;

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * r_map->num_blocks, colors, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_data);
		glBufferData(GL_ARRAY_BUFFER, sizeof(i32) * r_map->num_blocks, tile_data, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * r_map->num_blocks, model, GL_STREAM_DRAW);

		glUniformMatrix4fv(pv_uniform, 1, GL_FALSE, &pv[0][0]);
		GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0, r_map->num_blocks));

		if (clicked) {
			i32 data = 0;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glReadPixels(screen_width / 2, screen_height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &data);

			u32 pos = (u32)data >> 3;
			Point p = oned_to_threed(pos, map_width, map_height);

			colors[mappings[point_to_oned(hovered, map_width, map_height)]] -= glm::vec3(0.1, 0.1, 0.1);
			hovered = p;
			colors[mappings[pos]] += glm::vec3(0.1, 0.1, 0.1);

			clicked = false;
		}

		glDisable(GL_DEPTH_TEST);

        colors[0] = glm::vec3(1.0, 1.0, 1.0);
		pv = glm::ortho(-66.5f, 66.5f, -37.6f, 37.6f, -1.0f, 1.0f);

		model[0] = glm::vec3(0.1, 0.0, 0.0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), colors, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_data);
		glBufferData(GL_ARRAY_BUFFER, sizeof(i32), tile_data, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), model, GL_STREAM_DRAW);

		glUniformMatrix4fv(pv_uniform, 1, GL_FALSE, &pv[0][0]);
		GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0, 1));

		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, screen_width, screen_height, 0, 0, screen_width, screen_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		SDL_GL_SwapWindow(window);
	}

	SDL_Quit();

	return 0;
}
