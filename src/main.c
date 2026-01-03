#include "appstate.h"
#include "assets.h"
#include "audiodriver.h"
#include "font.h"
#include "gpu.h"
#include "gpudevicedriver.h"
#include "image.h"
#include "logcategory.h"
#include "math.h"
#include "matrix.h"
#include "model.h"
#include "resources.h"
#include "shader.h"
#include "uniformdata.h"
#include "videodriver.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef NDEBUG
#include <SDL3/SDL_messagebox.h>
#endif

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>

static constexpr auto mouse_sensitivity = 0.075F;
static constexpr auto move_speed = 1.5F;

static SDL_AppResult fatal_error([[maybe_unused]] SDL_Window *window, const char *message)
{
	SDL_LogCritical(LOG_CATEGORY_CORE, "%s: %s", message, SDL_GetError());
#ifdef NDEBUG
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, message, SDL_GetError(), window);
#endif
	return SDL_APP_FAILURE;
}

static void log_gpu_info(SDL_GPUDevice *device)
{
#if SDL_VERSION_ATLEAST(3, 4, 0)
	const SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);

	const char *name = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, nullptr);
	if (name != nullptr)
	{
		SDL_LogDebug(LOG_CATEGORY_CORE, "GPU: %s", name);
	}

	const char *driver_name = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, nullptr);
	if (driver_name != nullptr)
	{
		const char *driver_version = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_INFO_STRING,
			SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, nullptr)
		);

		if (driver_version != nullptr)
		{
			SDL_LogDebug(LOG_CATEGORY_CORE, "Driver: %s %s", driver_name, driver_version);
		}
	}
#endif
}

