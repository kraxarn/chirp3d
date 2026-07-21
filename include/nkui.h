#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#define NK_INCLUDE_STANDARD_BOOL

#define NK_MEMCPY SDL_memcpy

#include "nuklear.h"

typedef struct nk_context nk_context_t;
typedef struct nk_allocator nk_allocator_t;
typedef struct nk_buffer nk_buffer_t;
typedef struct nk_colorf nk_colorf_t;
typedef struct nk_font_atlas nk_font_atlas_t;
typedef struct nk_allocator nk_allocator_t;
typedef struct nk_draw_null_texture nk_draw_null_texture_t;
typedef struct nk_convert_config nk_convert_config_t;
typedef struct nk_draw_vertex_layout_element nk_draw_vertex_layout_element_t;
typedef struct nk_draw_command nk_draw_command_t;

typedef enum nk_convert_result nk_convert_result_t;

typedef struct
{
	nk_context_t nk;

	SDL_GPUGraphicsPipeline *pipeline;
	SDL_GPUSampler *sampler;
	SDL_GPUTexture *font_texture;

	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUBuffer *index_buffer;
	Uint32 vertex_buffer_size;
	Uint32 index_buffer_size;

	nk_buffer_t command_buffer;
	nk_draw_null_texture_t null_texture;

	void *vertex_data;
	void *element_data;
	bool insert_toggle;
} nkui_context_t;

bool nkui_init(SDL_Window *window, SDL_GPUDevice *device, nkui_context_t *context);

void nkui_deinit(nkui_context_t *context, SDL_GPUDevice *device);

void nkui_handle_event(nkui_context_t *context, const SDL_Event *event);

bool nkui_render_upload(nkui_context_t *context, SDL_GPUDevice *device,
	SDL_GPUCommandBuffer *command_buffer);

bool nkui_render_draw(nkui_context_t *context, SDL_Window *window,
	SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass);
