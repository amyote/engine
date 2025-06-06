// Config file!!!

#include <math.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>



const char WINDOW_NAME[] = "Vulkan Tutorial Fr Edition";
const int DEFAULT_WINDOW_WIDTH = 1920;
const int DEFAULT_WINDOW_HEIGHT = 1080;

const uint32_t VULKAN_VERSION = VK_API_VERSION_1_3;

const bool ENABLE_VALIDATION_LAYERS = true;
const uint32_t VALIDATION_LAYER_COUNT = 1;
const char *VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation"
};

const char REQUIRED_EXTENSION_COUNT = 1;
const char *REQUIRED_EXTENSIONS[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const double sensitivity = 0.002;
const double pitch_max = M_PI_2 - 0.2; // Has to be less than M_PI_2. Otherwise, you can look straight up which causes trouble.
const double speed = 0.001;

