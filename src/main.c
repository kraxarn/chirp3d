#include "appstate.h"
#include "gpu.h"
#include "image.h"
#include "logcategory.h"
#include "math.h"
#include "matrix.h"
#include "model.h"
#include "shader.h"
#include "uniformdata.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef NDEBUG
#include <SDL3/SDL_messagebox.h>
#endif

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

static SDL_AppResult fatal_error([[maybe_unused]] SDL_Window *window, const char *message)
{
	SDL_LogCritical(LOG_CATEGORY_CORE, "%s: %s", message, SDL_GetError());
#ifdef NDEBUG
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, message, SDL_GetError(), window);
#endif
	return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] const int argc,
	[[maybe_unused]] char **argv)
{
#ifdef NDEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_WARN);
#else
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#endif

	// TODO: Set these from the game, not engine
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, ENGINE_NAME);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, ENGINE_VERSION);
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

	state->camera = (camera_t){
		.position = (vector3f_t){.x = 0.F, .y = 20.F, .z = -30.F},
		.target = vector3f_zero(),
		.up = (vector3f_t){.x = 0.F, .y = 1.F, .z = 0.F},
		.fov_y = 45.F,
		.near_plane = 0.1F,
		.far_plane = 100.F,
	};

	const char *vert_filename;
	const char *frag_filename;

	switch (shader_format(state->device))
	{
		case SDL_GPU_SHADERFORMAT_MSL:
			vert_filename = "shaders/msl/simple.vert.msl";
			frag_filename = "shaders/msl/simple.frag.msl";
			break;

		case SDL_GPU_SHADERFORMAT_SPIRV:
			vert_filename = "shaders/spv/simple.vert.spv";
			frag_filename = "shaders/spv/simple.frag.spv";
			break;

		case SDL_GPU_SHADERFORMAT_DXIL:
			vert_filename = "shaders/dxil/simple.vert.dxil";
			frag_filename = "shaders/dxil/simple.frag.dxil";
			break;

		default:
			SDL_SetError("Unknown shader format: %d", shader_format(state->device));
			return fatal_error(state->window, "Failed to find valid shaders");
	}

	SDL_GPUShader *vert_shader = load_shader(state->device, vert_filename,
		SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
	if (vert_shader == nullptr)
	{
		return fatal_error(state->window, "Failed to load vertex shader");
	}

	SDL_GPUShader *frag_shader = load_shader(state->device, frag_filename,
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

	char *texture_path = nullptr;
	SDL_asprintf(&texture_path, "%sresources/textures/wall.qoi", SDL_GetBasePath());
	SDL_Surface *texture = load_qoi(texture_path);
	if (texture == nullptr)
	{
		SDL_free(texture_path);
		return fatal_error(state->window, "Failed to load texture");
	}
	SDL_free(texture_path);

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

	constexpr float rotation_speed = 32.F / 1000.F;

	const Uint64 current_update = SDL_GetTicks();
	const Uint64 elapsed = current_update - state->last_update;
	state->last_update = current_update;

	state->current_rotation = SDL_fmodf(state->current_rotation + (rotation_speed * (float) elapsed), 360.F);
	const matrix4x4_t mesh_proj = matrix4x4_create_rotation_y(deg2rad(state->current_rotation));

	const SDL_FColor clear_color = {.r = 0.F, .g = 0.F, .b = 0.F, .a = 1.F};

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

		vertex_uniform_data_t vertex_data = {
			.mvp = matrix4x4_multiply(mesh_proj, view_proj),
			.color = (SDL_FColor){.r = 0.6F, .g = 0.6F, .b = 0.6F, .a = 1.F},
			.camera_position = (vector4f_t){
				.x = state->camera.position.x,
				.y = state->camera.position.y,
				.z = state->camera.position.z,
			},
		};
		SDL_memcpy(vertex_data.lights, state->lights, sizeof(point_light_t) * state->num_lights);

		SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline);
		SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_data, sizeof(vertex_uniform_data_t));
		mesh_draw(state->mesh, render_pass);
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

	const app_state_t *state = appstate;

	if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT)
	{
		SDL_SetWindowRelativeMouseMode(state->window, true);
	}
	else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
	{
		SDL_SetWindowRelativeMouseMode(state->window, false);
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result)
{
	const app_state_t *state = appstate;

	mesh_destroy(state->mesh);

	SDL_ReleaseGPUGraphicsPipeline(state->device, state->pipeline);
	SDL_ReleaseWindowFromGPUDevice(state->device, state->window);

	SDL_DestroyWindow(state->window);
	SDL_DestroyGPUDevice(state->device);

	SDL_free(appstate);
}
