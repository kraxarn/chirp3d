#include "appstate.h"
#include "assets.h"
#include "camera.h"
#include "ecs.h"
#include "gpu.h"
#include "gpudriver.h"
#include "gpushaderformat.h"
#include "input.h"
#include "logcategory.h"
#include "math.h"
#include "matrix.h"
#include "model.h"
#include "physics.h"
#include "physicsconfig.h"
#include "resources.h"
#include "scriptengine.h"
#include "shader.h"
#include "systeminfo.h"
#include "systems.h"
#include "vector.h"
#include "windowconfig.h"
#include "ui/debugoverlay.h"

#include "dcimgui.h"
#include "flecs.h"
#include "backends/dcimgui_impl_sdl3.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef NDEBUG
#include <SDL3/SDL_messagebox.h>
#endif

// For use with RenderDoc
#ifdef FORCE_X11
#include <SDL3/SDL_hints.h>
#endif

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_video.h>

static constexpr auto mouse_sensitivity = 0.0015F;

static SDL_AppResult fatal_error(const char *message)
{
	SDL_LogCritical(LOG_CATEGORY_CORE, "%s: %s", message, SDL_GetError());
#ifdef NDEBUG
	SDL_Window *window = ecs_mut_data_ptr("chirp.Window");
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, message, SDL_GetError(), window);
#endif
	return SDL_APP_FAILURE;
}

static void log_system_info(SDL_GPUDevice *device)
{
	SDL_LogDebug(LOG_CATEGORY_CORE, "Platform: %s",
		system_info_platform());

	SDL_LogDebug(LOG_CATEGORY_CORE, "CPU: %s",
		system_info_cpu_name());

	SDL_LogDebug(LOG_CATEGORY_CORE, "GPU: %s (%s)",
		system_info_gpu_name(device), system_info_gpu_driver(device));
}

static bool init_imgui()
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

	SDL_Window *window = ecs_mut_data_ptr("chirp.Window");

	if (!cImGui_ImplSDL3_InitForSDLGPU(window))
	{
		return SDL_SetError("Failed to initialise SDL3 backend");
	}

	SDL_GPUDevice *gpu_device = ecs_mut_data_ptr("chirp.GpuDevice");

	ImGui_ImplSDLGPU3_InitInfo init_info = {
		.Device = gpu_device,
		.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
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
	const auto floor_size = (vector3f_t){.x = 100.F, .y = 0.F, .z = 100.F};

	physics_engine_t *physics_engine = ecs_mut_data("chirp.PhysicsEngine");
	const physics_config_t *physics_config = ecs_const_data("chirp.PhysicsConfig");

	const box_config_t floor_config = {
		.half_extents = floor_size,
		.friction = 15.F,
		.body = (body_config_t){
			.motion_type = MOTION_TYPE_STATIC,
			.layer = OBJ_LAYER_STATIC,
			.position = vector3f_zero(),
			.activate = false,
		},
	};
	physics_add_box(physics_engine, &floor_config);

	const camera_t *camera = ecs_const_data("chirp.Camera");

	const capsule_config_t player_config = {
		.half_height = 0.5F,
		.radius = 1.F,
		.allowed_dof = DOF_TRANSLATION_3D,
		.body = (body_config_t){
			.position = vector3f_zero(),
			.motion_type = MOTION_TYPE_DYNAMIC,
			.layer = OBJ_LAYER_DYNAMIC,
			.activate = false,
		},
	};
	state->player_body_id = physics_add_capsule(physics_engine, &player_config);
	physics_body_set_position(physics_engine, state->player_body_id, camera->position, true);

	const vector3f_t gravity = {.y = -physics_config->gravity_y};
	physics_set_gravity(physics_engine, gravity);

	physics_optimize(physics_engine);

	return SDL_APP_CONTINUE;
}

