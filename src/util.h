#pragma once

#include <time.h>
#include <stdint.h>

#include <vulkan/vulkan.h>



uint32_t clamp_uint32_t(uint32_t value, uint32_t max, uint32_t min);
double clamp_double(double value, double max, double min);
VkCommandBuffer begin_single_usage_commands();
void end_single_usage_commands(VkCommandBuffer command_buffer);

