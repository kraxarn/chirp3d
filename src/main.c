#include "appstate.h"
#include "assets.h"
#include "camera.h"
#include "ecs.h"
#include "input.h"
#include "logcategory.h"
#include "model.h"
#include "physics.h"
#include "physicsconfig.h"
#include "scriptengine.h"
#include "systeminfo.h"
#include "termcolors.h"
#include "vector.h"
#include "ecs/components.h"
#include "ecs/entities.h"
#include "ecs/events.h"
#include "ecs/tags.h"

#include "dcimgui.h"
#include "flecs.h"
#include "backends/dcimgui_impl_sdl3.h"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef NDEBUG
#include <SDL3/SDL_messagebox.h>
#endif

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
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
	SDL_Window *window = ecs_get_id_ptr(EcsWindow);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, message, SDL_GetError(), window);
#endif
	return SDL_APP_FAILURE;
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

static Uint16 fix_entity_name(const char *name)
{
	Uint16 changes = 0;

	char *str;
	while ((str = SDL_strchr(name, '.')) != nullptr)
	{
		*str = '_';
		changes++;
	}

	return changes;
}

static ecs_entity_t load_model(const assets_t *assets, gpu_device_t *gpu_device, const char *name)
{
	model_t model;
	if (!assets_load_model(assets, gpu_device, name, &model))
	{
		return 0;
	}

	// TODO: Needed because data is fetched when we create an instance, without progressing the ecs
	ecs_defer_suspend(ecs_world());

	const ecs_entity_desc_t parent_desc = {
		.name = "Model",
	};
	const ecs_entity_t parent = ecs_entity_init(ecs_world(), &parent_desc);

	const ecs_entity_desc_t entity_desc = {
		.name = name,
	};
	const ecs_entity_t entity = ecs_entity_init(ecs_world(), &entity_desc);
	ecs_add_pair(ecs_world(), entity, EcsChildOf, parent);

	ecs_set_id(ecs_world(), entity, EcsModel,
		sizeof(model_t), &model);

	for (size_t i = 0; i < model.node_count; i++)
	{
		char *node_name = SDL_strdup(model_node_name(&model, i));
		if (fix_entity_name(node_name) > 0)
		{
			SDL_LogWarn(LOG_CATEGORY_MODEL, "Renamed invalid entity name \"%s\" to \"%s\" in \"%s\"",
				model_node_name(&model, i), node_name, name);
		}
		const ecs_entity_t node = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = node_name,
		});
		SDL_free(node_name);

		ecs_add_pair(ecs_world(), node, EcsChildOf, entity);

		const position_t position = model_node_translation(&model, i);
		ecs_set_id(ecs_world(), node, EcsPosition,
			sizeof(position_t), &position);
	}

	ecs_defer_resume(ecs_world());
	return entity;
}

static ecs_entity_t create_instance(const ecs_entity_t model)
{
	SDL_assert(model != 0);

	const ecs_entity_t instance = ecs_new(ecs_world());

	char *instance_name = nullptr;
	SDL_asprintf(&instance_name, "%s#%ld",
		ecs_get_name(ecs_world(), model),
		instance
	);
	ecs_set_name(ecs_world(), instance, instance_name);
	SDL_free(instance_name);

	const ecs_entity_desc_t parent_desc = {
		.name = "Instance",
	};
	const ecs_entity_t parent = ecs_entity_init(ecs_world(), &parent_desc);
	ecs_add_pair(ecs_world(), instance, EcsChildOf, parent);

	ecs_add_pair(ecs_world(), instance, EcsInstanceOf, model);

	ecs_iter_t iter = ecs_children(ecs_world(), model);
	while (ecs_children_next(&iter))
	{
		for (Sint32 i = 0; i < iter.count; i++)
		{
			const ecs_entity_t child = iter.entities[i];

			const ecs_entity_t node = ecs_new_w_pair(ecs_world(), EcsChildOf, instance);
			char *node_name = nullptr;
			SDL_asprintf(&node_name, "%s#%ld", ecs_get_name(ecs_world(), child), node);
			ecs_set_name(ecs_world(), node, node_name);
			SDL_free(node_name);

			const projection_t projection = {.rebuild = true};
			ecs_set_id(ecs_world(), node, EcsProjection,
				sizeof(projection_t), &projection);

			ecs_add_pair(ecs_world(), node, EcsInstanceOf, child);
		}
	}

	return instance;
}

static void log_spawn_position(ecs_iter_t *iter)
{
	const position_t *spawn_position = ecs_field(iter, position_t, 0);
	SDL_Log("Spawn: %f %f %f", spawn_position->x, spawn_position->y, spawn_position->z);
}

