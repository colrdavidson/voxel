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
	SDL_Surface *wall_surf = IMG_Load("assets/brick.png");
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

	GLuint vbo_cube_tex_coords;
	glGenBuffers(1, &vbo_cube_tex_coords);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_tex_coords);
	GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex_coords), cube_tex_coords, GL_STATIC_DRAW));

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
	GLuint mvp_attr = glGetAttribLocation(obj_shader_program, "mvp");

	GLint obj_tex_uniform = glGetUniformLocation(obj_shader_program, "tex");

	glViewport(0, 0, screen_width, screen_height);


	u8 map_width = 25;
	u8 map_height = 25;
	u8 map_depth = 25;
	u32 map_size = map_width * map_height * map_depth;
	u8 *map = (u8 *)malloc(map_size);
	memset(map, 1, map_size);
	glm::vec3 cam_pos = glm::vec3(0.0, 0.0, -map_depth * 2.0);

	glm::mat4 *mvps = (glm::mat4 *)malloc(sizeof(glm::mat4) * map_size);
	i32 *tile_data = (i32 *)malloc(sizeof(i32) * map_size);

	GLuint vbo_mvps;
	glGenBuffers(1, &vbo_mvps);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_mvps);

	GLuint vbo_tile_data;
	glGenBuffers(1, &vbo_tile_data);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_data);

	f32 current_time = (f32)SDL_GetTicks() / 60.0;
	f32 t = 0.0;
	f32 dt = 0.0;

	glm::vec2 angle = glm::vec2(0.0, 0.0);

	i32 old_mouse_x = 0;
	i32 old_mouse_y = 0;

	f32 scale = 25.0f;
	u8 persp = true;
	u8 running = true;
	while (running) {
		SDL_Event event;

		f32 new_time = (f32)SDL_GetTicks() / 60.0;
		f32 dt = new_time - current_time;
		current_time = new_time;
		t += dt;

		glm::vec3 dir = glm::vec3(sin(angle.y) * sin(angle.x), sin(angle.y), cos(angle.y) * cos(angle.x));
		glm::vec3 right = glm::vec3(sin(angle.x - 3.14f / 2.0f), 0, cos(angle.x - 3.14f / 2.0f));
		glm::vec3 up = glm::cross(right, dir);


		f32 cam_speed = 1.0;
		f32 mouse_speed = 0.005;
		SDL_PumpEvents();
		const u8 *state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_W]) {
			cam_pos += dir * dt * cam_speed;
		}
		if (state[SDL_SCANCODE_S]) {
			cam_pos -= dir * dt * cam_speed;
		}
		if (state[SDL_SCANCODE_A]) {
			cam_pos -= right * dt * cam_speed;
		}
		if (state[SDL_SCANCODE_D]) {
			cam_pos += right * dt * cam_speed;
		}

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_f: {
							t = 0;
						} break;
					}
				} break;
				case SDL_MOUSEMOTION: {
					i32 mouse_x, mouse_y;
					SDL_GetMouseState(&mouse_x, &mouse_y);

					angle.x += mouse_speed * dt * (f32)((screen_width / 2) - mouse_x);
					angle.y += mouse_speed * dt * (f32)((screen_height / 2) - (screen_height - mouse_y));

                    f32 angle_max = 3.14 * 2;

					if (angle.x > angle_max || angle.x < -angle_max) {
						angle.x = 0.0f;
					}

					if (angle.y > angle_max || angle.y < -angle_max) {
						angle.y = 0.0f;
					}

					SDL_WarpMouseInWindow(window, screen_width / 2, screen_height / 2);
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					i32 mouse_x, mouse_y;
					u32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

					if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
						SDL_SetRelativeMouseMode(SDL_TRUE);

						i32 data = 0;
						glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
						glReadBuffer(GL_COLOR_ATTACHMENT1);
						glReadPixels(screen_width / 2, screen_height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &data);

						u32 pos = (u32)data >> 3;
						if (pos < map_size) {
							map[pos] = 0;
						} else {
							printf("%u\n", pos);
						}
					} else if (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
						i32 data = 0;
						glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer);
						glReadBuffer(GL_COLOR_ATTACHMENT1);
						glReadPixels(mouse_x, screen_height - mouse_y, 1, 1, GL_RED_INTEGER, GL_INT, &data);

						u32 pos = (u32)data >> 3;
						u8 side = ((u32)data << 29) >> 29;
						Point p = oned_to_threed(pos, map_width, map_height);

						i8 x_adj = 0;
						i8 y_adj = 0;
						i8 z_adj = 0;

						switch (side) {
							case 0: {
								y_adj = 1;
							} break;
							case 1: {
								z_adj = 1;
							} break;
							case 2: {
								y_adj = -1;
							} break;
							case 3: {
								z_adj = -1;
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
						map[threed_to_oned(p.x + x_adj, p.y + y_adj, p.z + z_adj, map_width, map_height)] = 1;
					}
				} break;
				case SDL_QUIT: {
					running = 0;
				} break;
			}
		}

		printf("%f\n", dt);
		printf("%f, %f\n", angle.x, angle.y);
		printf("%f, %f, %f\n", cam_pos.x, cam_pos.y, cam_pos.z);

		glEnable(GL_DEPTH_TEST);
		glm::mat4 perspective;
		perspective = glm::perspective(glm::radians(40.0f), (f32)screen_width / (f32)screen_height, 0.1f, 500.0f);

		glm::mat4 view;
		view = glm::lookAt(cam_pos, cam_pos + dir, up);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GL_CHECK(glDrawBuffers(2, obj_buffers));
		glUseProgram(obj_shader_program);

		glEnableVertexAttribArray(points_attr);
		glEnableVertexAttribArray(texpos_attr);
		glEnableVertexAttribArray(normals_attr);
		glEnableVertexAttribArray(tile_data_attr);
		glEnableVertexAttribArray(mvp_attr);

		i32 size;

		GL_CHECK(glActiveTexture(GL_TEXTURE0));
		GL_CHECK(glUniform1i(obj_tex_uniform, 0));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_points));
		GL_CHECK(glVertexAttribPointer(points_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, wall_tex));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_normals));
		GL_CHECK(glVertexAttribPointer(normals_attr, 3, GL_FLOAT, GL_FALSE, 0, 0));

		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_cube_tex_coords));
		GL_CHECK(glVertexAttribPointer(texpos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0));

		GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_cube_indices));
		GL_CHECK(glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size));

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
		for (u32 i = 0; i < map_size; i++) {
			u32 tile_id = map[i];
			if (tile_id != 0) {
				Point p = oned_to_threed(i, map_width, map_height);

				glm::mat4 model = glm::mat4(1.0);

				model = glm::translate(model, glm::vec3(p.x * 2.0f - map_width, p.z * 2.0f, p.y * 2.0f - map_height));
				glm::mat4 mvp = pv * model;

				mvps[i] = mvp;
				tile_data[i] = i;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo_tile_data);
		glBufferData(GL_ARRAY_BUFFER, sizeof(i32) * map_size, tile_data, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_mvps);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * map_size, mvps, GL_STREAM_DRAW);

		/*
		   GL_CHECK(glUniformMatrix4fv(obj_mvp_uniform, 1, GL_FALSE, &mvp[0][0]));
		   GL_CHECK(glUniform1i(obj_tile_data_uniform, i));

		   GL_CHECK(glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0));
		*/

		GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0, map_size));

		glDisableVertexAttribArray(points_attr);
		glDisableVertexAttribArray(texpos_attr);
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
