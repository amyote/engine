// The main!!!

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <cglm/cglm.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "global_variables.h"
#include "config.h"
#include "init.h"
#include "util.h"
#include "draw.h"
#include "cleanup.h"
#include "structs.h"
#include "update.h"



// Basic stuff.
VkInstance instance = VK_NULL_HANDLE;

VkSurfaceKHR surface = VK_NULL_HANDLE;

QueueFamilyIndices qfi = {};

VkPhysicalDevice pdevice = VK_NULL_HANDLE;

VkQueue present_queue = VK_NULL_HANDLE;
VkQueue graphics_queue = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;

// All vars about the swapchain.
VkExtent2D swapchain_extent = {};
VkFormat swapchain_image_format = {};
VkImage *swapchain_images = NULL;
VkImageView *swapchain_image_views = NULL;
VkFramebuffer *swapchain_framebuffers = NULL;
uint32_t swapchain_image_count = 0;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;

// The render pass! (It's all alone)
VkRenderPass render_pass = VK_NULL_HANDLE;

// Variables for descriptor sets here.
VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];

// Variables about the pipeline.
VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
VkPipeline pipeline = VK_NULL_HANDLE;

// For command buffers.
VkCommandPool command_pool = VK_NULL_HANDLE;
VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];

// For synchronisation.
VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];

// For the uniform buffers.
VkBuffer uniform_buffers[MAX_FRAMES_IN_FLIGHT];
VkDeviceMemory uniform_buffers_memory[MAX_FRAMES_IN_FLIGHT];
void *uniform_buffers_mapped[MAX_FRAMES_IN_FLIGHT];

// For user data!
Vertex *vertices = NULL;
uint32_t vertex_count = 8;
VkBuffer vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory vertex_buffer_memory = VK_NULL_HANDLE;

uint16_t *indices = NULL;
uint32_t index_count = 36;
VkBuffer index_buffer = VK_NULL_HANDLE;
VkDeviceMemory index_buffer_memory = VK_NULL_HANDLE;

// General information that changes some program behavior.
bool running = true;
bool framebuffer_resized = false;
uint32_t current_frame = 0;

int main() {

	// Starting SDL!
	SDL_Init(SDL_INIT_VIDEO);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	SDL_Window *window = SDL_CreateWindow(
			WINDOW_NAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			DEFAULT_WINDOW_WIDTH,
			DEFAULT_WINDOW_HEIGHT,
			window_flags);

	if (window == NULL) {
		fprintf(stderr, "Failed to create window! (SDL says: %s)", SDL_GetError());
		exit(1);
	}

	// In order for the mouse to not move when you look around. Like in minecraft and a ton of other games.
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// Just initializing vertices and indices like that yk fr...
	// This part is pretty hacky but if this was a more well-made 3d rendering thing I'd make something to load the data from a file.
	vertices = malloc(sizeof(Vertex) * vertex_count);
	if (vertices == NULL) {
		fprintf(stderr, "Failed to get memory for vertices!");
		exit(1);
	}

	float v1pos[] = {0.5f, 0.5f, 1.0f};
	float v2pos[] = {-0.5f, 0.5f, 1.0f};
	float v3pos[] = {-0.5f, -0.5f, 1.0f};
	float v4pos[] = {0.5f, -0.5f, 1.0f};
	float v5pos[] = {0.5f, 0.5f, 2.0f};
	float v6pos[] = {-0.5f, 0.5f, 2.0f};
	float v7pos[] = {-0.5f, -0.5f, 2.0f};
	float v8pos[] = {0.5f, -0.5f, 2.0f};

	float v1col[] = {1.0f, 0.0f, 0.0f};
	float v2col[] = {0.0f, 1.0f, 0.0f};
	float v3col[] = {0.0f, 0.0f, 1.0f};
	float v4col[] = {1.0f, 1.0f, 1.0f};
	float v5col[] = {1.0f, 0.0f, 0.0f};
	float v6col[] = {0.0f, 1.0f, 0.0f};
	float v7col[] = {0.0f, 0.0f, 1.0f};
	float v8col[] = {1.0f, 1.0f, 1.0f};

	glm_vec3_make(v1pos, vertices[0].position);
	glm_vec3_make(v2pos, vertices[1].position);
	glm_vec3_make(v3pos, vertices[2].position);
	glm_vec3_make(v4pos, vertices[3].position);
	glm_vec3_make(v5pos, vertices[4].position);
	glm_vec3_make(v6pos, vertices[5].position);
	glm_vec3_make(v7pos, vertices[6].position);
	glm_vec3_make(v8pos, vertices[7].position);

	glm_vec3_make(v1col, vertices[0].color);
	glm_vec3_make(v2col, vertices[1].color);
	glm_vec3_make(v3col, vertices[2].color);
	glm_vec3_make(v4col, vertices[3].color);
	glm_vec3_make(v5col, vertices[4].color);
	glm_vec3_make(v6col, vertices[5].color);
	glm_vec3_make(v7col, vertices[6].color);
	glm_vec3_make(v8col, vertices[7].color);

	indices = malloc(sizeof(indices[0]) * index_count);
	if (indices == NULL) {
		fprintf(stderr, "Failed to get memory for indices!");
		exit(1);
	}

	// Making the cube...
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 2;
	indices[4] = 3;
	indices[5] = 0;

	indices[6] = 5;
	indices[7] = 1;
	indices[8] = 0;
	indices[9] = 0;
	indices[10] = 4;
	indices[11] = 5;

	indices[12] = 3;
	indices[13] = 2;
	indices[14] = 6;
	indices[15] = 6;
	indices[16] = 7;
	indices[17] = 3;

	indices[18] = 6;
	indices[19] = 5;
	indices[20] = 4;
	indices[21] = 4;
	indices[22] = 7;
	indices[23] = 6;

	indices[24] = 6;
	indices[25] = 2;
	indices[26] = 1;
	indices[27] = 1;
	indices[28] = 5;
	indices[29] = 6;

	indices[30] = 0;
	indices[31] = 3;
	indices[32] = 7;
	indices[33] = 7;
	indices[34] = 4;
	indices[35] = 0;

	// Initialising vulkan!!!
	init_validation_layers();
	init_instance(window);
	if (SDL_Vulkan_CreateSurface(window, instance, &surface) == SDL_FALSE) {
		fprintf(stderr, "Failed to create surface! (SDL says: %s)", SDL_GetError());
		exit(1);
	}
	init_pdevice_and_qfi(); // QFI stands for queue family indicies by the way.
	init_device_and_queues();
	init_swapchain(window);
	init_images_and_image_views();
	init_render_pass();
	init_descriptor_set_layout();
	init_graphics_pipeline();
	init_framebuffers();
	init_command_pool();
	init_vertex_buffer();
	init_index_buffer();
	init_uniform_buffers();
	init_descriptor_pool();
	init_descriptor_sets();
	init_command_buffers();
	init_synchronisation_objects();

	// Just freeing vertices and indices like that yk fr...
	free(vertices);
	free(indices);

	// These variables exist to calculate the delta_time parameter for the update function.
	// Also, for the first time the update function is called, we will initialise time_at_last_call here.
	uint32_t time_at_call, time_at_last_call;
	time_at_last_call = SDL_GetTicks();

	// The main loop!
	while (running) {
		
		// We give the update function the time since it was last called, so like the delta time.
		time_at_call = SDL_GetTicks();
		update(time_at_call - time_at_last_call);
		time_at_last_call = time_at_call;

		draw_frame(window);
		framebuffer_resized = false;

		SDL_Delay(16);
	}

	// Ok we're done time to clean up!
	cleanup(window);
	return 0;
}