[[nodiscard]]
static bool sdl_supported()
{
	const int linked = SDL_GetVersion();

	if (linked < SDL_VERSIONNUM(3, 4, 0))
	{
		return SDL_SetError("SDL 3.4 or newer is required");
	}

	constexpr int compiled = SDL_VERSION;

	// Micro is bugfixes only, so just ignore it
	if (SDL_VERSIONNUM_MAJOR(linked) != SDL_VERSIONNUM_MAJOR(compiled)
		|| SDL_VERSIONNUM_MINOR(linked) != SDL_VERSIONNUM_MINOR(compiled))
	{
		SDL_LogWarn(LOG_CATEGORY_CORE,
			"Binary is linked against SDL %d.%d, but running against SDL %d.%d",
			SDL_VERSIONNUM_MAJOR(linked), SDL_VERSIONNUM_MINOR(linked),
			SDL_VERSIONNUM_MAJOR(compiled), SDL_VERSIONNUM_MINOR(compiled)
		);
	}

	return true;
}

static ecs_entity_t load_model(const char *name)
{
	const assets_t *assets = ecs_const_data("chirp.Assets");
	gpu_device_t *gpu_device = ecs_mut_data_ptr("chirp.GpuDevice");

	model_t model;
	if (!assets_load_model(assets, gpu_device, name, &model))
	{
		return 0;
	}

	const ecs_entity_desc_t parent_desc = {
		.name = "Model",
	};
	const ecs_entity_t parent = ecs_entity_init(ecs_world(), &parent_desc);

	const ecs_entity_desc_t entity_desc = {
		.name = name,
	};
	const ecs_entity_t entity = ecs_entity_init(ecs_world(), &entity_desc);
	ecs_add_pair(ecs_world(), entity, EcsChildOf, parent);

	const ecs_id_t model_id = ecs_lookup(ecs_world(), "chirp.Model");
	ecs_set_id(ecs_world(), entity, model_id, sizeof(model_t), &model);

	return entity;
}

static ecs_entity_t create_instance(const ecs_entity_t entity, const size_t node_index)
{
	// TODO: Make prefab

	const ecs_entity_t instance = ecs_new(ecs_world());

	char *instance_name = nullptr;
	SDL_asprintf(&instance_name, "%s#%ld",
		ecs_get_name(ecs_world(), entity),
		instance
	);
	ecs_set_name(ecs_world(), instance, instance_name);
	SDL_free(instance_name);

	const ecs_entity_desc_t parent_desc = {
		.name = "Instance",
	};
	const ecs_entity_t parent = ecs_entity_init(ecs_world(), &parent_desc);
	ecs_add_pair(ecs_world(), instance, EcsChildOf, parent);

	const ecs_id_t instance_of_id = ecs_lookup(ecs_world(), "chirp.InstanceOf");
	ecs_set_id(ecs_world(), instance, ecs_pair(instance_of_id, entity),
		sizeof(size_t), &node_index);

	const projection_t projection = {.rebuild = true};
	const ecs_id_t projection_id = ecs_lookup(ecs_world(), "chirp.Projection");
	ecs_set_id(ecs_world(), instance, projection_id,
		sizeof(projection_t), &projection);

	return instance;
}

static bool query_cleanup(ecs_query_t *query)
{
	ecs_query_fini(query);
	return false;
}

#define _query_name__(x, y) x##y
#define _query_name_(x, y) _query_name__(x, y)
#define _query_name(x) _query_name_(x, __COUNTER__)

#define _query(e, d, q)										\
	const ecs_query_desc_t d = {.expr = e};					\
	ecs_query_t *q = ecs_query_init(ecs_world(), &d); 		\
	for (ecs_iter_t iter = ecs_query_iter(ecs_world(), q);	\
		ecs_query_next(&iter) || query_cleanup(q);)

#define query(expr) _query(expr, _query_name(_d), _query_name(_q))

