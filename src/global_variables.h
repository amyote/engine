// Where I will put global vars.

#pragma once

#include <vulkan/vulkan.h>

#include "structs.h"
#include "config.h"



extern VkInstance instance;

extern VkSurfaceKHR surface;

extern QueueFamilyIndices qfi;

extern VkPhysicalDevice pdevice;

extern VkQueue present_queue;
extern VkQueue graphics_queue;
extern VkDevice device;

extern VkExtent2D swapchain_extent;
extern VkFormat swapchain_image_format;
extern VkImage *swapchain_images;
extern VkImageView *swapchain_image_views;
extern VkFramebuffer *swapchain_framebuffers;
extern uint32_t swapchain_image_count;
extern VkSwapchainKHR swapchain;

extern VkRenderPass render_pass;

extern VkDescriptorSetLayout descriptor_set_layout;
extern VkDescriptorPool descriptor_pool;
extern VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];

extern VkPipelineLayout pipeline_layout;
extern VkPipeline pipeline;

extern VkCommandPool command_pool;
extern VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];

extern VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
extern VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
extern VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];

extern bool running;
extern bool framebuffer_resized;
extern uint32_t current_frame;

extern Vertex *vertices;
extern uint32_t vertex_count;
extern VkBuffer vertex_buffer;
extern VkDeviceMemory vertex_buffer_memory;

extern uint16_t *indices;
extern uint32_t index_count;
extern VkBuffer index_buffer;
extern VkDeviceMemory index_buffer_memory;

extern Camera camera;

extern VkBuffer uniform_buffers[MAX_FRAMES_IN_FLIGHT];
extern VkDeviceMemory uniform_buffers_memory[MAX_FRAMES_IN_FLIGHT];
extern void *uniform_buffers_mapped[MAX_FRAMES_IN_FLIGHT];