static void build_scene(ecs_iter_t *iter)
{
	const assets_t *assets = ecs_field(iter, assets_t, 0);
	SDL_GPUDevice *gpu_device = *ecs_field(iter, gpu_device_t*, 1);
	physics_engine_t *physics_engine = ecs_field(iter, physics_engine_t, 2);
	const physics_config_t *physics_config = ecs_field(iter, physics_config_t, 3);
	const camera_t *camera = ecs_field(iter, camera_t, 4);

	ecs_observer_init(ecs_world(), &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsPosition, .inout = EcsIn},
			(ecs_term_t){
				.first.id = EcsPredEq,
				.second = (ecs_term_ref_t){
					.id = EcsIsName,
					.name = "Model.scene.Spawn",
				},
				.inout = EcsInOutNone,
			},
		},
		.events = {EcsOnSet},
		.callback = log_spawn_position,
	});

	// Blaster

	const ecs_entity_t blaster = load_model(assets, gpu_device, "blaster");
	if (blaster == 0)
	{
		SDL_LogError(LOG_CATEGORY_CORE, "Failed to load model: %s", SDL_GetError());
		return;
	}

	const ecs_entity_t blaster_instance = create_instance(blaster);

	const rotation_t rotation = {
		.x = 0.F,
		.y = 90.F,
		.z = 0.F,
	};
	ecs_set_id(ecs_world(), blaster_instance, EcsRotation,
		sizeof(rotation_t), &rotation);

	const position_t position = {
		.x = 5.F,
		.y = 1.F,
		.z = -5.F,
	};
	ecs_set_id(ecs_world(), blaster_instance, EcsPosition,
		sizeof(position_t), &position);

	// Bullet

	load_model(assets, gpu_device, "bullet");

	// Scene

	const ecs_entity_t scene = load_model(assets, gpu_device, "scene");
	if (scene == 0)
	{
		SDL_LogError(LOG_CATEGORY_CORE, "Failed to load scene");
		return;
	}

	ecs_add_id(ecs_world(), scene, EcsScene);

	// Physics

	const auto floor_size = (vector3f_t){.x = 100.F, .y = 0.F, .z = 100.F};

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
	const physics_body_id_t player_physics_id = physics_add_capsule(physics_engine, &player_config);
	physics_body_set_position(physics_engine, player_physics_id, camera->position, true);

	const ecs_entity_desc_t entity_desc = {
		.name = "Player",
	};
	const ecs_entity_t player_entity = ecs_entity_init(ecs_world(), &entity_desc);
	ecs_set_id(ecs_world(), player_entity, EcsPhysicsBody,
		sizeof(physics_body_id_t), &player_physics_id);

	const vector3f_t gravity = {.y = -physics_config->gravity_y};
	physics_set_gravity(physics_engine, gravity);

	physics_optimize(physics_engine);
}

static void lock_cursor(ecs_iter_t *iter)
{
	const SDL_MouseButtonEvent *event = ecs_field(iter, SDL_MouseButtonEvent, 0);
	if (event->button != SDL_BUTTON_LEFT)
	{
		return;
	}

	SDL_Window *window = *ecs_field(iter, window_t*, 1);
	SDL_SetWindowRelativeMouseMode(window, true);
}

static void unlock_cursor(ecs_iter_t *iter)
{
	const SDL_KeyboardEvent *event = ecs_field(iter, SDL_KeyboardEvent, 0);
	if (event->key != SDLK_ESCAPE)
	{
		return;
	}

	SDL_Window *window = *ecs_field(iter, window_t*, 1);
	SDL_SetWindowRelativeMouseMode(window, false);
}

static void set_default_metadata()
{
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, ENGINE_NAME);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, ENGINE_VERSION);
	SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING, ENGINE_IDENTIFIER);
}

[[nodiscard]]
static const char *log_category_name(const int category)
{
	switch (category)
	{
		case SDL_LOG_CATEGORY_APPLICATION: return "app";
		case SDL_LOG_CATEGORY_ERROR: return "error";
		case SDL_LOG_CATEGORY_ASSERT: return "assert";
		case SDL_LOG_CATEGORY_SYSTEM: return "system";
		case SDL_LOG_CATEGORY_AUDIO: return "audio";
		case SDL_LOG_CATEGORY_VIDEO: return "video";
		case SDL_LOG_CATEGORY_RENDER: return "render";
		case SDL_LOG_CATEGORY_INPUT: return "input";
		case SDL_LOG_CATEGORY_TEST: return "test";
		case SDL_LOG_CATEGORY_GPU: return "gpu";
		case LOG_CATEGORY_CORE: return "core";
		case LOG_CATEGORY_RENDER: return "render"; // Same name as built-in render logging
		case LOG_CATEGORY_FONT: return "font";
		case LOG_CATEGORY_ASSETS: return "assets";
		case LOG_CATEGORY_INPUT: return "input";
		case LOG_CATEGORY_PHYSICS: return "physics";
		case LOG_CATEGORY_MODEL: return "model";
		case LOG_CATEGORY_ECS: return "ecs";
		case LOG_CATEGORY_SCRIPT: return "script";
		case LOG_CATEGORY_UI: return "ui";
		default: return "unknown";
	}
}