SDL_AppResult SDL_AppInit(void **appstate, const int argc, char **argv)
{
#ifdef NDEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
#else
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#endif

	app_state_t *state = SDL_calloc(1, sizeof(app_state_t));
	if (state == nullptr)
	{
		return fatal_error(nullptr, "Memory allocation failed");
	}
	*appstate = state;

	if (argc == 2)
	{
		state->assets = assets_create_from_folder(argv[1]);
		if (state->assets == nullptr)
		{
			return fatal_error(state->window, "Failed to load assets");
		}
	}
	else
	{
		state->assets = nullptr;

		SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, ENGINE_NAME);
		SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, ENGINE_VERSION);
		SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, ENGINE_IDENTIFIER);
		SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");
	}

	if (!SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		return fatal_error(nullptr, "Initialisation failed");
	}

	state->last_update = SDL_GetTicks();
	state->current_rotation = 0.F;

	constexpr auto window_w = 1280;
	constexpr auto window_h = 720;
	constexpr auto window_flags = SDL_WINDOW_RESIZABLE;

	state->window = SDL_CreateWindow(ENGINE_NAME, window_w, window_h, window_flags);
	if (state->window == nullptr)
	{
		return fatal_error(nullptr, "Window creation failed");
	}

	state->device = create_device(state->window);
	if (state->device == nullptr)
	{
		return fatal_error(state->window, "GPU context creation failed");
	}

	const char *device_name = SDL_GetGPUDeviceDriver(state->device);
	if (device_name != nullptr)
	{
		char *title = nullptr;
		SDL_asprintf(&title, "%s (%s)", SDL_GetWindowTitle(state->window), device_name);
		SDL_SetWindowTitle(state->window, title);
		SDL_free(title);
	}

	log_gpu_info(state->device);

	if (SDL_GetLogPriority(LOG_CATEGORY_CORE) >= SDL_LOG_PRIORITY_VERBOSE)
	{
		char *gpu_drivers = gpu_driver_names();
		if (gpu_drivers != nullptr)
		{
			SDL_LogDebug(LOG_CATEGORY_CORE, "Available GPU drivers: %s", gpu_drivers);
			SDL_free(gpu_drivers);
		}

		char *shader_formats = shader_format_names(state->device);
		if (shader_formats != nullptr)
		{
			SDL_LogDebug(LOG_CATEGORY_CORE, "Available shader formats for %s: %s",
				SDL_GetGPUDeviceDriver(state->device), shader_formats);
			SDL_free(shader_formats);
		}
	}

	if (!SDL_SetGPUSwapchainParameters(state->device, state->window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
	{
		SDL_LogWarn(LOG_CATEGORY_CORE, "VSync not supported: %s", SDL_GetError());
	}

	SDL_IOStream *font_source = SDL_IOFromConstMem(font_monogram_ttf, sizeof(font_monogram_ttf));
	if (font_source == nullptr)
	{
		return fatal_error(nullptr, "Failed to load font data");
	}

	constexpr Uint16 font_size = 24;
	const SDL_Color font_color = {.r = 0xf5, .g = 0xf5, .b = 0xf5, .a = SDL_ALPHA_OPAQUE};
	state->font = font_create(state->window, state->device, font_source, font_size, font_color);
	if (state->font == nullptr)
	{
		return fatal_error(nullptr, "Failed to load font");
	}

	state->camera = (camera_t){
		.position = (vector3f_t){.x = 0.F, .y = 20.F, .z = -30.F},
		.target = vector3f_zero(),
		.up = (vector3f_t){.x = 0.F, .y = 1.F, .z = 0.F},
		.fov_y = 45.F,
		.near_plane = 0.1F,
		.far_plane = 100.F,
	};

	SDL_IOStream *vert_source;
	SDL_IOStream *frag_source;

	switch (shader_format(state->device))
	{
		case SDL_GPU_SHADERFORMAT_MSL:
			vert_source = SDL_IOFromConstMem(shader_default_vert_msl, sizeof(shader_default_vert_msl));
			frag_source = SDL_IOFromConstMem(shader_default_frag_msl, sizeof(shader_default_frag_msl));
			break;

		case SDL_GPU_SHADERFORMAT_SPIRV:
			vert_source = SDL_IOFromConstMem(shader_default_vert_spv, sizeof(shader_default_vert_spv));
			frag_source = SDL_IOFromConstMem(shader_default_frag_spv, sizeof(shader_default_frag_spv));
			break;

		case SDL_GPU_SHADERFORMAT_DXIL:
			vert_source = SDL_IOFromConstMem(shader_default_vert_dxil, sizeof(shader_default_vert_dxil));
			frag_source = SDL_IOFromConstMem(shader_default_frag_dxil, sizeof(shader_default_frag_dxil));
			break;

		default:
			SDL_SetError("Unknown shader format: %d", shader_format(state->device));
			return fatal_error(state->window, "Failed to find valid shaders");
	}

	SDL_GPUShader *vert_shader = load_shader(state->device, vert_source,
		SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
	if (vert_shader == nullptr)
	{
		return fatal_error(state->window, "Failed to load vertex shader");
	}

	SDL_GPUShader *frag_shader = load_shader(state->device, frag_source,
		SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
	if (frag_shader == nullptr)
	{
		return fatal_error(state->window, "Failed to load fragment shader");
	}

	state->pipeline = create_pipeline(state->device, state->window, vert_shader, frag_shader);
	if (state->pipeline == nullptr)
	{
		SDL_ReleaseGPUShader(state->device, vert_shader);
		SDL_ReleaseGPUShader(state->device, frag_shader);
		return fatal_error(state->window, "Failed to create pipeline");
	}

	SDL_ReleaseGPUShader(state->device, vert_shader);
	SDL_ReleaseGPUShader(state->device, frag_shader);

	const vector3f_t mesh_size = {.x = 10.F, .y = 10.F, .z = 10.F};
	state->mesh = create_cube(state->device, vector3f_zero(), mesh_size);

	SDL_IOStream *texture_stream = SDL_IOFromConstMem(texture_wall_qoi, sizeof(texture_wall_qoi));
	SDL_Surface *texture = load_qoi(texture_stream);
	if (texture == nullptr)
	{
		return fatal_error(state->window, "Failed to load texture");
	}

	if (!mesh_set_texture(state->mesh, texture))
	{
		SDL_DestroySurface(texture);
		return fatal_error(state->window, "Failed to set mesh texture");
	}
	SDL_DestroySurface(texture);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	app_state_t *state = appstate;

	constexpr float rotation_speed = 64.F / 1000.F;

	const Uint64 current_update = SDL_GetTicks();
	state->dt = current_update - state->last_update;
	state->last_update = current_update;

	state->current_rotation = SDL_fmodf(state->current_rotation + (rotation_speed * state->dt), 360.F);
	const matrix4x4_t mesh_proj = matrix4x4_multiply(
		matrix4x4_create_rotation_x(deg2rad(state->current_rotation)),
		matrix4x4_create_rotation_y(deg2rad(state->current_rotation))
	);

	if (state->time.fps == 0)
	{
		state->time.fps = 1'000.F / (float) state->dt;
	}

	state->time.count++;
	state->time.duration += state->dt;
	if (state->time.duration >= 1'000)
	{
		state->time.fps = state->time.count;
		state->time.count = 0;
		state->time.duration = 0;
	}

	const SDL_FColor clear_color = {.r = 0.12F, .g = 0.12F, .b = 0.12F, .a = 1.F};

	SDL_GPUCommandBuffer *command_buffer = nullptr;
	SDL_GPURenderPass *render_pass = nullptr;
	vector2f_t size;

	if (draw_begin(state->device, state->window, clear_color, &command_buffer, &render_pass, &size))
	{
		const matrix4x4_t proj = matrix4x4_create_perspective(
			deg2rad(state->camera.fov_y),
			size.x / size.y,
			state->camera.near_plane,
			state->camera.far_plane
		);
		const matrix4x4_t view = matrix4x4_create_look_at(
			state->camera.position,
			state->camera.target,
			state->camera.up
		);
		const matrix4x4_t view_proj = matrix4x4_multiply(view, proj);

		const vertex_uniform_data_t vertex_data = {
			.mvp = matrix4x4_multiply(mesh_proj, view_proj),
		};

		SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline);
		SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_data, sizeof(vertex_uniform_data_t));
		mesh_draw(state->mesh, render_pass);

		static constexpr size_t debug_text_len = 256;
		static char debug_text[debug_text_len];
		SDL_snprintf(debug_text, debug_text_len,
#ifndef NDEBUG
			"- debug mode -\n"
#endif
			"%s %s\n"
			"FPS     : %u\n"
			"Video   : %s\n"
			"Audio   : %s\n"
			"Renderer: %s\n"
			"Camera  : %6.2f %6.2f %6.2f\n"
			"Target  : %6.2f %6.2f %6.2f",
			ENGINE_NAME, ENGINE_VERSION,
			state->time.fps,
			video_driver_display_name(SDL_GetCurrentVideoDriver()),
			audio_driver_display_name(SDL_GetCurrentAudioDriver()),
			gpu_device_driver_display_name(SDL_GetGPUDeviceDriver(state->device)),
			state->camera.position.x, state->camera.position.y, state->camera.position.z,
			state->camera.target.x, state->camera.target.y, state->camera.target.z
		);

		font_draw_text(state->font, render_pass, command_buffer, size,
			(vector2f_t){.x = 16.F, .y = 16.F}, debug_text);
	}
	if (!draw_end())
	{
		return fatal_error(state->window, "Rendering failed");
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	app_state_t *state = appstate;

	if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT)
	{
		SDL_SetWindowRelativeMouseMode(state->window, true);
	}
	else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
	{
		SDL_SetWindowRelativeMouseMode(state->window, false);
	}

	if (event->type == SDL_EVENT_MOUSE_MOTION && (int) SDL_GetWindowRelativeMouseMode(state->window))
	{
		state->camera.target.x -= event->motion.xrel * mouse_sensitivity;
		state->camera.target.y -= event->motion.yrel * mouse_sensitivity;
	}

	if (event->type == SDL_EVENT_KEY_DOWN && (int) SDL_GetWindowRelativeMouseMode(state->window))
	{
		switch (event->key.key)
		{
			case SDLK_W:
				state->camera.position.z += move_speed;
				state->camera.target.z += move_speed;
				break;

			case SDLK_S:
				state->camera.position.z -= move_speed;
				state->camera.target.z -= move_speed;
				break;

			case SDLK_A:
				state->camera.position.x += move_speed;
				state->camera.target.x += move_speed;
				break;

			case SDLK_D:
				state->camera.position.x -= move_speed;
				state->camera.target.x -= move_speed;
				break;

			case SDLK_Q:
				state->camera.position.y += move_speed;
				state->camera.target.y += move_speed;
				break;

			case SDLK_E:
				state->camera.position.y -= move_speed;
				state->camera.target.y -= move_speed;
				break;

			default:
				break;
		}
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result)
{
	const app_state_t *state = appstate;

	mesh_destroy(state->mesh);
	font_destroy(state->font);
	assets_destroy(state->assets);

	SDL_ReleaseGPUGraphicsPipeline(state->device, state->pipeline);
	SDL_ReleaseWindowFromGPUDevice(state->device, state->window);

	SDL_DestroyWindow(state->window);
	SDL_DestroyGPUDevice(state->device);

	SDL_free(appstate);
}
