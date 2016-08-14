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
#include "gl_helper.h"

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

	GLuint wall_tex;
	SDL_Surface *wall_surf = IMG_Load("assets/grass.png");
	glGenTextures(1, &wall_tex);
	glBindTexture(GL_TEXTURE_2D, wall_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wall_surf->w, wall_surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, wall_surf->pixels);

	SDL_FreeSurface(wall_surf);

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
	GLuint texpos_attr = glGetAttribLocation(obj_shader_program, "tex_coords");
	GLuint normals_attr = glGetAttribLocation(obj_shader_program, "normals");
	GLuint tile_data_attr = glGetAttribLocation(obj_shader_program, "tile_data");
	GLuint tile_color_attr = glGetAttribLocation(obj_shader_program, "color");
	GLuint mvp_attr = glGetAttribLocation(obj_shader_program, "mvp");

	glViewport(0, 0, screen_width, screen_height);

	u32 map_width = 16 * 5;
	u32 map_height = 16;
	u32 map_depth = 16 * 5;
	u32 map_size = map_width * map_height * map_depth;
	u8 *map = (u8 *)malloc(map_size);
	memset(map, 1, map_size);

	glm::mat4 *mvps = (glm::mat4 *)malloc(sizeof(glm::mat4) * map_size);
	glm::vec3 *colors = (glm::vec3 *)malloc(sizeof(glm::vec3) * map_size);
	i32 *tile_data = (i32 *)malloc(sizeof(i32) * map_size);

	GLuint vbo_mvps;
	glGenBuffers(1, &vbo_mvps);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_mvps);

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

	Point hovered;

	u8 warped = false;
	u8 warp = false;
	u8 running = true;
	while (running) {
		SDL_Event event;

		f32 new_time = (f32)SDL_GetTicks() / 60.0;
		f32 dt = new_time - current_time;
		current_time = new_time;
		t += dt;

		f32 saved_y = camera_pos.y;
		f32 cam_speed = 0.5;
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

		camera_pos.y = saved_y;

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

						f32 mouse_speed = 0.25;
						x_off *= mouse_speed;
						y_off *= mouse_speed;

						yaw += x_off;
						pitch += y_off;

						if (pitch > 89.0f)
							pitch = 89.0f;
						if (pitch < -89.0f)
							pitch = -89.0f;

						glm::vec3 front = glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)), sin(glm::radians(yaw)) * cos(glm::radians(pitch)));

						i32 data = 0;
						glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
						glReadBuffer(GL_COLOR_ATTACHMENT1);
						glReadPixels(screen_width / 2, screen_height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &data);

						u32 pos = (u32)data >> 3;

						hovered = oned_to_threed(pos, map_width, map_height);

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
						u32 pos = threed_to_oned(hovered.x, hovered.y, hovered.z, map_width, map_height);

						if (pos < map_size) {
							map[pos] = 0;
						} else {
							printf("%u\n", pos);
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
								puts("front");
							} break;
							case 1: {
								y_adj = 1;
								puts("top");
							} break;
							case 2: {
								z_adj = -1;
								puts("back");
							} break;
							case 3: {
								y_adj = -1;
								puts("bottom");
							} break;
							case 4: {
								x_adj = -1;
								puts("left");
							} break;
							case 5: {
								x_adj = 1;
								puts("right");
							} break;
							default: {
								puts("not a valid side");
								return 1;
							}
						}

						printf("(%d, %d, %d) -> (%d, %d, %d)\n", p.x, p.y, p.z, p.x + x_adj, p.y + y_adj, p.z + z_adj);
						printf("%d\n", map[threed_to_oned(p.x + x_adj, p.y + y_adj, p.z + z_adj, map_width, map_height)]);
						map[threed_to_oned(p.x + x_adj, p.y + y_adj, p.z + z_adj, map_width, map_height)] = 1;
					}
				} break;
				case SDL_QUIT: {
					running = 0;
				} break;
			}
		}


		glEnable(GL_DEPTH_TEST);
		glm::mat4 perspective;
		perspective = glm::perspective(glm::radians(50.0f), (f32)screen_width / (f32)screen_height, 0.1f, 500.0f);

		glm::mat4 view;
		view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GL_CHECK(glDrawBuffers(2, obj_buffers));
		glUseProgram(obj_shader_program);

		glEnableVertexAttribArray(points_attr);
		glEnableVertexAttribArray(tile_color_attr);
		glEnableVertexAttribArray(normals_attr);
		glEnableVertexAttribArray(tile_data_attr);
		glEnableVertexAttribArray(mvp_attr);

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

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_mvps));
		for (u32 i = 0; i < 4; i++) {
			GL_CHECK(glVertexAttribPointer(mvp_attr + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(glm::vec4) * i)));
			GL_CHECK(glEnableVertexAttribArray(mvp_attr + i));
			GL_CHECK(glVertexAttribDivisor(mvp_attr + i, 1));
		}

		glm::mat4 pv = perspective * view;
		memset(mvps, 0, sizeof(glm::mat4) * map_size);
		memset(tile_data, 0, sizeof(i32) * map_size);
		memset(colors, 0, sizeof(glm::vec3) * map_size);
		for (u32 i = 0; i < map_size; i++) {
			u32 tile_id = map[i];
			if (tile_id != 0) {
				Point p = oned_to_threed(i, map_width, map_height);

				glm::mat4 model = glm::mat4(1.0);

				model = glm::translate(model, glm::vec3(p.x, p.y, p.z));
				glm::mat4 mvp = pv * model;

				if (p.y == map_height - 1) {
					colors[i] = glm::vec3(0.2, 0.7, 0.2);
				} else {
					colors[i] = glm::vec3(0.5, 0.35, 0.05);
				}

				if (point_eq(p, hovered)) {
                	colors[i] += glm::vec3(0.1, 0.1, 0.1);
				}

				mvps[i] = mvp;
				tile_data[i] = i;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_color);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * map_size, colors, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_data);
		glBufferData(GL_ARRAY_BUFFER, sizeof(i32) * map_size, tile_data, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_mvps);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * map_size, mvps, GL_STREAM_DRAW);

		GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0, map_size));

		glDisableVertexAttribArray(points_attr);
		glDisableVertexAttribArray(tile_color_attr);
		glDisableVertexAttribArray(normals_attr);
		glDisableVertexAttribArray(tile_data_attr);
		glDisableVertexAttribArray(mvp_attr);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, screen_width, screen_height, 0, 0, screen_width, screen_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		SDL_GL_SwapWindow(window);
	}

	SDL_Quit();

	return 0;
}