static void log(void *userdata, const int category, const SDL_LogPriority priority, const char *message)
{
	const char *category_name = log_category_name(category);

	static constexpr size_t temp_len = 256;
	static char temp[temp_len];

	SDL_snprintf(temp, temp_len, COLOR_FG_BOLD("%-7s") " " COLOR_FG_WHITE("%s"), category_name, message);
	SDL_GetDefaultLogOutputFunction()(userdata, category, priority, temp);
}

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] const int argc,
	[[maybe_unused]] char **argv)
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

	SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_DEBUG, COLOR_FG_BLUE("debug "));
	SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_INFO, COLOR_FG_GREEN("info  "));
	SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_WARN, COLOR_FG_YELLOW("warn  "));
	SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_ERROR, COLOR_FG_RED("error "));
	SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_CRITICAL, COLOR_FG_RED("fatal "));
	SDL_SetLogOutputFunction(log, nullptr);

	SDL_LogDebug(LOG_CATEGORY_CORE, "Assertion level: %s",
#if SDL_ASSERT_LEVEL == 0
		"disabled"
#elif SDL_ASSERT_LEVEL == 1
		"release"
#elif SDL_ASSERT_LEVEL == 2
		"debug"
#elif SDL_ASSERT_LEVEL == 3
		"paranoid"
#endif
	);

	// For use with RenderDoc
