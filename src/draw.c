// For drawing!!!

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>

#include <cglm/cglm.h>

#include "global_variables.h"
#include "structs.h"
#include "config.h"
#include "util.h"
#include "init.h"



void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) {

	// Begin the command buffer!
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
		fprintf(stderr, "Failed to begin recording command buffer!");
		exit(1);
	}

	// Begin the render pass!
	VkRenderPassBeginInfo render_pass_info = {};
	VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = swapchain_framebuffers[image_index];
	render_pass_info.renderArea.offset = (VkOffset2D) {0, 0};
	render_pass_info.renderArea.extent = swapchain_extent;
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;
	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	// Bind pipeline with vertex buffer and pipeline with index buffer!!
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkBuffer vertex_buffers[] = {vertex_buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16); // And it's here we specify the size of the indices! (They can be uint16_t or uint32_t.)

	// We said we'd specify viewport and scissor at draw time. Well, it is draw time!
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchain_extent.width;
	viewport.height = (float) swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = (VkOffset2D) {0, 0};
	scissor.extent = swapchain_extent;

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	// Bind descriptor sets.
	vkCmdBindDescriptorSets(command_buffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS, // Since descriptor sets can also be used in compute pipelines, we specify here we're using it for a graphics pipeline.
			pipeline_layout,
			0, // Index of the first descriptor set.
			1, // Number of descriptor sets to bind.
			&descriptor_sets[current_frame], // Descriptor sets to bind.
			0,
			NULL);

	// Now, we say to draw!!!
	vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);

	// Now we end everything.
	vkCmdEndRenderPass(command_buffer);
	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to record command buffer!");
		exit(1);
	}
}

void draw_frame(SDL_Window *window) {

	// A little declaration...
	VkResult result;

	// First, we have to wait for the previous frame to finish.
	vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
	
	// Second, get an image from the swapchain.
	uint32_t image_index;
	result = vkAcquireNextImageKHR(device,
			swapchain,
			UINT64_MAX,
			image_available_semaphores[current_frame],
			VK_NULL_HANDLE,
			&image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		reinit_swapchain(window);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		fprintf(stderr, "Failed to get swapchain image!");
		exit(1);
	}

	// At this point, we know we will submit the image now (unless we crash), so we know the fence will be signaled so we can reset it without fear of a deadlock.
	vkResetFences(device, 1, &in_flight_fences[current_frame]);

	// Third, record instructions to the command buffer.
	vkResetCommandBuffer(command_buffers[current_frame], 0);
	record_command_buffer(command_buffers[current_frame], image_index);

	// Fourth, we submit the command buffer.
	// In the command buffer, we want to wait for the image to be available before writing colors.
	// We also need to signal the render finished semaphore at the end of rendering.
	VkSubmitInfo submit_info = {};
	VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore signal_semaphores[] = {render_finished_semaphores[current_frame]};

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[current_frame];

	if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS) {
		fprintf(stderr, "Failed to submit draw command buffer!");
		exit(1);
	}

	// Fifth, we get a present!!!
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphores[current_frame];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain;
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
		reinit_swapchain(window);
	}
	else if (result != VK_SUCCESS) {
		fprintf(stderr, "Failed to present swapchain image!");
		exit(1);
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

