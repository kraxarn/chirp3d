#include "appstate.h"
#include "logcategory.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef NDEBUG
#include <SDL3/SDL_messagebox.h>
#endif

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

#ifdef NDEBUG
constexpr auto debug = false;
#else
constexpr auto debug = true;
#endif

static SDL_AppResult fatal_error([[maybe_unused]] SDL_Window *window, const char *message)
{
	SDL_LogCritical(LOG_CATEGORY_CORE, "%s: %s", message, SDL_GetError());
#ifdef NDEBUG
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, message, SDL_GetError(), window);
#endif
	return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit([[maybe_unused]] void **appstate,
	[[maybe_unused]] const int argc, [[maybe_unused]] char **argv)
{
#ifdef NDEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_WARN);
#else
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#endif

	// TODO: Set these from the game, not engine
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, ENGINE_VERSION);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

	if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		return fatal_error(nullptr, "Initialisation failed");
	}

	app_state_t *state = SDL_calloc(1, sizeof(app_state_t));
	if (state == nullptr)
	{
		return fatal_error(nullptr, "Memory allocation failed");
	}
	*appstate = state;

	constexpr auto window_w = 1280;
	constexpr auto window_h = 720;
	constexpr auto window_flags = SDL_WINDOW_RESIZABLE;

	state->window = SDL_CreateWindow(ENGINE_NAME, window_w, window_h, window_flags);
	if (state->window == nullptr)
	{
		return fatal_error(nullptr, "Window creation failed");
	}

	constexpr SDL_GPUShaderFormat format_flags =
		SDL_GPU_SHADERFORMAT_SPIRV
		| SDL_GPU_SHADERFORMAT_DXIL
		| SDL_GPU_SHADERFORMAT_MSL;

	state->device = SDL_CreateGPUDevice(format_flags, debug, nullptr);
	if (state->device == nullptr)
	{
		return fatal_error(nullptr, "GPU context creation failed");
	}

	if (!SDL_ClaimWindowForGPUDevice(state->device, state->window))
	{
		return fatal_error(nullptr, "Context binding failed");
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *appstate)
{
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_AppResult result)
{
	const app_state_t *state = appstate;
	SDL_DestroyWindow(state->window);
	SDL_DestroyGPUDevice(state->device);

	SDL_free(appstate);
}