#ifdef FORCE_X11
	if (!SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11"))
	{
		return fatal_error("Failed to set X11 hint");
	}
#endif

#ifndef NDEBUG
	if (!SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1"))
	{
		SDL_LogWarn(LOG_CATEGORY_CORE, "Failed to enable screensaver: %s", SDL_GetError());
	}
#endif

	app_state_t *state = SDL_calloc(1, sizeof(app_state_t));
	if (state == nullptr)
	{
		return fatal_error("Memory allocation failed");
	}
	*appstate = state;

	ecs_create();

	ecs_add_gpu();
	ecs_add_window();
	ecs_add_assets();
	ecs_add_physics();
	ecs_add_render();
	ecs_add_script_engine();

	ecs_observer_init(ecs_world(), &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsAssets, .inout = EcsIn},
			(ecs_term_t){.id = EcsGpuDevice, .inout = EcsIn},
			(ecs_term_t){.id = EcsPhysicsEngine, .inout = EcsIn},
			(ecs_term_t){.id = EcsPhysicsConfig, .inout = EcsIn},
			(ecs_term_t){.id = EcsCamera, .inout = EcsIn},
		},
		.events = {EcsOnSet},
		.callback = build_scene,
	});

	// Should be changed by assets later when loaded
	set_default_metadata();

	constexpr SDL_InitFlags init_flags = SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD;
	if (!SDL_Init(init_flags))
	{
		return fatal_error("Initialisation failed");
	}

	ecs_set_id(ecs_world(), EcsEngine, EcsInit,
		sizeof(SDL_InitFlags), &init_flags);

	state->last_update = SDL_GetTicks();

	const camera_t camera = camera_create_default();
	ecs_set_id(ecs_world(), EcsEngine, EcsCamera,
		sizeof(camera_t), &camera);

	const physics_config_t physics_config = physics_config_create_default();
	ecs_set_id(ecs_world(), EcsEngine, EcsPhysicsConfig,
		sizeof(physics_config_t), &physics_config);

	physics_engine_t physics_engine;
	if (!physics_create(&physics_engine))
	{
		return fatal_error("Failed to initialise physics engine");
	}

	ecs_set_id(ecs_world(), EcsEngine, EcsPhysicsEngine,
		sizeof(physics_engine_t), &physics_engine);

	const SDL_FColor clear_color = {.r = 0.12F, .g = 0.12F, .b = 0.12F, .a = 1.F};
	ecs_set_id(ecs_world(), EcsEngine, EcsClearColor,
		sizeof(clear_color_t), &clear_color);

	state->status_query = ecs_query_init(ecs_world(), &(ecs_query_desc_t){
		.terms = {
			(ecs_term_t){.id = EcsError},
		},
	});

	ecs_observer_init(ecs_world(), &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsMouseButtonEvent, .inout = EcsIn},
			(ecs_term_t){.id = EcsWindow, .src.name = "$window", .inout = EcsIn},
		},
		.events = {EcsOnMouseButton},
		.callback = lock_cursor,
	});

	ecs_observer_init(ecs_world(), &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsKeyboardEvent, .inout = EcsIn},
			(ecs_term_t){.id = EcsWindow, .src.name = "$window", .inout = EcsIn},
		},
		.events = {EcsOnKey},
		.callback = unlock_cursor,
	});

	// NOTE: Returning from here will call a default SDL_Init if not already called
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	app_state_t *state = appstate;

	constexpr auto ns_s = 1'000'000'000.F;
	const Uint64 current_update = SDL_GetTicksNS();
	state->dt = (float) (current_update - state->last_update) / ns_s;
	state->last_update = current_update;

	if (state->fps == 0)
	{
		state->fps = 1.F / state->dt;
	}

	state->count++;
	state->duration += state->dt;
	if (state->duration >= 1.F)
	{
		state->fps = state->count;
		state->count = 0;
		state->duration = 0;
	}

	ecs_progress(ecs_world(), state->dt);

	physics_engine_t *physics_engine = ecs_get_mut_id(ecs_world(), EcsEngine, EcsPhysicsEngine);
	const physics_config_t *physics_config = ecs_get_id(ecs_world(), EcsEngine, EcsPhysicsConfig);

	const ecs_entity_t player_entity = ecs_lookup(ecs_world(), "Player");
	const physics_body_id_t player_body_id = player_entity != 0
		? *((physics_body_id_t*) ecs_get_id(ecs_world(), player_entity, EcsPhysicsBody))
		: 0;

	SDL_Window *window = ecs_get_id_ptr(EcsWindow);
	camera_t *camera = ecs_get_mut_id(ecs_world(), EcsEngine, EcsCamera);

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
			physics_body_add_linear_velocity(physics_engine, player_body_id, velocity);
		}

		if (input_is_down("move_backward"))
		{
			const vector3f_t velocity = camera_to_z(camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(physics_engine, player_body_id, velocity);
		}

		if (input_is_down("move_left"))
		{
			const vector3f_t velocity = camera_to_x(camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(physics_engine, player_body_id, velocity);
		}

		if (input_is_down("move_right"))
		{
			const vector3f_t velocity = camera_to_x(camera, move_speed * state->dt);
			physics_body_add_linear_velocity(physics_engine, player_body_id, velocity);
		}

		if (input_is_down("move_up"))
		{
			const vector3f_t velocity = camera_to_y(camera, move_speed * state->dt);
			physics_body_add_linear_velocity(physics_engine, player_body_id, velocity);
		}

		if (input_is_down("move_down"))
		{
			const vector3f_t velocity = camera_to_y(camera, -(move_speed * state->dt));
			physics_body_add_linear_velocity(physics_engine, player_body_id, velocity);
		}

		if (input_is_pressed("jump"))
		{
			const vector3f_t velocity = physics_body_linear_velocity(physics_engine, player_body_id);
			if (velocity.y > -0.1F && velocity.y < 0.1F)
			{
				const vector3f_t jump_velocity = {
					.x = velocity.x,
					.y = jump_speed,
					.z = velocity.z,
				};
				physics_body_set_linear_velocity(physics_engine, player_body_id, jump_velocity);
			}
		}

		if (input_is_pressed("shoot"))
		{
			static constexpr float firepower = 100.F;

			const ecs_entity_t bullet = ecs_lookup(ecs_world(), "Model.bullet");
			const ecs_entity_t entity = create_instance(bullet);

			position_t position = {};
			query("[none] (chirp.InstanceOf, Model.blaster), [in] chirp.Position")
			{
				position = *ecs_field(&iter, position_t, 1);
			}

			ecs_set_id(ecs_world(), entity, EcsPosition,
				sizeof(position_t), &position);

			const rotation_t rotation = vector3f_zero();
			ecs_set_id(ecs_world(), entity, EcsRotation,
				sizeof(rotation_t), &rotation);

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
			ecs_set_id(ecs_world(), entity, EcsPhysicsBody,
				sizeof(physics_body_id_t), &body_id);

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
	const vector3f_t velocity = physics_body_linear_velocity(physics_engine, player_body_id);
	const vector3f_t clamped_velocity = vector3f_clamp(velocity, min_velocity, max_velocity);
	physics_body_set_linear_velocity(physics_engine, player_body_id, clamped_velocity);

	// TODO: There should be a better way to do this, right?
	const vector3f_t player_position = physics_body_position(physics_engine, player_body_id);
	camera->target = vector3f_add(camera->target, vector3f_sub(player_position, camera->position));
	camera->position = player_position;

	const vector3f_t forward_n = vector3f_normalize(vector3f_sub(camera->target, camera->position));
	const vector3f_t right_n = vector3f_normalize(vector3f_cross(forward_n, camera->up));
	const vector3f_t up_n = vector3f_normalize(camera->up);

	vector3f_t weapon_position = camera->position;
	weapon_position = vector3f_add(weapon_position, vector3f_scale(forward_n, 0.2F));
	weapon_position = vector3f_add(weapon_position, vector3f_scale(right_n, 0.25F));
	weapon_position = vector3f_add(weapon_position, vector3f_scale(up_n, -0.2F));

	query(
		"[none] chirp.InstanceOf($i, $m),"
		"[none] $m == \"Model.blaster\","
		"[out]  chirp.Position($i),"
		"[out]  chirp.Rotation($i),"
		"[none] ChildOf($n, $i),"
		"[out]  chirp.Projection($n)")
	{
		vector3f_t *positions = ecs_field(&iter, position_t, 2);
		vector3f_t *rotations = ecs_field(&iter, rotation_t, 3);
		projection_t *projections = ecs_field(&iter, projection_t, 5);

		positions[0] = weapon_position;
		rotations[0] = (rotation_t){
			.x = SDL_asinf(forward_n.y),
			.y = SDL_atan2f(-forward_n.z, forward_n.x) - (SDL_PI_F * 0.5F),
			.z = 0.0F,
		};
		projections[0].rebuild = true;
	}

	ecs_iter_t iter = ecs_query_iter(ecs_world(), state->status_query);
	if (ecs_query_next(&iter))
	{
		const error_t *error = ecs_field(&iter, error_t, 0);
		SDL_LogCritical(LOG_CATEGORY_CORE, "%s: %s", error->title, error->message);
#ifdef NDEBUG
		SDL_Window *window = ecs_get_id_ptr(EcsWindow);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, error->title, error->message, window);
#endif
		ecs_iter_fini(&iter);
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

static void emit(const ecs_entity_t event, const ecs_id_t value_type, void *value)
{
	const ecs_entity_t entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
		.set = ecs_values((ecs_value_t){.type = value_type, .ptr = value}),
	});

	ecs_emit(ecs_world(), &(ecs_event_desc_t){
		.event = event,
		.entity = entity,
		.ids = &(ecs_type_t){
			.array = (ecs_id_t[]){value_type},
			.count = 1,
		},
	});

	ecs_delete(ecs_world(), entity);
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event)
{
	const SDL_EventType event_type = event->type;

	if (event_type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	if (event_type == SDL_EVENT_MOUSE_BUTTON_DOWN
		|| event_type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		emit(EcsOnMouseButton, EcsMouseButtonEvent, &event->button);
	}

	if (event_type == SDL_EVENT_KEY_DOWN
		|| event_type == SDL_EVENT_KEY_UP)
	{
		emit(EcsOnKey, EcsKeyboardEvent, &event->key);
	}

	SDL_Window *window = *((window_t**) ecs_get_id(ecs_world(), EcsEngine, EcsWindow));
	if (SDL_GetWindowRelativeMouseMode(window))
	{
		input_update(event);
	}

	if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		emit(EcsOnWindowResized, EcsWindowEvent, &event->window);
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

	assets_destroy(ecs_get_id(ecs_world(), EcsEngine, EcsAssets));
	physics_destroy(ecs_get_id(ecs_world(), EcsEngine, EcsPhysicsEngine));
	script_engine_destroy();

	// cImGui_ImplSDL3_Shutdown();
	// cImGui_ImplSDLGPU3_Shutdown();
	// ImGui_DestroyContext(nullptr);

	SDL_Window *window = ecs_get_id_ptr(EcsWindow);
	SDL_GPUDevice *gpu_device = ecs_get_id_ptr(EcsGpuDevice);
	SDL_GPUGraphicsPipeline *pipeline = ecs_get_id_ptr(EcsGpuGraphicsPipeline);
	SDL_GPUTexture *depth_texture = ecs_get_id_ptr(EcsDepthTexture);

	SDL_ReleaseGPUTexture(gpu_device, depth_texture);
	SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
	SDL_ReleaseWindowFromGPUDevice(gpu_device, window);

	SDL_DestroyWindow(window);
	SDL_DestroyGPUDevice(gpu_device);

	ecs_destroy();
	SDL_free(appstate);
}
