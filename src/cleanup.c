// The function for cleaning everything up...

#include <SDL2/SDL.h>

#include <vulkan/vulkan.h>

#include "global_variables.h"



void cleanup_swapchain() {
	for (int i = 0; i < swapchain_image_count; i++) {
		vkDestroyFramebuffer(device, swapchain_framebuffers[i], NULL);
		vkDestroyImageView(device, swapchain_image_views[i], NULL);
	}

	free(swapchain_images);
	free(swapchain_image_views);
	free(swapchain_framebuffers);

	vkDestroySwapchainKHR(device, swapchain, NULL);
}

void cleanup(SDL_Window *window) {

	// First, we wait for nothing to be in use.
	vkDeviceWaitIdle(device);

	// Now we can clean vulkan up.
	cleanup_swapchain();
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(device, uniform_buffers[i], NULL);
		vkFreeMemory(device, uniform_buffers_memory[i], NULL);
	}
	vkDestroyDescriptorPool(device, descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
	vkDestroyBuffer(device, vertex_buffer, NULL);
	vkFreeMemory(device, vertex_buffer_memory, NULL);
	vkDestroyBuffer(device, index_buffer, NULL);
	vkFreeMemory(device, index_buffer_memory, NULL);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, image_available_semaphores[i], NULL);
		vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
		vkDestroyFence(device, in_flight_fences[i], NULL);
	}
	vkDestroyCommandPool(device, command_pool, NULL);
	vkDestroyPipeline(device, pipeline, NULL);
	vkDestroyPipelineLayout(device, pipeline_layout, NULL);
	vkDestroyRenderPass(device, render_pass, NULL);
	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);

	// Cleaning SDL up.
	SDL_DestroyWindow(window);
	SDL_Quit();
}