SDL_AppResult SDL_AppInit(void **appstate, const int argc, char **argv)
{
	if (!system_info_cpu_supported())
	{
		return fatal_error("Unsupported CPU");
	}

#ifdef NDEBUG
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
#else
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
#endif

	if (!sdl_supported())
	{
		return fatal_error("Unsupported SDL version");
	}

	app_state_t *state = SDL_calloc(1, sizeof(app_state_t));
	if (state == nullptr)
	{
		return fatal_error("Memory allocation failed");
	}
	*appstate = state;

	ecs_create();

	const ecs_entity_t assets_id = ecs_lookup(ecs_world(), "chirp.Assets");
	system_register_assets();
	const assets_t *assets = ecs_get_id(ecs_world(), assets_id, assets_id);

#ifdef FORCE_X11
	if (!SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11"))
	{
		return fatal_error(nullptr, "Failed to set X11 hint");
	}
#endif

	constexpr SDL_InitFlags init_flags = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD;
	if (!SDL_Init(init_flags))
	{
		return fatal_error("Initialisation failed");
	}

	const ecs_entity_t init_id = ecs_lookup(ecs_world(), "chirp.Init");
	ecs_set_id(ecs_world(), init_id, init_id, sizeof(SDL_InitFlags), &init_flags);

	state->last_update = SDL_GetTicks();

	const ecs_entity_t window_config_id = ecs_lookup(ecs_world(), "chirp.WindowConfig");
	const window_config_t window_config = assets_window_config(assets);
	ecs_set_id(ecs_world(), window_config_id, window_config_id,
		sizeof(window_config_t), &window_config);

	SDL_Window *window = SDL_CreateWindow(window_config.title,
		window_config.size.x, window_config.size.y,
		window_config_flags(window_config)
	);
	if (window == nullptr)
	{
		return fatal_error("Failed to create window");
	}

	const ecs_entity_t window_id = ecs_lookup(ecs_world(), "chirp.Window");
	ecs_set_id(ecs_world(), window_id, window_id,
		sizeof(SDL_Window*), (void*) &window);

	SDL_GPUDevice *gpu_device = create_device(window);
	if (gpu_device == nullptr)
	{
		return fatal_error("Failed to initialise GPU context");
	}

	const ecs_entity_t gpu_device_id = ecs_lookup(ecs_world(), "chirp.GpuDevice");
	ecs_set_id(ecs_world(), gpu_device_id, gpu_device_id,
		sizeof(SDL_GPUDevice*), (void*) &gpu_device);

	log_system_info(gpu_device);

	if (SDL_GetLogPriority(LOG_CATEGORY_CORE) >= SDL_LOG_PRIORITY_VERBOSE)
	{
		char *gpu_drivers = gpu_driver_names();
		SDL_LogDebug(LOG_CATEGORY_CORE, "Available GPU drivers: %s", gpu_drivers);

		char *shader_formats = shader_format_names(gpu_device);
		if (shader_formats != nullptr)
		{
			SDL_LogDebug(LOG_CATEGORY_CORE, "Available shader formats for %s: %s",
				SDL_GetGPUDeviceDriver(gpu_device), shader_formats);
		}
	}

	if (!SDL_SetGPUSwapchainParameters(gpu_device, window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
	{
		SDL_LogWarn(LOG_CATEGORY_CORE, "VSync not supported: %s", SDL_GetError());
	}

	if (!init_imgui())
	{
		return fatal_error("Failed to initialise ImGui");
	}

	vector2i_t depth_size;
	if (!SDL_GetWindowSize(window, &depth_size.x, &depth_size.y))
	{
		return fatal_error("Failed to get window size");
	}

	SDL_GPUTexture *depth_texture = create_depth_texture(gpu_device, depth_size);
	if (depth_texture == nullptr)
	{
		return fatal_error("Failed to initialise depth texture");
	}

	const ecs_entity_t depth_texture_id = ecs_lookup(ecs_world(), "chirp.DepthTexture");
	ecs_set_id(ecs_world(), depth_texture_id, depth_texture_id,
		sizeof(SDL_GPUTexture*), (const void*) &depth_texture);

	const ecs_entity_t camera_id = ecs_lookup(ecs_world(), "chirp.Camera");
	const camera_t camera = camera_create_default();
	ecs_set_id(ecs_world(), camera_id, camera_id,
		sizeof(camera_t), &camera);

	const ecs_entity_t physics_config_id = ecs_lookup(ecs_world(), "chirp.PhysicsConfig");
	const physics_config_t physics_config = physics_config_create_default();
	ecs_set_id(ecs_world(), physics_config_id, physics_config_id,
		sizeof(physics_config_t), &physics_config);

	SDL_IOStream *vert_source;
	SDL_IOStream *frag_source;

	switch (shader_format(gpu_device))
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
			SDL_SetError("Unknown shader format: %d", shader_format(gpu_device));
			return fatal_error("Failed to find valid shaders");
	}

	SDL_GPUShader *vert_shader = load_shader(gpu_device, vert_source,
		SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
	if (vert_shader == nullptr)
	{
		return fatal_error("Failed to load vertex shader");
	}

	SDL_GPUShader *frag_shader = load_shader(gpu_device, frag_source,
		SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
	if (frag_shader == nullptr)
	{
		return fatal_error("Failed to load fragment shader");
	}

	SDL_GPUGraphicsPipeline *pipeline = create_pipeline(gpu_device, window, vert_shader, frag_shader);
	if (pipeline == nullptr)
	{
		SDL_ReleaseGPUShader(gpu_device, vert_shader);
		SDL_ReleaseGPUShader(gpu_device, frag_shader);
		return fatal_error("Failed to initialise pipeline");
	}

	const ecs_entity_t pipeline_id = ecs_lookup(ecs_world(), "chirp.GpuGraphicsPipeline");
	ecs_set_id(ecs_world(), pipeline_id, pipeline_id,
		sizeof(SDL_GPUGraphicsPipeline*), (const void*) &pipeline);

	SDL_ReleaseGPUShader(gpu_device, vert_shader);
	SDL_ReleaseGPUShader(gpu_device, frag_shader);

	physics_engine_t physics_engine;
	if (!physics_create(&physics_engine))
	{
		return fatal_error("Failed to initialise physics engine");
	}

	const ecs_entity_t physics_engine_id = ecs_lookup(ecs_world(), "chirp.PhysicsEngine");
	ecs_set_id(ecs_world(), physics_engine_id, physics_engine_id,
		sizeof(physics_engine_t), &physics_engine);

	{
		const ecs_entity_t blaster = load_model("blaster");
		if (blaster == 0)
		{
			return fatal_error("Failed to load model");
		}

		const ecs_entity_t blaster_instance = create_instance(blaster, 0);

		const rotation_t rotation = {
			.x = 0.F,
			.y = 90.F,
			.z = 0.F,
		};
		const ecs_id_t rotation_id = ecs_lookup(ecs_world(), "chirp.Rotation");
		ecs_set_id(ecs_world(), blaster_instance, rotation_id, sizeof(rotation_t), &rotation);

		const position_t position = {
			.x = 5.F,
			.y = 1.F,
			.z = -5.F,
		};
		const ecs_id_t position_id = ecs_lookup(ecs_world(), "chirp.Position");
		ecs_set_id(ecs_world(), blaster_instance, position_id, sizeof(position_t), &position);
	}
	{
		const ecs_entity_t scene = load_model("scene");
		if (scene == 0)
		{
			return fatal_error("Failed to load scene");
		}

		const ecs_id_t scene_id = ecs_lookup(ecs_world(), "chirp.Scene");
		ecs_add_id(ecs_world(), scene, scene_id);
	}
	{
		load_model("bullet");
	}

	// TODO
	// const vector3f_t spawn_position = model_node_position(array_ptr(state->models, 1), "Spawn");
	// SDL_Log("Spawn: %f %f %f", spawn_position.x, spawn_position.y, spawn_position.z);

	script_engine_create();

	SDL_IOStream *script_stream = assets_load_script(assets, "main");
	if (script_stream == nullptr)
	{
		return fatal_error("Failed to load script");
	}

	if (!script_engine_exec("main", script_stream, true))
	{
		return fatal_error("Failed to execute script");
	}

	return build_scene(state);
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

	ecs_progress(ecs_world(), state->dt);

	physics_engine_t *physics_engine = ecs_mut_data("chirp.PhysicsEngine");
	const physics_config_t *physics_config = ecs_const_data("chirp.PhysicsConfig");

	if (!physics_update(physics_engine, state->dt))
	{
		return fatal_error("Failed to update physics");
	}

	SDL_Window *window = ecs_mut_data_ptr("chirp.Window");
	camera_t *camera = ecs_mut_data("chirp.Camera");

	if (SDL_GetWindowRelativeMouseMode(window))
	{
		vector2f_t mouse;
		SDL_GetRelativeMouseState(&mouse.x, &mouse.y);
		camera_rotate_x(camera, -(mouse.x * mouse_sensitivity));
		camera_rotate_y(camera, -(mouse.y * mouse_sensitivity));

		const float move_speed = physics_config->move_speed;
		const float jump_speed = physics_config->jump_speed;

		if (input_is_down("move_forward"))
		{
			const vector3f_t velocity = camera_to_z(camera, move_speed * state->dt);
			physics_body_add_linear_velocity(physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_backward"))
		{
			const vector3f_t velocity = camera_to_z(camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_left"))
		{
			const vector3f_t velocity = camera_to_x(camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_right"))
		{
			const vector3f_t velocity = camera_to_x(camera, move_speed * state->dt);
			physics_body_add_linear_velocity(physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_up"))
		{
			const vector3f_t velocity = camera_to_y(camera, move_speed * state->dt);
			physics_body_add_linear_velocity(physics_engine, state->player_body_id, velocity);
		}

		if (input_is_down("move_down"))
		{
			const vector3f_t velocity = camera_to_y(camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(physics_engine, state->player_body_id, velocity);
		}

		if (input_is_pressed("jump"))
		{
			const vector3f_t velocity = physics_body_linear_velocity(physics_engine, state->player_body_id);
			if (velocity.y > -0.1F && velocity.y < 0.1F)
			{
				const vector3f_t jump_velocity = {
					.x = velocity.x,
					.y = jump_speed,
					.z = velocity.z,
				};
				physics_body_set_linear_velocity(physics_engine, state->player_body_id, jump_velocity);
			}
		}

		if (input_is_pressed("shoot"))
		{
			static constexpr float firepower = 100.F;

			const ecs_entity_t bullet = ecs_lookup(ecs_world(), "Model.bullet");
			const ecs_entity_t entity = create_instance(bullet, 0);

			position_t position = {};
			query("[none] (chirp.InstanceOf, Model.blaster), [in] chirp.Position")
			{
				position = *ecs_field(&iter, position_t, 1);
			}

			const ecs_id_t position_id = ecs_lookup(ecs_world(), "chirp.Position");
			ecs_set_id(ecs_world(), entity, position_id, sizeof(position_t), &position);

			const rotation_t rotation = vector3f_zero();
			const ecs_id_t rotation_id = ecs_lookup(ecs_world(), "chirp.Rotation");
			ecs_set_id(ecs_world(), entity, rotation_id, sizeof(rotation_t), &rotation);

			const cylinder_config_t config = {
				.half_height = 0.1F,
				.radius = 0.5F,
				.body = (body_config_t){
					.position = position,
					.motion_type = MOTION_TYPE_DYNAMIC,
					.layer = OBJ_LAYER_DYNAMIC,
					.activate = false,
				},
			};
			const physics_body_id_t body_id = physics_add_cylinder(physics_engine, &config);

			const ecs_id_t physics_body_id = ecs_lookup(ecs_world(), "chirp.PhysicsBody");
			ecs_set_id(ecs_world(), entity, physics_body_id, sizeof(physics_body_id_t), &body_id);

			const vector3f_t forward = vector3f_normalize(vector3f_sub(camera->target, position));
			const vector3f_t velocity = vector3f_scale(forward, firepower);
			physics_body_set_linear_velocity(physics_engine, body_id, velocity);
			physics_body_set_rotation(physics_engine, body_id, vector4f_normalize((vector4f_t){
				.x = 0.5F,
			}), true);
		}
	}

	const vector3f_t min_velocity =
	{
		.x = -physics_config->max_move_speed,
		.y = -1'000.F,
		.z = -physics_config->max_move_speed,
	};
	const vector3f_t max_velocity =
	{
		.x = physics_config->max_move_speed,
		.y = 1'000.F,
		.z = physics_config->max_move_speed,
	};
	const vector3f_t velocity = physics_body_linear_velocity(physics_engine, state->player_body_id);
	const vector3f_t clamped_velocity = vector3f_clamp(velocity, min_velocity, max_velocity);
	physics_body_set_linear_velocity(physics_engine, state->player_body_id, clamped_velocity);

	// TODO: There should be a better way to do this, right?
	const vector3f_t player_position = physics_body_position(physics_engine, state->player_body_id);
	camera->target = vector3f_add(camera->target, vector3f_sub(player_position, camera->position));
	camera->position = player_position;

	const vector3f_t forward_n = vector3f_normalize(vector3f_sub(camera->target, camera->position));
	const vector3f_t right_n = vector3f_normalize(vector3f_cross(forward_n, camera->up));
	const vector3f_t up_n = vector3f_normalize(camera->up);

	vector3f_t weapon_position = camera->position;
	weapon_position = vector3f_add(weapon_position, vector3f_scale(forward_n, 0.2F));
	weapon_position = vector3f_add(weapon_position, vector3f_scale(right_n, 0.25F));
	weapon_position = vector3f_add(weapon_position, vector3f_scale(up_n, -0.2F));
	query("[none] (chirp.InstanceOf, Model.blaster), [out] chirp.Position,"
		"[out] chirp.Rotation, [out] chirp.Projection")
	{
		*ecs_field(&iter, position_t, 1) = weapon_position;
		*ecs_field(&iter, rotation_t, 2) = (rotation_t){
			.x = SDL_asinf(forward_n.y),
			.y = SDL_atan2f(-forward_n.z, forward_n.x) - (SDL_PI_F * 0.5F),
			.z = 0.0F,
		};
		ecs_field(&iter, projection_t, 3)->rebuild = true;
	}

	query("[in] chirp.PhysicsBody, [out] chirp.Position, [out] chirp.Rotation")
	{
		const physics_body_id_t body_id = *ecs_field(&iter, physics_body_id_t, 0);
		auto position = ecs_field(&iter, position_t, 1);
		auto rotation = ecs_field(&iter, rotation_t, 2);

		*position = physics_body_position(physics_engine, body_id);

		const vector4f_t jph_rotation = physics_body_rotation(physics_engine, body_id);
		*rotation = (rotation_t){
			.x = jph_rotation.x,
			.y = jph_rotation.y,
			.z = jph_rotation.z,
		};
	}

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

	SDL_GPUDevice *gpu_device = ecs_mut_data_ptr("chirp.GpuDevice");
	SDL_GPUTexture *depth_texture = ecs_mut_data_ptr("chirp.DepthTexture");

	if (draw_begin(gpu_device, window, clear_color, depth_texture,
		draw_data, &command_buffer, &render_pass, &size))
	{
		const matrix4x4_t proj = matrix4x4_create_perspective(
			deg2rad(camera->fov_y),
			size.x / size.y,
			camera->near_plane,
			camera->far_plane
		);
		const matrix4x4_t view = matrix4x4_create_look_at(
			camera->position,
			camera->target,
			camera->up
		);
		const matrix4x4_t view_proj = matrix4x4_multiply(view, proj);

		SDL_GPUGraphicsPipeline *pipeline = ecs_mut_data_ptr("chirp.GpuGraphicsPipeline");
		SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

		query("[in] chirp.Model, [none] chirp.Scene")
		{
			const model_t *model = ecs_field(&iter, model_t, 0);
			model_draw(model, render_pass, command_buffer, view_proj);
		}

		query("[in] (chirp.InstanceOf, *), [inout] chirp.Projection,"
			"[in] ?chirp.Scale, [in] ?chirp.Rotation, [in] ?chirp.Position")
		{
			const ecs_id_t pair_id = ecs_field_id(&iter, 0);
			auto projection = ecs_field(&iter, projection_t, 1);

			const ecs_id_t model_id = ecs_lookup(ecs_world(), "chirp.Model");
			const ecs_entity_t model_entity = ecs_pair_second(ecs_world(), pair_id);
			const model_t *model = ecs_get_id(ecs_world(), model_entity, model_id);
			const size_t *index = ecs_field(&iter, size_t, 0);

			if (projection->rebuild)
			{
				Uint8 mul = 0;
				projection->value = matrix4x4_zero();

				const scale_t *scale = ecs_field(&iter, scale_t, 2);
				if (scale != nullptr)
				{
					projection->value = mul++ > 0
						? matrix4x4_multiply(projection->value, matrix4x4_create_scale(*scale))
						: matrix4x4_create_scale(*scale);
				}

				const scale_t *rotation = ecs_field(&iter, rotation_t, 3);
				if (rotation != nullptr)
				{
					const matrix4x4_t transform = matrix4x4_multiply_n((matrix4x4_t[]){
						matrix4x4_create_rotation_x(rotation->x),
						matrix4x4_create_rotation_y(rotation->y),
						matrix4x4_create_rotation_z(rotation->z),
					}, 3);

					projection->value = mul++ > 0
						? matrix4x4_multiply(projection->value, transform)
						: transform;
				}

				const scale_t *position = ecs_field(&iter, position_t, 4);
				if (position != nullptr)
				{
					projection->value = mul > 0
						? matrix4x4_multiply(projection->value, matrix4x4_create_translation(*position))
						: matrix4x4_create_translation(*position);
				}

				projection->rebuild = false;
			}

			model_draw_indexed(model, *index, render_pass, command_buffer,
				matrix4x4_multiply(projection->value, view_proj));
		}

		cImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
	}
	if (!draw_end())
	{
		return fatal_error("Rendering failed");
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	cImGui_ImplSDL3_ProcessEvent(event);

	SDL_Window *window = ecs_mut_data_ptr("chirp.Window");

	if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN
		&& event->button.button == SDL_BUTTON_LEFT
		&& !ImGui_GetIO()->WantCaptureMouse)
	{
		SDL_SetWindowRelativeMouseMode(window, true);
	}
	else if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
	{
		SDL_SetWindowRelativeMouseMode(window, false);
	}

	if (SDL_GetWindowRelativeMouseMode(window))
	{
		input_update(event);
	}

	if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		SDL_GPUDevice *gpu_device = ecs_mut_data_ptr("chirp.GpuDevice");
		const auto depth_texture = (SDL_GPUTexture**) ecs_const_data("chirp.DepthTexture");

		SDL_ReleaseGPUTexture(gpu_device, *depth_texture);
		const auto size = (vector2i_t){
			.x = event->window.data1,
			.y = event->window.data2,
		};
		*depth_texture = create_depth_texture(gpu_device, size);
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result)
{
	query("[in] chirp.Model")
	{
		const model_t *models = ecs_field(&iter, model_t, 0);
		for (Sint32 i = 0; i < iter.count; i++)
		{
			model_destroy(models + i);
		}
	}

	assets_destroy(ecs_const_data("chirp.Assets"));
	physics_destroy(ecs_const_data("chirp.PhysicsEngine"));
	script_engine_destroy();

	cImGui_ImplSDL3_Shutdown();
	cImGui_ImplSDLGPU3_Shutdown();
	ImGui_DestroyContext(nullptr);

	SDL_Window *window = ecs_mut_data_ptr("chirp.Window");
	SDL_GPUDevice *gpu_device = ecs_mut_data_ptr("chirp.GpuDevice");
	SDL_GPUGraphicsPipeline *pipeline = ecs_mut_data_ptr("chirp.GpuGraphicsPipeline");
	SDL_GPUTexture *depth_texture = ecs_mut_data_ptr("chirp.DepthTexture");

	SDL_ReleaseGPUTexture(gpu_device, depth_texture);
	SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
	SDL_ReleaseWindowFromGPUDevice(gpu_device, window);

	SDL_DestroyWindow(window);
	SDL_DestroyGPUDevice(gpu_device);

	ecs_destroy();
	SDL_free(appstate);
}
