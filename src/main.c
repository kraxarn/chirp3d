#include "appstate.h"
#include "assets.h"
#include "audiodriver.h"
#include "camera.h"
#include "gpu.h"
#include "gpudevicedriver.h"
#include "gpudriver.h"
#include "gpushaderformat.h"
#include "input.h"
#include "logcategory.h"
#include "math.h"
#include "matrix.h"
#include "mesh.h"
#include "physics.h"
#include "resources.h"
#include "shader.h"
#include "shapes.h"
#include "vector.h"
#include "videodriver.h"
#include "windowconfig.h"

#include "cpuinfo.h"
#include "dcimgui.h"
#include "backends/dcimgui_impl_sdl3.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef NDEBUG
#include <SDL3/SDL_messagebox.h>
#endif

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_video.h>

static constexpr auto mouse_sensitivity = 0.0015F;
static constexpr auto move_speed = 250.F;
static constexpr auto max_move_speed = 50.F;
static constexpr auto gravity_y = 100.F;
static constexpr auto jump_speed = 60.F;

static SDL_AppResult fatal_error([[maybe_unused]] SDL_Window *window, const char *message)
{
	SDL_LogCritical(LOG_CATEGORY_CORE, "%s: %s", message, SDL_GetError());
#ifdef NDEBUG
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, message, SDL_GetError(), window);
#endif
	return SDL_APP_FAILURE;
}

static void log_cpu_info()
{
	if (!cpuinfo_initialize())
	{
		// Assume error is already logged to stderr
		return;
	}

	SDL_LogDebug(LOG_CATEGORY_CORE, "CPU: %s", cpuinfo_get_package(0)->name);

	cpuinfo_deinitialize();
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

static bool init_imgui(const app_state_t *state)
{
	CIMGUI_CHECKVERSION();

	if (ImGui_CreateContext(nullptr) == nullptr)
	{
		return SDL_SetError("Failed to initialise ImGui context");
	}

	ImGuiIO *im_io = ImGui_GetIO();
	im_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

	// ImGui wants to own and free the data
	void *font_data = CIM_ALLOC(sizeof(font_maple_mono_nl_regular_ttf));
	SDL_memcpy(font_data, font_maple_mono_nl_regular_ttf, sizeof(font_maple_mono_nl_regular_ttf));

	if (ImFontAtlas_AddFontFromMemoryTTF(im_io->Fonts, font_data, sizeof(font_maple_mono_nl_regular_ttf),
		16.F, nullptr, nullptr) == nullptr)
	{
		return SDL_SetError("Failed to add font");
	}

	if (SDL_GetSystemTheme() == SDL_SYSTEM_THEME_LIGHT)
	{
		ImGui_StyleColorsLight(nullptr);
	}
	else
	{
		ImGui_StyleColorsDark(nullptr);
	}

	const float content_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	ImGuiStyle *style = ImGui_GetStyle();
	ImGuiStyle_ScaleAllSizes(style, content_scale);
	style->FontScaleDpi = content_scale;

	if (!cImGui_ImplSDL3_InitForSDLGPU(state->window))
	{
		return SDL_SetError("Failed to initialise SDL3 backend");
	}

	ImGui_ImplSDLGPU3_InitInfo init_info = {
		.Device = state->device,
		.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(state->device, state->window),
		.MSAASamples = SDL_GPU_SAMPLECOUNT_1,
		.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		.PresentMode = SDL_GPU_PRESENTMODE_VSYNC,
	};

	if (!cImGui_ImplSDLGPU3_Init(&init_info))
	{
		return SDL_SetError("Failed to initialise SDL3 GPU backend");
	}

	return true;
}

static SDL_AppResult build_scene(app_state_t *state)
{
	const auto floor_size = (vector3f_t){.x = 100.F, .y = 2.5F, .z = 100.F};

	const mesh_t *meshes[] = {
		create_cube(state->device, floor_size),
		create_cube(state->device, (vector3f_t){.x = 10.F, .y = 20.F, .z = 10.F}),
	};

	state->num_meshes = SDL_arraysize(meshes);
	state->meshes = (mesh_t **) SDL_malloc(sizeof(meshes));
	SDL_memcpy((void *) state->meshes, (void *) meshes, sizeof(meshes));

	SDL_Surface *light = assets_load_texture(state->assets, "light");
	if (!mesh_set_texture(state->meshes[0], light))
	{
		return fatal_error(state->window, "Failed to load texture");
	}
	SDL_DestroySurface(light);

	SDL_Surface *purple = assets_load_texture(state->assets, "purple");
	if (!mesh_set_texture(state->meshes[1], purple))
	{
		return fatal_error(state->window, "Failed to load texture");
	}
	mesh_set_position(state->meshes[1], (vector3f_t){.x = -20.F, .y = 11.2F, .z = 20.F});
	SDL_DestroySurface(purple);

	const box_config_t floor_config = {
		.motion_type = MOTION_TYPE_STATIC,
		.layer = OBJ_LAYER_STATIC,
		.position = mesh_position(state->meshes[0]),
		.half_extents = floor_size,
		.activate = false,
		.friction = 5.F,
	};
	physics_add_box(state->physics_engine, &floor_config);

	const capsule_config_t player_config = {
		.half_height = 10.F,
		.radius = 5.F,
		.position = vector3f_zero(),
		.motion_type = MOTION_TYPE_DYNAMIC,
		.layer = OBJ_LAYER_PLAYER,
		.activate = false,
		.allowed_dof = DOF_TRANSLATION_3D,
	};
	state->player_body_id = physics_add_capsule(state->physics_engine, &player_config);
	physics_body_set_position(state->physics_engine, state->player_body_id, state->camera.position, true);

	const vector3f_t gravity = {.y = -gravity_y};
	physics_set_gravity(state->physics_engine, gravity);

	physics_optimize(state->physics_engine);

	return SDL_APP_CONTINUE;
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

	const char *assets_path = argc == 2 ? argv[1] : SDL_GetBasePath();
	state->assets = assets_create_from_folder(assets_path);
	if (state->assets == nullptr)
	{
		return fatal_error(nullptr, "Failed to load assets");
	}

	constexpr SDL_InitFlags init_flags = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD;
	if (!SDL_Init(init_flags))
	{
		return fatal_error(nullptr, "Initialisation failed");
	}

	state->last_update = SDL_GetTicks();

	const window_config_t window_config = state->assets->window_config;

	state->window = SDL_CreateWindow(window_config.title,
		window_config.size.x, window_config.size.y,
		window_config_flags(window_config)
	);
	if (state->window == nullptr)
	{
		return fatal_error(nullptr, "Failed to create window");
	}

	state->device = create_device(state->window);
	if (state->device == nullptr)
	{
		return fatal_error(state->window, "Failed to initialise GPU context");
	}

	log_cpu_info();
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

	if (!init_imgui(state))
	{
		return fatal_error(nullptr, "Failed to initialise ImGui");
	}

	vector2i_t depth_size;
	if (!SDL_GetWindowSize(state->window, &depth_size.x, &depth_size.y))
	{
		return fatal_error(state->window, "Failed to get window size");
	}

	state->depth_texture = create_depth_texture(state->device, depth_size);
	if (state->depth_texture == nullptr)
	{
		return fatal_error(state->window, "Failed to initialise depth texture");
	}

	state->camera = camera_create_default();

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
		return fatal_error(state->window, "Failed to initialise pipeline");
	}

	SDL_ReleaseGPUShader(state->device, vert_shader);
	SDL_ReleaseGPUShader(state->device, frag_shader);

	state->physics_engine = physics_create();
	if (state->physics_engine == nullptr)
	{
		return fatal_error(state->window, "Failed to initialise physics engine");
	}

	return build_scene(state);
}

