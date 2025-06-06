// For some utility functions...

#include <stdint.h>

#include <vulkan/vulkan.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "global_variables.h"
#include "config.h"



// For clamping!
uint32_t clamp_uint32_t(uint32_t value, uint32_t max, uint32_t min) {
	const uint32_t temp = value < min ? min : value;
	return temp > max ? max : temp;
}

double clamp_double(double value, double max, double min) {
	const double temp = value < min ? min : value;
	return temp > max ? max : temp;
}

// Basic command buffer actions which I don't instantly destroy because premature optimization is a no-no.
VkCommandBuffer begin_single_usage_commands() {

	// Allocate and say the command buffer is beginning.
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = command_pool;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void end_single_usage_commands(VkCommandBuffer command_buffer) {

	// Say the command buffer is ending, submit it and deallocate it.
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

