#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_rect.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Ensure C23 compatibility
// NOLINTBEGIN(*-use-trailing-return-type, *-use-using)

// enum ImGuiConfigFlags_
typedef enum [[clang::flag_enum]] imgui_config_flags_t
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

//enum ImGuiWindowFlags_
typedef enum [[clang::flag_enum]] imgui_window_flags_t
{
	IMGUI_WINDOW_NONE                        = 0,
	IMGUI_WINDOW_NO_TITLE_BAR                = 1 << 0,
	IMGUI_WINDOW_NO_RESIZE                   = 1 << 1,
	IMGUI_WINDOW_NO_MOVE                     = 1 << 2,
	IMGUI_WINDOW_NO_SCROLLBAR                = 1 << 3,
	IMGUI_WINDOW_NO_SCROLL_WITH_MOUSE        = 1 << 4,
	IMGUI_WINDOW_NO_COLLAPSE                 = 1 << 5,
	IMGUI_WINDOW_ALWAYS_AUTO_RESIZE          = 1 << 6,
	IMGUI_WINDOW_NO_BACKGROUND               = 1 << 7,
	IMGUI_WINDOW_NO_SAVED_SETTINGS           = 1 << 8,
	IMGUI_WINDOW_NO_MOUSE_INPUTS             = 1 << 9,
	IMGUI_WINDOW_MENU_BAR                    = 1 << 10,
	IMGUI_WINDOW_HORIZONTAL_SCROLLBAR        = 1 << 11,
	IMGUI_WINDOW_NO_FOCUS_ON_APPEARING       = 1 << 12,
	IMGUI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS  = 1 << 13,
	IMGUI_WINDOW_ALWAYS_VERTICAL_SCROLLBAR   = 1 << 14,
	IMGUI_WINDOW_ALWAYS_HORIZONTAL_SCROLLBAR = 1 << 15,
	IMGUI_WINDOW_NO_NAV_INPUTS               = 1 << 16,
	IMGUI_WINDOW_NO_NAV_FOCUS                = 1 << 17,
	IMGUI_WINDOW_UNSAVED_DOCUMENT            = 1 << 18,

	IMGUI_WINDOW_NO_NAV        = IMGUI_WINDOW_NO_NAV_INPUTS | IMGUI_WINDOW_NO_NAV_FOCUS,
	IMGUI_WINDOW_NO_DECORATION = IMGUI_WINDOW_NO_TITLE_BAR | IMGUI_WINDOW_NO_RESIZE |
	IMGUI_WINDOW_NO_SCROLLBAR | IMGUI_WINDOW_NO_COLLAPSE,
	IMGUI_WINDOW_NO_INPUTS = IMGUI_WINDOW_NO_MOUSE_INPUTS | IMGUI_WINDOW_NO_NAV_INPUTS |
	IMGUI_WINDOW_NO_NAV_FOCUS,
} imgui_window_flags_t;

typedef struct ImDrawData imgui_draw_data_t;
typedef struct ImFont imgui_font_t;
typedef struct ImGuiViewport imgui_viewport_t;

bool imgui_create_context(imgui_config_flags_t config_flags);

void imgui_destroy_context();

void imgui_shutdown();

[[nodiscard]]
imgui_font_t *imgui_add_font(const Uint8 *data, size_t data_size, float font_size);

void imgui_style_colors_dark();

void imgui_style_colors_light();

void imgui_style_colors_custom();

void imgui_set_scale(float scale);

bool imgui_init_for_sdl3gpu(SDL_Window *window, SDL_GPUDevice *device);

bool imgui_process_event(const SDL_Event *event);

void imgui_new_frame();

void imgui_render();

[[nodiscard]]
imgui_draw_data_t *imgui_draw_data();

void imgui_prepare_draw_data(imgui_draw_data_t *draw_data, SDL_GPUCommandBuffer *command_buffer);

void imgui_render_draw_data(imgui_draw_data_t *draw_data,
	SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass);

[[nodiscard]]
bool imgui_want_capture_mouse();

[[nodiscard]]
imgui_viewport_t *imgui_main_viewport();

[[nodiscard]]
SDL_FRect imgui_viewport_work_area(const imgui_viewport_t *viewport);

// NOLINTEND(*-use-trailing-return-type, *-use-using)

#ifdef __cplusplus
}
#endif
