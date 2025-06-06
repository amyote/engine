// Config file!!! (Declarations...)

#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>



extern const char WINDOW_NAME[];
extern const int DEFAULT_WINDOW_WIDTH;
extern const int DEFAULT_WINDOW_HEIGHT;

extern const uint32_t VULKAN_VERSION;

extern const bool ENABLE_VALIDATION_LAYERS;
extern const uint32_t VALIDATION_LAYER_COUNT;
extern const char *VALIDATION_LAYERS[];

extern const char REQUIRED_EXTENSION_COUNT;
extern const char *REQUIRED_EXTENSIONS[];

extern const double sensitivity;
extern const double pitch_max;
extern const double speed;

#define MAX_FRAMES_IN_FLIGHT 2

