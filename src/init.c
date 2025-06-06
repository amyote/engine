// For all initialisations!!!

#include <stdbool.h>
#include <stddef.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "global_variables.h"
#include "config.h"
#include "util.h"
#include "cleanup.h"



void init_validation_layers() {

	// Get available layers.
	uint32_t available_layer_count = 0;
	vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
	VkLayerProperties available_layers[available_layer_count];
	vkEnumerateInstanceLayerProperties(&available_layer_count,
			available_layers);

	// Check if the layers we want are available. Otherwise, crash the program.
	for (int i = 0; i < VALIDATION_LAYER_COUNT; i++) {
		bool layer_found = false;

		for (int j = 0; j < available_layer_count; j++) {
			if (strcmp(VALIDATION_LAYERS[i], available_layers[j].layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (layer_found == false) {
			fprintf(stderr, "Failed to initialise validation layers! Layer %s not found!", VALIDATION_LAYERS[i]);
			exit(1);
		}
	}
}

void init_instance(SDL_Window *window) {

	// Specify api version (all the rest is pointless.)
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.apiVersion = VULKAN_VERSION;

	// Get enabled extensions.
	uint32_t enabled_extension_count = 0;
	SDL_Vulkan_GetInstanceExtensions(window,
			&enabled_extension_count,
			NULL);
	const char *enabled_extensions[enabled_extension_count];
	SDL_Vulkan_GetInstanceExtensions(window,
			&enabled_extension_count,
			enabled_extensions);

	// Give some info for extensions and validation layers.
	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = enabled_extension_count;
	create_info.ppEnabledExtensionNames = enabled_extensions;
	create_info.enabledLayerCount = 0;

	if (ENABLE_VALIDATION_LAYERS) {
		create_info.enabledLayerCount = VALIDATION_LAYER_COUNT;
		create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	// Try to create instance.
	if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create instance!");
		exit(1);
	}
}

// Returns whether they have been found or not.
bool check_and_get_pdevice_qfi(VkPhysicalDevice pdevice) {

	VkBool32 found_present = false;
	VkBool32 found_graphics = false;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pdevice,
			&queue_family_count,
			NULL);
	VkQueueFamilyProperties queue_families[queue_family_count];
	vkGetPhysicalDeviceQueueFamilyProperties(pdevice,
			&queue_family_count,
			queue_families);

	for (int i = 0; i < queue_family_count; i++) {

		// Make variables in the loop to just check if *this* queue has present or graphics qfis.
		VkBool32 this_has_present = false;
		VkBool32 this_has_graphics = false;
		
		// Check if current queue family of current physical device supports presenting to surface.
		vkGetPhysicalDeviceSurfaceSupportKHR(pdevice,
				i,
				surface,
				&this_has_present);
		
		if (this_has_present) {
			qfi.present = i;
			found_present = true;
		}

		// Check if the queue family supports graphics.
		this_has_graphics = queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

		if (this_has_graphics) {
			qfi.graphics = i;
			found_graphics = true;
		}
	}
	
	return found_present && found_graphics;
}

bool check_pdevice_extensions(VkPhysicalDevice pdevice) {
	
	bool extensions_supported = true;

	// Get available extensions
	uint32_t supported_extension_count;
	vkEnumerateDeviceExtensionProperties(pdevice,
			NULL,
			&supported_extension_count,
			NULL);

	VkExtensionProperties supported_extensions[supported_extension_count];
	vkEnumerateDeviceExtensionProperties(pdevice,
			NULL,
			&supported_extension_count,
			supported_extensions);

	// Check if every required extension is available.
	for (int i = 0; i < REQUIRED_EXTENSION_COUNT; i++) {

		// Check if the required extension this iteration of the loop is supported.
		bool this_extension_supported = false;

		for (int j = 0; j < supported_extension_count; j++) {
			if (strcmp(REQUIRED_EXTENSIONS[i], supported_extensions[j].extensionName) == 0) {

				// If we find a supported extension by the same name, we understand the extension to be supported.
				this_extension_supported = true;
				break;
			}
		}
		
		if (this_extension_supported == false) {
			extensions_supported = false;
			break;
		}
	}

	return extensions_supported;
}

// We really just wanna check if we have any formats and present modes...
bool check_pdevice_swapchain(VkPhysicalDevice pdevice) {

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice,
			surface,
			&format_count,
			NULL);

	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice,
			surface,
			&present_mode_count,
			NULL);

	return format_count > 0 && present_mode_count > 0;
}

