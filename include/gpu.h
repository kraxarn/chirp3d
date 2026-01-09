#pragma once

#include "meshinfo.h"
#include "vector.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_video.h>

[[nodiscard]]
SDL_GPUDevice *create_device(SDL_Window *window);

[[nodiscard]]
SDL_GPUGraphicsPipeline *create_pipeline(SDL_GPUDevice *device, SDL_Window *window,
	SDL_GPUShader *vertex_shader, SDL_GPUShader *fragment_shader);

[[nodiscard]]
SDL_GPUTexture *create_depth_texture(SDL_GPUDevice *device, vector2i_t size);

bool draw_begin(SDL_GPUDevice *device, SDL_Window *window, SDL_FColor clear_color,
	SDL_GPUTexture *depth_texture, SDL_GPUCommandBuffer **command_buffer,
	SDL_GPURenderPass **render_pass, vector2f_t *size);

bool draw_end();

bool gpu_upload_mesh_info(SDL_GPUDevice *device, mesh_info_t info,
	SDL_GPUBuffer **vertex_buffer, SDL_GPUBuffer **index_buffer);

bool gpu_upload_texture(SDL_GPUDevice *device, const SDL_Surface *surface,
	const SDL_GPUSamplerCreateInfo *sampler_info,
	SDL_GPUSampler **sampler, SDL_GPUTexture **texture);
