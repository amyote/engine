// The file containing the update function!!!

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>

#include <cglm/cglm.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "global_variables.h"
#include "config.h"
#include "init.h"
#include "util.h"
#include "structs.h"




void update(uint32_t delta_time) {

	// Declare and initialise camera!!!
	// Also, a yaw of 0 and pitch of 0 should point in the direction of (0, 0, 1), so GLM_ZUP.
	static Camera camera = {};

	// 1 means going in the direction, 0 means not doing anything in that direction, -1 means going in the opposite direction.
	// Note that for up and left, they are based on where the camera is looking. So forwards means the camera goes straight forward.
	// However, up will always stay the same up (which is (0, 1, 0)), no matter where the player faces.
	static float forwards_scalar = 0;
	static float left_scalar = 0;
	static float up_scalar = 0;

	// Moving the mouse around should make the camera look around.
	// We record here this mouse movement.
	int x_mouse_movement = 0;
	int y_mouse_movement = 0;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {

		// Exit if told to.
		if (event.type == SDL_QUIT) {
			running = false;
		}

		// If we resize, remember it.
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
			framebuffer_resized = true;
		}

		// For moving around.
		if (event.type == SDL_KEYDOWN && event.key.repeat == false) {
			switch (event.key.keysym.sym) {
			case SDLK_w:
				forwards_scalar += 1;
				break;
			case SDLK_s:
				forwards_scalar -= 1;
				break;
			case SDLK_a:
				left_scalar += 1;
				break;
			case SDLK_d:
				left_scalar -= 1;
				break;
			case SDLK_SPACE:
				up_scalar += 1;
				break;
			case SDLK_LSHIFT:
				up_scalar -= 1;
				break;
			default:
				break;
			}
		}
		if (event.type == SDL_KEYUP && event.key.repeat == false) {
			switch (event.key.keysym.sym) {
			case SDLK_w:
				forwards_scalar -= 1;
				break;
			case SDLK_s:
				forwards_scalar += 1;
				break;
			case SDLK_a:
				left_scalar -= 1;
				break;
			case SDLK_d:
				left_scalar += 1;
				break;
			case SDLK_SPACE:
				up_scalar -= 1;
				break;
			case SDLK_LSHIFT:
				up_scalar += 1;
				break;
			default:
				break;
			}
		}
		
		// For looking around.
		if (event.type == SDL_MOUSEMOTION) {
			x_mouse_movement += event.motion.xrel;
			y_mouse_movement += event.motion.yrel;
		}
	}

	// First, let's edit the yaw and pitch. (Note that pitch here is represented in an inverted way because SDL inverts the y cooridate, but this is fine since it's reversed again while making the matrix to get forwards. This doesn't matter so I won't try to be clearer.)
	// We must also clamp the pitch so that we can't look straight up or look straight down.
	camera.yaw += x_mouse_movement * sensitivity;
	camera.pitch += y_mouse_movement * sensitivity;
	camera.pitch = clamp_double(camera.pitch, pitch_max, -pitch_max);

	// Second, let's calculate our vectors:
	// Where is the forwards? Where is the left? Where is the up???
	vec3 forwards, left;

	// We can make a transformation matrix to calculate our forwards vector, which will be GLM_ZUP rotated by yaw and pitch.
	glm_vec3_copy(GLM_ZUP, forwards);
	glm_vec3_rotate(forwards, camera.pitch, GLM_XUP); // Rotating by pitch.
	glm_vec3_rotate(forwards, -camera.yaw, GLM_YUP); // Rotating by yaw.

	// We will now calculate the left vector. It must be perpendicular to forwards and GLM_ZUP and it must be a normalized, so we can just use a cross product.
	glm_vec3_crossn(GLM_YUP, forwards, left);

	// Now we know all our directions. It's time to calculate the movement in every direction.
	vec3 forwards_movement, left_movement, up_movement;
	glm_vec3_scale(forwards, forwards_scalar * speed * delta_time, forwards_movement);
	glm_vec3_scale(left, left_scalar * speed * delta_time, left_movement);
	glm_vec3_scale(GLM_YUP, up_scalar * speed * delta_time, up_movement);

	// Now we can add all of these to the camera position.
	glm_vec3_add(camera.position, forwards_movement, camera.position);
	glm_vec3_add(camera.position, left_movement, camera.position);
	glm_vec3_add(camera.position, up_movement, camera.position);

	// Alright! All the basic logic is done.
	// Now we init the UBO.
	UniformBufferObject ubo;

	glm_mat4_identity(ubo.model); // No model movement or rotation whatsoever.
	glm_look(camera.position, forwards, GLM_YUP, ubo.view); // We look from camera.position in the forwards direction with GLM_YUP as our up vector.
	glm_perspective(glm_rad(45.0f), // FOV.
			swapchain_extent.width / (float) swapchain_extent.height, // Aspect ratio.
			0.1f, // Near clipping.
			50.0f, // Far clipping.
			ubo.projection); // Where to write that matrix.
	
	ubo.projection[1][1] *= -1; // To invert the inversion of the image. (OpenGL takes the image inverted, but vulkan doesn't and cglm was made for OpenGL.)

	// Finally, we write it to the UB.
	memcpy(uniform_buffers_mapped[current_frame], &ubo, sizeof(ubo));
}