bool check_pdevice(VkPhysicalDevice pdevice) {

	VkPhysicalDeviceFeatures pdevice_features;
	vkGetPhysicalDeviceFeatures(pdevice, &pdevice_features);

	return pdevice_features.geometryShader
		&& check_pdevice_extensions(pdevice)
		&& check_and_get_pdevice_qfi(pdevice)
		&& check_pdevice_swapchain(pdevice);
}

void init_pdevice_and_qfi() {

	// Get a list of the physical devices.
	uint32_t pdevice_count = 0;
	vkEnumeratePhysicalDevices(instance,
			&pdevice_count,
			NULL);

	if (pdevice_count == 0) {
		fprintf(stderr, "Failed to find GPUs with Vulkan support!");
	}

	VkPhysicalDevice pdevices[pdevice_count];
	vkEnumeratePhysicalDevices(instance,
			&pdevice_count,
			pdevices);

	// Check every physical device and take the first one who passes the suitability check.
	for (int i = 0; i < pdevice_count; i++) {
		if (check_pdevice(pdevices[i])) {
			pdevice = pdevices[i];
			break;
		}
	}

	if (pdevice == VK_NULL_HANDLE) {
		fprintf(stderr, "Failed to find GPUs with Vulkan support who don't suck!");
		exit(1);
	}
}

void init_device_and_queues() {

	VkDeviceCreateInfo device_create_info = {};
	VkPhysicalDeviceFeatures device_features = {};

	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pEnabledFeatures = &device_features;
	device_create_info.enabledExtensionCount = REQUIRED_EXTENSION_COUNT;
	device_create_info.ppEnabledExtensionNames = REQUIRED_EXTENSIONS;

	float queue_priority = 1.0; // We don't really care about this, but vulkan does...

	// If it's on the same queue family index, that changes some settings.
	if (qfi.present == qfi.graphics) {
		
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = qfi.present;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		device_create_info.queueCreateInfoCount = 1;
		device_create_info.pQueueCreateInfos = &queue_create_info;
		
		// Now we init!
		if (vkCreateDevice(pdevice, &device_create_info, NULL, &device) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create logical device!");
		}
		vkGetDeviceQueue(device, qfi.present, 0, &present_queue);
		vkGetDeviceQueue(device, qfi.graphics, 0, &graphics_queue);
	}
	else {
		
		// Two queue create infos: one for present, one for graphics.
		VkDeviceQueueCreateInfo queue_create_infos[2] = {{}, {}};

		// For present.
		queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[0].queueFamilyIndex = qfi.present;
		queue_create_infos[0].queueCount = 1;
		queue_create_infos[0].pQueuePriorities = &queue_priority;

		// For graphics.
		queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[1].queueFamilyIndex = qfi.graphics;
		queue_create_infos[1].queueCount = 1;
		queue_create_infos[1].pQueuePriorities = &queue_priority;

		device_create_info.queueCreateInfoCount = 2;
		device_create_info.pQueueCreateInfos = queue_create_infos;

		// Now we init!
		if (vkCreateDevice(pdevice, &device_create_info, NULL, &device) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create logical device!");
		}
		vkGetDeviceQueue(device, qfi.present, 0, &present_queue);
		vkGetDeviceQueue(device, qfi.graphics, 0, &graphics_queue);
	}
}

VkExtent2D choose_swapchain_extent(VkSurfaceCapabilitiesKHR *capabilities, SDL_Window *window) {
	
	if (capabilities->currentExtent.width != UINT32_MAX) {
		return capabilities->currentExtent;
	}

	else {
		int width, height;
		SDL_GetWindowSizeInPixels(window, &width, &height);

		uint32_t width_min = capabilities->minImageExtent.width;
		uint32_t height_min = capabilities->minImageExtent.height;
		uint32_t width_max = capabilities->maxImageExtent.width;
		uint32_t height_max = capabilities->maxImageExtent.height;

		VkExtent2D actual_extent;
		actual_extent.width = clamp_uint32_t((uint32_t) width, width_min, width_max);
		actual_extent.height = clamp_uint32_t((uint32_t) height, height_min, height_max);

		return actual_extent;
	}
}

