#pragma once

#include "vector.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_pixels.h>

typedef struct font_t font_t;

[[nodiscard]]
font_t *font_create(SDL_GPUDevice *device, SDL_IOStream *source, float font_size, SDL_Color color);

void font_destroy(font_t *font);

void font_draw_text(const font_t *font, SDL_GPURenderPass *render_pass, SDL_GPUCommandBuffer *command_buffer,
	vector2f_t swapchain_size, vector2f_t position, const char *text);