typedef enum [[clang::flag_enum]] debug_overlay_elements_t
{
	DEBUG_OVERLAY_DELTA   = 1 << 0,
	DEBUG_OVERLAY_SYSTEM  = 1 << 1,
	DEBUG_OVERLAY_CAMERA  = 1 << 2,
	DEBUG_OVERLAY_PHYSICS = 1 << 3,
} debug_overlay_elements_t;

static void draw_debug_overlay(const app_state_t *state)
{
	static auto open = true;
	static auto demo_open = false;

#ifdef NDEBUG
	static debug_overlay_elements_t elements = 0;
#else
	static debug_overlay_elements_t elements = DEBUG_OVERLAY_CAMERA | DEBUG_OVERLAY_PHYSICS;
#endif

	if (demo_open)
	{
		ImGui_ShowDemoWindow(&demo_open);
	}

	constexpr auto padding = 16.F;
	constexpr auto alpha = 0.35F;

	constexpr ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav
		| ImGuiWindowFlags_NoMove;

	const ImGuiViewport *viewport = ImGui_GetMainViewport();
	const ImVec2 pos = {
		.x = viewport->WorkPos.x + padding,
		.y = viewport->WorkPos.y + padding,
	};
	ImGui_SetNextWindowPos(pos, ImGuiCond_Always);

	ImGui_SetNextWindowBgAlpha(alpha);

	if (ImGui_Begin("debug_overlay", &open, window_flags))
	{
		ImGui_PushFontFloat(ImGui_GetFont(), 18.F);

#ifndef NDEBUG
		ImGui_Text("- debug mode -");
#endif

		ImGui_Text("%s %s", ENGINE_NAME, ENGINE_VERSION);

		if (ImGui_BeginTable("table_debug", 3, ImGuiTableFlags_None))
		{
			ImGui_TableNextRow();
			ImGui_TableSetColumnIndex(0);
			ImGui_Text("FPS");
			ImGui_TableSetColumnIndex(1);
			ImGui_Text("%u", state->time.fps);

			if ((elements & DEBUG_OVERLAY_DELTA) > 0)
			{
				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Delta");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%.2f ms", state->dt * 1'000.F);
			}

			if ((elements & DEBUG_OVERLAY_SYSTEM) > 0)
			{
				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Video");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%s", video_driver_display_name(SDL_GetCurrentVideoDriver()));

				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Audio");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%s", audio_driver_display_name(SDL_GetCurrentAudioDriver()));

				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Renderer");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%s", gpu_device_driver_display_name(SDL_GetGPUDeviceDriver(state->device)));
			}

			if ((elements & DEBUG_OVERLAY_CAMERA) > 0)
			{
				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Camera");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%-6.2f %-6.2f %-6.2f", state->camera.position.x,
					state->camera.position.y, state->camera.position.z);

				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Target");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%-6.2f %-6.2f %-6.2f", state->camera.target.x,
					state->camera.target.y, state->camera.target.z);
			}

			if ((elements & DEBUG_OVERLAY_PHYSICS) > 0)
			{
				const vector3f_t position = physics_body_position(state->physics_engine, state->player_body_id);
				const vector3f_t velocity = physics_body_linear_velocity(state->physics_engine, state->player_body_id);

				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Position");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%-6.2f %-6.2f %-6.2f", position.x, position.y, position.z);

				ImGui_TableNextRow();
				ImGui_TableSetColumnIndex(0);
				ImGui_Text("Velocity");
				ImGui_TableSetColumnIndex(1);
				ImGui_Text("%-6.2f %-6.2f %-6.2f", velocity.x, velocity.y, velocity.z);
			}

			ImGui_EndTable();
		}

		if (ImGui_BeginPopupContextWindow())
		{
			if (ImGui_MenuItemEx("Debug: Delta time", nullptr,
				(elements & DEBUG_OVERLAY_DELTA) > 0, true))
			{
				elements ^= DEBUG_OVERLAY_DELTA;
			}

			if (ImGui_MenuItemEx("Debug: System info", nullptr,
				(elements & DEBUG_OVERLAY_SYSTEM) > 0, true))
			{
				elements ^= DEBUG_OVERLAY_SYSTEM;
			}

			if (ImGui_MenuItemEx("Debug: Camera position/target", nullptr,
				(elements & DEBUG_OVERLAY_CAMERA) > 0, true))
			{
				elements ^= DEBUG_OVERLAY_CAMERA;
			}
			if (ImGui_MenuItemEx("Debug: Physics properties", nullptr,
				(elements & DEBUG_OVERLAY_PHYSICS) > 0, true))
			{
				elements ^= DEBUG_OVERLAY_PHYSICS;
			}

			if (ImGui_MenuItemEx("Demo window", nullptr, demo_open, true))
			{
				demo_open = (int) demo_open != 0;
			}

			ImGui_EndPopup();
		}

		ImGui_PopFont();
	}
	ImGui_End();
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	app_state_t *state = appstate;

	constexpr auto ns_s = 1'000'000'000.F;
	const Uint64 current_update = SDL_GetTicksNS();
	state->dt = (float) (current_update - state->last_update) / ns_s;
	state->last_update = current_update;

	if (state->time.fps == 0)
	{
		state->time.fps = 1.F / state->dt;
	}

	state->time.count++;
	state->time.duration += state->dt;
	if (state->time.duration >= 1.F)
	{
		state->time.fps = state->time.count;
		state->time.count = 0;
		state->time.duration = 0;
	}

	if (!physics_update(state->physics_engine, state->dt))
	{
		return fatal_error(state->window, "Failed to update physics");
	}

	if (SDL_GetWindowRelativeMouseMode(state->window))
	{
		vector2f_t mouse;
		SDL_GetRelativeMouseState(&mouse.x, &mouse.y);
		if (!ImGui_GetIO()->WantCaptureMouse)
		{
			camera_rotate_x(&state->camera, -(mouse.x * mouse_sensitivity));
			camera_rotate_y(&state->camera, -(mouse.y * mouse_sensitivity));
		}

		if (input_is_down("move_forward"))
		{
			const vector3f_t velocity = camera_to_z(&state->camera, move_speed * state->dt);
			physics_body_add_linear_velocity(state->physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_backward"))
		{
			const vector3f_t velocity = camera_to_z(&state->camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(state->physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_left"))
		{
			const vector3f_t velocity = camera_to_x(&state->camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(state->physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_right"))
		{
			const vector3f_t velocity = camera_to_x(&state->camera, move_speed * state->dt);
			physics_body_add_linear_velocity(state->physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_up"))
		{
			const vector3f_t velocity = camera_to_y(&state->camera, move_speed * state->dt);
			physics_body_add_linear_velocity(state->physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_down"))
		{
			const vector3f_t velocity = camera_to_y(&state->camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(state->physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("jump"))
		{
			const vector3f_t velocity = physics_body_linear_velocity(state->physics_engine, state->player_body_id);
			if (velocity.y > -0.1F && velocity.y < 0.1F)
			{
				const vector3f_t jump_velocity = {
					.x = velocity.x,
					.y = jump_speed,
					.z = velocity.z,
				};
				physics_body_set_linear_velocity(state->physics_engine, state->player_body_id, jump_velocity);
			}
		}
	}

	const vector3f_t min_velocity =
	{
		.x = -max_move_speed,
		.y = -1'000.F,
		.z = -max_move_speed,
	};
	const vector3f_t max_velocity =
	{
		.x = max_move_speed,
		.y = 1'000.F,
		.z = max_move_speed,
	};
	const vector3f_t velocity = physics_body_linear_velocity(state->physics_engine, state->player_body_id);
	const vector3f_t clamped_velocity = vector3f_clamp(velocity, min_velocity, max_velocity);
	physics_body_set_linear_velocity(state->physics_engine, state->player_body_id, clamped_velocity);

	// TODO: There should be a better way to do this, right?
	const vector3f_t player_position = physics_body_position(state->physics_engine, state->player_body_id);
	state->camera.target = vector3f_add(state->camera.target, vector3f_sub(player_position, state->camera.position));
	state->camera.position = player_position;

	const SDL_FColor clear_color = {.r = 0.12F, .g = 0.12F, .b = 0.12F, .a = 1.F};

	cImGui_ImplSDLGPU3_NewFrame();
	cImGui_ImplSDL3_NewFrame();
	ImGui_NewFrame();
	{
		draw_debug_overlay(state);
	}
	ImGui_Render();
	ImDrawData *draw_data = ImGui_GetDrawData();

	SDL_GPUCommandBuffer *command_buffer = nullptr;
	SDL_GPURenderPass *render_pass = nullptr;
	vector2f_t size;

	if (draw_begin(state->device, state->window, clear_color, state->depth_texture,
		draw_data, &command_buffer, &render_pass, &size))
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

		SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline);

		for (size_t i = 0; i < state->num_meshes; i++)
		{
			mesh_draw(state->meshes[i], render_pass, command_buffer, view_proj);
		}

		cImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
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

	cImGui_ImplSDL3_ProcessEvent(event);

	app_state_t *state = appstate;

	if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN
		&& event->button.button == SDL_BUTTON_LEFT
		&& !ImGui_GetIO()->WantCaptureMouse)
	{
		SDL_SetWindowRelativeMouseMode(state->window, true);
	}
	else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
	{
		SDL_SetWindowRelativeMouseMode(state->window, false);
	}

	if (SDL_GetWindowRelativeMouseMode(state->window))
	{
		input_update(event);
	}

	if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		SDL_ReleaseGPUTexture(state->device, state->depth_texture);
		const auto size = (vector2i_t){
			.x = event->window.data1,
			.y = event->window.data2,
		};
		state->depth_texture = create_depth_texture(state->device, size);
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result)
{
	const app_state_t *state = appstate;

	for (size_t i = 0; i < state->num_meshes; i++)
	{
		mesh_destroy(state->meshes[i]);
	}
	SDL_free((void *) state->meshes);

	assets_destroy(state->assets);
	physics_destroy(state->physics_engine);

	cImGui_ImplSDL3_Shutdown();
	cImGui_ImplSDLGPU3_Shutdown();
	ImGui_DestroyContext(nullptr);

	SDL_ReleaseGPUTexture(state->device, state->depth_texture);
	SDL_ReleaseGPUGraphicsPipeline(state->device, state->pipeline);
	SDL_ReleaseWindowFromGPUDevice(state->device, state->window);

	SDL_DestroyWindow(state->window);
	SDL_DestroyGPUDevice(state->device);

	SDL_free(appstate);
}