VkSurfaceFormatKHR choose_swapchain_format(VkSurfaceFormatKHR *formats, int format_count) {

	// We want preferrably that B8G8R8A8 SRGB format with nonlinear color space.
	for (int i = 0; i < format_count; i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
			&& formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return formats[i];
		}
	}

	// Otherwise we just return the first.
	return formats[0];
}

VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR *present_modes, int present_mode_count) {

	// We want preferrably the mailbox present mode.
	for (int i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return present_modes[i];
		}
	}

	// Otherwise we take FIFO.
	return VK_PRESENT_MODE_FIFO_KHR;
}

void init_swapchain(SDL_Window *window) {

	// First, we retrieve info about the swapchain.
	// First, surface capabilities.
	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, surface, &surface_capabilities);

	// Second, surface formats. We know their count is non-zero, we checked before!
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice,
			surface,
			&format_count,
			NULL);
	VkSurfaceFormatKHR formats[format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice,
			surface,
			&format_count,
			formats);

	// Third, surface present modes. We also checked the count is non-zero.
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice,
			surface,
			&present_mode_count,
			NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice,
			surface,
			&present_mode_count,
			present_modes);

	// Ok, now we have all the info we need. We will now choose what format, present mode and extent we want.
	VkPresentModeKHR present_mode = choose_swapchain_present_mode(present_modes, present_mode_count);
	VkSurfaceFormatKHR surface_format = choose_swapchain_format(formats, format_count);
	swapchain_extent = choose_swapchain_extent(&surface_capabilities, window); // We want to remember the extent.
	swapchain_image_format = surface_format.format; // We want to remember the image format.
	
	// We will set the amount of swapchain images to 1 + the possible minimum.
	// We also have to be careful not to exceed the minimum, we check for that after.
	uint32_t image_count = surface_capabilities.minImageCount + 1;
	if (0 < surface_capabilities.maxImageCount
			&& surface_capabilities.maxImageCount < image_count) {
		image_count = surface_capabilities.minImageCount;
	}
	
	// Alright, now we make the create info for the swapchain.
	// First, specify the surface.
	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;

	// Second, about the swapchain images.
	create_info.minImageCount = image_count;
	create_info.imageFormat = swapchain_image_format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = swapchain_extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	create_info.preTransform = surface_capabilities.currentTransform; // No transform.
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // We ignoring alpha, no blending with other windows!
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	// We need to set the image sharing mode, but that'll chenge depending on if the graphics and present queue are the same.
	if (qfi.graphics == qfi.present) {
		// If they are the same, should clearly take ownership of the image. Apparently it's quicker this way.
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		// Create swapchain!!!
		if (vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create swapchain!\n");
			exit(1);
		}
	}
	else {
		// Otherwise, we won't do ownership things.
		// We also need to specify the queue family indices that will access the images.
		uint32_t queue_family_indices[2] = { qfi.graphics, qfi.present };
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;

		// Create swapchain!!!
		if (vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create swapchain!\n");
			exit(1);
		}
	}
}

void init_images_and_image_views() {
	
	// First, get image count.
	vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL);
	
	// Second, allocate memory.
	swapchain_images = malloc(swapchain_image_count * sizeof(VkImage));
	swapchain_image_views = malloc(swapchain_image_count * sizeof(VkImageView));

	if (swapchain_images == NULL || swapchain_image_views == NULL) {
		fprintf(stderr, "Failed to allocate memory for images or for image views!\n");
		exit(1);
	}

	// Third, fill in everything.
	vkGetSwapchainImagesKHR(device,
			swapchain,
			&swapchain_image_count,
			swapchain_images);

	for (int i = 0; i < swapchain_image_count; i++) {

		// One image view for every image.
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = swapchain_image_format;

		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device,
					&create_info,
					NULL,
					&swapchain_image_views[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image views!\n");
			exit(1);
		}
	}
}

