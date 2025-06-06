// For some structs...

#pragma once

#include <cglm/cglm.h>



// QFI stands for queue family indicies by the way.
typedef struct {
	uint32_t graphics;
	uint32_t present;
} QueueFamilyIndices;

typedef struct {
	char *code;
	size_t size;
} ShaderCode;

typedef struct {
	vec3 position;
	vec3 color;
} Vertex;

typedef struct {
	mat4 model;
	mat4 view;
	mat4 projection;
} UniformBufferObject;

// Not adding the roll in yaw-pitch-roll btw, it makes moving really weird.
typedef struct {
	vec3 position;
	double yaw;
	double pitch;
} Camera;

