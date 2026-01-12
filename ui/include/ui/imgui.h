#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>

#ifdef __cplusplus
extern "C"
{
#endif

// enum ImGuiConfigFlags_
typedef enum imgui_config_flags_t
{
	IMGUI_CONFIG_NONE                   = 0,
	IMGUI_CONFIG_NAV_ENABLE_KEYBOARD    = 1 << 0,
	IMGUI_CONFIG_NAV_ENABLE_GAMEPAD     = 1 << 1,
	IMGUI_CONFIG_NO_MOUSE               = 1 << 4,
	IMGUI_CONFIG_NO_MOUSE_CURSOR_CHANGE = 1 << 5,
	IMGUI_CONFIG_NO_KEYBOARD            = 1 << 6,

	IMGUI_CONFIG_IS_SRGB         = 1 << 20,
	IMGUI_CONFIG_IS_TOUCH_SCREEN = 1 << 21,
} imgui_config_flags_t;

typedef struct ImDrawData imgui_draw_data_t;

bool imgui_create_context(imgui_config_flags_t config_flags);

void imgui_destroy_context();

void imgui_style_colors_dark();

void imgui_style_colors_light();

void imgui_set_scale(float scale);

bool imgui_init_for_sdl3gpu(SDL_Window *window, SDL_GPUDevice *device);

void imgui_process_event(const SDL_Event *event);

void imgui_new_frame();

void imgui_show_demo_window(bool *open);

void imgui_render();

[[nodiscard]]
imgui_draw_data_t *imgui_draw_data();

void imgui_prepare_draw_data(imgui_draw_data_t *draw_data, SDL_GPUCommandBuffer *command_buffer);

void imgui_render_draw_data(imgui_draw_data_t *draw_data,
	SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass);

void imgui_shutdown();

#ifdef __cplusplus
}
#endif