void init_render_pass() {

	// We want to say we write to write colors to a color framebuffer.
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = swapchain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Reference the attachment we just made.
	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0; // We refer to color_attachment by index in a one element list consisting of just color_attachment.
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Now we make a subpass! Dang!!!
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	// Now we make a subpass dependency.
	// The render pass supposes it has to do the image layout transitions at the start of the pipeline.
	// This is not true in this case, because we will have the image only at the moment we want to write to it! (In draw_frame, check wait_stages.)
	// We then need to override this supposition with a subpass dependency, which will say to do the image layout transition at the moment the pipeline would wanna write colors to the image.
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Between the before-anything (that is, external) subpass
	dependency.dstSubpass = 0; // And the first subpass,
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Let's wait until the pipeline's color-attachment-write moment
	dependency.srcAccessMask = 0; // (No idea wtf an access mask is btw...)
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Before doing the pipeline's color-attachment-write moment.
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// This seems to mean we just do no waiting. So then what's the point??
	// What this does is set when to do the subpass transition. We don't want to do any other waiting then that, so the srcStageMask and srcAccessMask are the same.

	VkRenderPassCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.attachmentCount = 1;
	create_info.pAttachments = &color_attachment;
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies = &dependency;

	// Now time to create!
	if (vkCreateRenderPass(device, &create_info, NULL, &render_pass) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create render pass!");
		exit(1);
	}
}

ShaderCode read_shader(char *filename) {
	
	ShaderCode shader_code = {};
	FILE *file = fopen(filename, "rb");

	if (file == NULL) {
		fprintf(stderr, "Failed to open shader %s!\n", filename);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0L, SEEK_END);
	shader_code.size = ftell(file);
	rewind(file);
	shader_code.code = malloc(shader_code.size * sizeof(char));

	if (shader_code.code == NULL) {
		fprintf(stderr, "Failed to allocate memory!\n");
		exit(EXIT_FAILURE);
	}

	if (fread(shader_code.code, sizeof(char), shader_code.size, file) != shader_code.size) {
		fprintf(stderr, "Failed to read shader %s!", filename);
		fclose(file);
		exit(1);
	}
	fclose(file);

	return shader_code;
}

VkShaderModule init_shader_module(char *filename) {
	
	// First, get shader code.
	ShaderCode shader_code = read_shader(filename);

	// Second, we fill in the structs and create the shader module!
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = shader_code.size;
	create_info.pCode = (const uint32_t *) shader_code.code;

	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create shader module!");
		exit(1);
	}

	free(shader_code.code);

	return shader_module;
}

VkVertexInputBindingDescription init_binding_description() {

	VkVertexInputBindingDescription binding_description = {};
	binding_description.binding = 0;
	binding_description.stride = sizeof(Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding_description;
}

void init_attribute_descriptions(VkVertexInputAttributeDescription attribute_descriptions[2]) {

	// For the position attribute.
	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // This means that this attribute is three floats. That's true since we pass a vec3.
	attribute_descriptions[0].offset = offsetof(Vertex, position);

	// For the color attribute.
	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // This makes perfect sense here.
	attribute_descriptions[1].offset = offsetof(Vertex, color);
}

void init_graphics_pipeline() {

	// First, let's load shaders!
	// Then we create structs that will specify info about them (when the shader is run, what's it's entry point, etc...)
	VkShaderModule vert_shader_module = init_shader_module("res/vert.spv");
	VkShaderModule frag_shader_module = init_shader_module("res/frag.spv");

	VkPipelineShaderStageCreateInfo shader_stage_infos[2] = {{}, {}};

	shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_infos[0].module = vert_shader_module;
	shader_stage_infos[0].pName = "main";

	shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_infos[1].module = frag_shader_module;
	shader_stage_infos[1].pName = "main";

	// Second, we set dynamic states.
	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = 2;
	dynamic_state_info.pDynamicStates = dynamic_states;

	// Third, we set the fixed function configs.
	// For vertex input, we have to first get binding and attribute descriptions.
	// Then we pass them into the struct.
	VkVertexInputBindingDescription binding_description = init_binding_description();
	VkVertexInputAttributeDescription attribute_descriptions[2];
	init_attribute_descriptions(attribute_descriptions);

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = 2;
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

	// For input assembly, we want to make a triangle of our verticies.
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	// For viewport and scissors, we just want the whole window and nothing cut out.
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchain_extent.width;
	viewport.height = (float) swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = swapchain_extent;

	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.scissorCount = 1;

	// Now, we set the rasterizer!
	VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
	rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_info.depthClampEnable = VK_FALSE;
	rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_info.lineWidth = 1.0f;
	rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer_info.depthBiasEnable = VK_FALSE;

	// Now we say: no multisampling!
	VkPipelineMultisampleStateCreateInfo multisampling_info = {};
	multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_info.sampleShadingEnable = VK_FALSE;
	multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Now, finally, we set up color blending!
	// We have to set up an attachment that will specify we want to write RGBA colo.
	// And we want to just overwrite the image when we write to it. No mixing them together.
	VkPipelineColorBlendAttachmentState color_blending_attachment = {};
	color_blending_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	color_blending_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blending_info = {};
	color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending_info.logicOpEnable = VK_FALSE; // No ANDing the colors together or whatever. We just wanna overwrite.
	color_blending_info.attachmentCount = 1;
	color_blending_info.pAttachments = &color_blending_attachment;

	// Now we make the pipeline layout!
	// We're sending a uniform buffer with the matrices to the vertex shader.
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	
	if (vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &pipeline_layout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create pipeline layout!");
		exit(1);
	}

	// Big struct to be created here!
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// Shaders.
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stage_infos;
	// Fixed functions.
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState = &viewport_info;
	pipeline_info.pRasterizationState = &rasterizer_info;
	pipeline_info.pMultisampleState = &multisampling_info;
	pipeline_info.pColorBlendState = &color_blending_info;
	pipeline_info.pDynamicState = &dynamic_state_info;
	// Pipeline layout.
	pipeline_info.layout = pipeline_layout;
	// Render pass.
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0; // This pipeline runs at subpass 0, so the first one.
	
	// Time to make the pipeline!
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create graphics pipeline!");
		exit(1);
	}

	// We can do this once the pipeline is created.
	vkDestroyShaderModule(device, vert_shader_module, NULL);
	vkDestroyShaderModule(device, frag_shader_module, NULL);
}

