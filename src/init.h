// Declarations of initialisations!!!

#pragma once

#include <SDL2/SDL.h>



void init_validation_layers();
void init_instance(SDL_Window *window);
void init_pdevice_and_qfi();
void init_device_and_queues();
void init_swapchain(SDL_Window *window);
void init_images_and_image_views();
void init_render_pass();
void init_descriptor_set_layout();
void init_graphics_pipeline();
void init_framebuffers();
void init_command_pool();
void init_vertex_buffer();
void init_index_buffer();
void init_uniform_buffers();
void init_descriptor_pool();
void init_descriptor_sets();
void init_command_buffers();
void init_synchronisation_objects();
void reinit_swapchain(SDL_Window *window);
void init_textures();

