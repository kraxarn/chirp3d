#pragma once

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

bool imgui_create_context(imgui_config_flags_t config_flags);

void imgui_style_colors_dark();

void imgui_style_colors_light();

void imgui_set_scale(float scale);

bool imgui_init_for_sdl3gpu(SDL_Window *window, SDL_GPUDevice *device);

#ifdef __cplusplus
}
#endif