void init_framebuffers() {

	// We want to make one framebuffer for each imageview we have.
	swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * swapchain_image_count);
	if (swapchain_framebuffers == NULL) {
		fprintf(stderr, "Failed to allocate memory for framebuffers!");
		exit(1);
	}
	
	for (int i = 0; i < swapchain_image_count; i++) {

		VkFramebufferCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		create_info.renderPass = render_pass;
		create_info.attachmentCount = 1;
		create_info.pAttachments = &swapchain_image_views[i];
		create_info.width = swapchain_extent.width;
		create_info.height = swapchain_extent.height;
		create_info.layers = 1;

		if (vkCreateFramebuffer(device, &create_info, NULL, &swapchain_framebuffers[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create framebuffers!");
			exit(1);
		}
	}
}

void init_command_pool() {

	// We set the flags to RESET_COMMAND_BUFFER_BIT because we want to be able to reset our one command buffer every frame, but idk why we're doing this since there's just one.
	// Also, we need to specify the graphics queue's queue family index because the command pool can only make command buffers that go on one type of queue.
	// In setting the queue family index to that, we tell the command pool to make command buffers for drawing.
	VkCommandPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = qfi.graphics;
	
	if (vkCreateCommandPool(device, &create_info, NULL, &command_pool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool!");
		exit(1);
	}
}

void init_command_buffers() {

	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = command_pool;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(device, &allocate_info, command_buffers) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate command buffer!");
		exit(1);
	}
}

void init_synchronisation_objects() {

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphore_info, NULL, &image_available_semaphores[i])
			|| vkCreateSemaphore(device, &semaphore_info, NULL, &render_finished_semaphores[i])
			|| vkCreateFence(device, &fence_info, NULL, &in_flight_fences[i])) {
			fprintf(stderr, "Failed to initialise synchronisation primitives!");
			exit(1);
		}
	}
}

void reinit_swapchain(SDL_Window *window) {

	vkDeviceWaitIdle(device);

	cleanup_swapchain();
	init_swapchain(window);

	init_images_and_image_views();
	init_framebuffers();
}

// We want to get the index of a memory type that 1: is suitable with the buffer and 2: has the properties we specify.
uint32_t choose_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {

	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(pdevice, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {

		if ((type_filter & (1 << i))
			&& (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	fprintf(stderr, "Failed to find suitable memory type!");
	exit(1);
}

void init_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory) {

	// First, we create the buffer.
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create vertex buffer!");
		exit(1);
	}

	// Now we allocate memory for it.
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memory_requirements);
	
	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = choose_memory_type(memory_requirements.memoryTypeBits, properties);
	
	if (vkAllocateMemory(device, &allocate_info, NULL, buffer_memory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate vertex buffer memory!");
		exit(1);
	}

	// Finally, we specify that the allocated memory is for the buffer.
	vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
}

void copy_buffer(VkBuffer source_buffer, VkBuffer destination_buffer, VkDeviceSize size) {

	// We just want one copy command.
	VkCommandBuffer copy_command_buffer = begin_single_usage_commands();

	VkBufferCopy copy_region = {};
	copy_region.size = size;

	vkCmdCopyBuffer(copy_command_buffer, source_buffer, destination_buffer, 1, &copy_region);

	end_single_usage_commands(copy_command_buffer);
}

void init_vertex_buffer() {

	// First, let's make the staging buffer.
	// The buffer size is the same for both.
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	VkDeviceSize buffer_size = sizeof(Vertex) * vertex_count;
	init_buffer(buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&staging_buffer,
			&staging_buffer_memory);

	// Second, we will fill up the staging buffer.
	void *data;
	vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices, buffer_size);
	vkUnmapMemory(device, staging_buffer_memory);
	
	// Third, we make the vertex buffer.
	init_buffer(buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&vertex_buffer,
			&vertex_buffer_memory);

	// Fourth, we copy from staging buffer to vertex buffer.
	copy_buffer(staging_buffer, vertex_buffer, buffer_size);

	// Fifth, we deallocate the staging buffer!
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);
}

void init_index_buffer() {

	// First, let's make the staging buffer.
	// The buffer size is the same for both.
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	VkDeviceSize buffer_size = sizeof(indices[0]) * index_count;
	init_buffer(buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&staging_buffer,
			&staging_buffer_memory);

	// Second, we will fill up the staging buffer.
	void *data;
	vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, indices, buffer_size);
	vkUnmapMemory(device, staging_buffer_memory);
	
	// Third, we make the index buffer.
	init_buffer(buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&index_buffer,
			&index_buffer_memory);

	// Fourth, we copy from staging buffer to index buffer.
	copy_buffer(staging_buffer, index_buffer, buffer_size);

	// Fifth, we deallocate the staging buffer!
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);
}

void init_descriptor_set_layout() {

	// We want one uniform buffer stored at binding zero used in the vertex shader.
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	VkDescriptorSetLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.bindingCount = 1;
	create_info.pBindings = &ubo_layout_binding;

	if (vkCreateDescriptorSetLayout(device, &create_info, NULL, &descriptor_set_layout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create descriptor set layout!");
		exit(1);
	}
}

void init_uniform_buffers() {

	// The size of our uniform buffers! (They all contain just one)
	VkDeviceSize buffer_size = sizeof(UniformBufferObject);

	// And we init!!
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		init_buffer(buffer_size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&uniform_buffers[i],
				&uniform_buffers_memory[i]);

		vkMapMemory(device, uniform_buffers_memory[i], 0, buffer_size, 0, &uniform_buffers_mapped[i]);
	}
}

void init_descriptor_pool() {
	
	// Just say we need MAX_FRAMES_IN_FLIGHT descriptors...
	VkDescriptorPoolSize pool_size = {};
	pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.poolSizeCount = 1;
	create_info.pPoolSizes = &pool_size;
	create_info.maxSets = MAX_FRAMES_IN_FLIGHT;

	if (vkCreateDescriptorPool(device, &create_info, NULL, &descriptor_pool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create descriptor pool!");
		exit(1);
	}
}

void init_descriptor_sets() {

	// We need to make an array of descriptor set layouts, one for each descriptor set.
	// But we want to use the same layout all the time, so we just make that array and put that layout everywhere in it.
	VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		layouts[i] = descriptor_set_layout;
	}

	// We may move on to allocation.
	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = descriptor_pool;
	allocate_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocate_info.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate descriptor sets!");
		exit(1);
	}

	// Now we will populate the descriptiors.
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		// Specify the buffer we wanna bind to's data...
		VkDescriptorBufferInfo buffer_info = {};
		buffer_info.buffer = uniform_buffers[i];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformBufferObject);

		// Give some infos to the descriptor set about what it is.
		VkWriteDescriptorSet descriptor_write = {};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = descriptor_sets[i];
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;

		// Write the infos!
		vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, NULL);
	}
}

