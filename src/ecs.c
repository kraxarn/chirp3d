#include "ecs.h"
#include "args.h"
#include "assets.h"
#include "camera.h"
#include "ecsosapi.h"
#include "input.h"
#include "logcategory.h"
#include "mousebutton.h"
#include "physics.h"
#include "physicsconfig.h"
#include "windowconfig.h"
#include "ecs/components.h"
#include "ecs/entities.h"
#include "ecs/events.h"
#include "ecs/tags.h"

#include "flecs.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

static ecs_world_t *world = nullptr;
static ecs_entity_t phases[PHASE_COUNT];

static void log_debug_info()
{
	constexpr size_t temp_len = 160;
	char temp[temp_len] = {0};

#define append(str) if(temp[0] != '\0') SDL_strlcat(temp, ", ", temp_len); SDL_strlcat(temp, str, temp_len)

#ifdef FLECS_CPP
	append("cpp");
#endif

#ifdef FLECS_MODULE
	append("module");
#endif

#ifdef FLECS_SYSTEM
	append("system");
#endif

#ifdef FLECS_PIPELINE
	append("pipeline");
#endif

#ifdef FLECS_TIMER
	append("timer");
#endif

#ifdef FLECS_META
	append("meta");
#endif

#ifdef FLECS_UNITS
	append("units");
#endif

#ifdef FLECS_JSON
	append("json");
#endif

#ifdef FLECS_DOC
	append("doc");
#endif

#ifdef FLECS_HTTP
	append("http");
#endif

#ifdef FLECS_REST
	append("rest");
#endif

#ifdef FLECS_PARSER
	append("parser");
#endif

#ifdef FLECS_QUERY_DSL
	append("query_dsl");
#endif

#ifdef FLECS_SCRIPT
	append("script");
#endif

#ifdef FLECS_STATS
	append("stats");
#endif

#ifdef FLECS_METRICS
	append("metrics");
#endif

#ifdef FLECS_ALERTS
	append("alerts");
#endif

#ifdef FLECS_LOG
	append("log");
#endif

#ifdef FLECS_JOURNAL
	append("journal");
#endif

#ifdef FLECS_APP
	append("app");
#endif

#ifdef FLECS_OS_API_IMPL
	append("os_api_impl");
#endif

#undef append

	SDL_LogDebug(LOG_CATEGORY_ECS, "Addons: %s", temp);
}

static ecs_entity_t phase(const char *name)
{
	const ecs_entity_desc_t entity_desc = {
		.name = name,
		.add = ecs_ids(EcsPhase),
	};
	return ecs_entity_init(world, &entity_desc);
}

static void phase_depend(const phase_t source, const phase_t target)
{
	ecs_add_pair(world, phases[source], EcsDependsOn, phases[target]);
}

static void create_pipeline()
{
	const ecs_pipeline_desc_t pipeline_desc = {
		.query.terms = {
			(ecs_term_t){
				.id = EcsSystem,
			},
			(ecs_term_t){
				.id = EcsPhase,
				.src.id = EcsCascade,
				.trav = EcsDependsOn,
			},
		},
	};
	const ecs_entity_t pipeline = ecs_pipeline_init(world, &pipeline_desc);
	ecs_set_pipeline(world, pipeline);

	phases[PHASE_UPDATE] = phase("Update");
	phases[PHASE_PHYSICS_UPDATE] = phase("PhysicsUpdate");
	phases[PHASE_PHYSICS_SYNC] = phase("PhysicsSync");
	phases[PHASE_RENDER_BEGIN] = phase("RenderBegin");
	phases[PHASE_RENDER] = phase("Render");
	phases[PHASE_RENDER_END] = phase("RenderEnd");

	phase_depend(PHASE_PHYSICS_UPDATE, PHASE_UPDATE);
	phase_depend(PHASE_PHYSICS_SYNC, PHASE_PHYSICS_UPDATE);
	phase_depend(PHASE_RENDER_BEGIN, PHASE_PHYSICS_SYNC);
	phase_depend(PHASE_RENDER, PHASE_RENDER_BEGIN);
	phase_depend(PHASE_RENDER_END, PHASE_RENDER);
}

static void ctor_zero(void *ptr, const Sint32 count, const ecs_type_info_t *type_info)
{
	SDL_memset(ptr, 0, (size_t) count * type_info->size);
}

[[nodiscard]]
static ecs_entity_t entity(const char *name)
{
	return ecs_entity_init(world, &(ecs_entity_desc_t){
		.name = name,
	});
}

[[nodiscard]]
static ecs_id_t component_impl(const char *name, const char *symbol,
	const ecs_size_t size, const ecs_size_t alignment)
{
	const ecs_entity_desc_t entity_desc = {
		.use_low_id = true,
		.name = name,
		.symbol = symbol,
	};

	const ecs_component_desc_t component_desc = {
		.entity = ecs_entity_init(world, &entity_desc),
		.type = (ecs_type_info_t){
			.size = size,
			.alignment = alignment,
		},
	};

	const ecs_id_t component = ecs_component_init(world, &component_desc);
	SDL_assert(component != 0);

	const ecs_type_hooks_t hooks = {
		.ctor = ctor_zero,
	};
	ecs_set_hooks_id(world, component, &hooks);

	return component;
}

#define component(name, symbol)	\
	component_impl(name, #symbol, ECS_SIZEOF(symbol), ECS_ALIGNOF(symbol))

static ecs_entity_t tag(const char *name)
{
	const ecs_entity_desc_t entity_desc = {
		.name = name,
	};

	const ecs_entity_t entity = ecs_entity_init(world, &entity_desc);
	SDL_assert(entity != 0);
	return entity;
}

#ifndef NDEBUG

#define reflect(e, ...)							\
	do {										\
		const ecs_struct_desc_t struct_desc = {	\
			.entity = e,						\
			.members = {__VA_ARGS__},			\
		};										\
		ecs_struct_init(world, &struct_desc);	\
	} while (false)

#define reflect_enum(e, t, ...)					\
	ecs_enum_init(world, &(ecs_enum_desc_t){	\
		.entity = e,							\
		.constants = {__VA_ARGS__},				\
		.underlying_type = t,					\
	});

static ecs_entity_t reflect_string(const ecs_entity_t entity,
	const ecs_meta_serialize_t serialize)
{
	return ecs_opaque_init(world, &(ecs_opaque_desc_t){
		.entity = entity,
		.type = (EcsOpaque){
			.as_type = ecs_id(ecs_string_t),
			.serialize = serialize,
		},
	});
}

#endif

static void add_events()
{
	EcsOnMouseButton = entity("OnMouseButton");
	EcsMouseButtonEvent = component("MouseButtonEvent", SDL_MouseButtonEvent);

	EcsOnKey = entity("OnKey");
	EcsKeyboardEvent = component("KeyboardEvent", SDL_KeyboardEvent);

	EcsOnWindowResized = entity("OnWindowResized");
	EcsWindowEvent = component("WindowEvent", SDL_WindowEvent);
}

#ifndef NDEBUG

static int keycode_serialize(const ecs_serializer_t *ser, const void *ptr)
{
	const SDL_Keycode keycode = *(SDL_Keycode*) ptr;
	const char *name = SDL_GetKeyName(keycode);
	return ser->value(ser, ecs_id(ecs_string_t), (const void*) &name);
}

static int mouse_button_flags_serialize(const ecs_serializer_t *ser, const void *ptr)
{
	const SDL_MouseButtonFlags flags = *(SDL_MouseButtonFlags*) ptr;
	const char *name = mouse_button_name(flags);
	return ser->value(ser, ecs_id(ecs_string_t), (const void*) &name);
}

#endif

static void add_input()
{
	EcsInput = entity("Input");
	EcsKeycodeStates = entity("KeycodeStates");
	EcsMouseButtonStates = entity("MouseButtonStates");
	EcsMapsTo = entity("MapsTo");

	EcsInputState = component("InputState", input_state_t);
	EcsKeycode = component("Keycode", SDL_Keycode);
	EcsMouseButtonFlags = component("MouseButtonFlags", SDL_MouseButtonFlags);

#ifndef NDEBUG
	reflect_enum(EcsInputState, ecs_id(ecs_u8_t),
		(ecs_enum_constant_t){.name = "Up", .value_unsigned = STATE_UP},
		(ecs_enum_constant_t){.name = "Pressed", .value_unsigned = STATE_PRESSED},
		(ecs_enum_constant_t){.name = "Down", .value_unsigned = STATE_DOWN},
	);

	reflect_string(EcsKeycode, keycode_serialize);
	reflect_string(EcsMouseButtonFlags, mouse_button_flags_serialize);
#endif
}

static void module([[maybe_unused]] ecs_world_t *unused)
{
	ecs_scope("Chirp")
	{
		EcsInstanceOf = entity("InstanceOf");

		EcsEngine = tag("Engine");
		EcsScene = tag("Scene");

		EcsAssets = component("Assets", assets_t);
		EcsMetadata = component("Metadata", metadata_t);
		EcsInit = component("Init", init_flags_t);
		EcsWindowConfig = component("WindowConfig", window_config_t);
		EcsWindow = component("Window", window_t*);
		EcsGpuDevice = component("GpuDevice", gpu_device_t*);
		EcsGpuGraphicsPipeline = component("GpuGraphicsPipeline", gpu_graphics_pipeline_t*);
		EcsDepthTexture = component("DepthTexture", depth_texture_t*);
		EcsGpuCommandBuffer = component("GpuCommandBuffer", gpu_command_buffer_t*);
		EcsGpuRenderPass = component("GpuRenderPass", gpu_render_pass_t*);
		EcsSwapchainTexture = component("SwapchainTexture", swapchain_texture_t*);
		EcsSwapchainTextureSize = component("SwapchainTextureSize", swapchain_texture_size_t);
		EcsCamera = component("Camera", camera_t);
		EcsPhysicsConfig = component("PhysicsConfig", physics_config_t);
		EcsPhysicsEngine = component("PhysicsEngine", physics_engine_t);
		EcsModel = component("Model", model_t);
		EcsPhysicsBody = component("PhysicsBody", physics_body_id_t);
		EcsRotation = component("Rotation", rotation_t);
		EcsPosition = component("Position", position_t);
		EcsScale = component("Scale", scale_t);
		EcsProjection = component("Projection", projection_t);
		EcsImGuiContext = component("ImGuiContext", imgui_context_t*);
		EcsImGuiDrawData = component("ImGuiDrawData", imgui_draw_data_t);
		EcsVertexShader = component("VertexShader", vertex_shader_t*);
		EcsFragmentShader = component("FragmentShader", fragment_shader_t*);
		EcsClearColor = component("ClearColor", clear_color_t);
		EcsViewProjection = component("ViewProjection", view_projection_t);
		EcsWorldTransform = component("WorldTransform", world_transform_t);
		EcsError = component("Error", error_t);
		EcsScriptEngine = component("ScriptEngine", py_vm_index_t);
		EcsArgs = component("Args", args_t);
		EcsModelInstance = component("ModelInstance", model_instance_t);

#ifndef NDEBUG
		reflect(EcsMetadata,
			(ecs_member_t){.name = "name", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "version", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "identifier", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "creator", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "copyright", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "url", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "type", .type = ecs_id(ecs_string_t)},
		);

		reflect(EcsInit,
			(ecs_member_t){.name = "flags", .type = ecs_id(ecs_u32_t)},
		);

		reflect(EcsWindowConfig,
			(ecs_member_t){.name = "title", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "size", .type = ecs_id(ecs_i32_t), .count = 2},
			(ecs_member_t){.name = "fullscreen", .type = ecs_id(ecs_bool_t)},
		);

		reflect(EcsCamera,
			(ecs_member_t){.name = "position", .type = ecs_id(ecs_f32_t), .count = 3},
			(ecs_member_t){.name = "target", .type = ecs_id(ecs_f32_t), .count = 3},
			(ecs_member_t){.name = "up", .type = ecs_id(ecs_f32_t), .count = 3},
			(ecs_member_t){.name = "fov_y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "near_plane", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "far_plane", .type = ecs_id(ecs_f32_t)},
		);

		reflect(EcsPhysicsConfig,
			(ecs_member_t){.name = "move_speed", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "max_move_speed", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "gravity_y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "jump_speed", .type = ecs_id(ecs_f32_t)},
		);

		reflect(EcsPhysicsBody,
			(ecs_member_t){.name = "id", .type = ecs_id(ecs_u32_t)},
		);

		reflect(EcsRotation,
			(ecs_member_t){.name = "x", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "z", .type = ecs_id(ecs_f32_t)},
		);

		reflect(EcsPosition,
			(ecs_member_t){.name = "x", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "z", .type = ecs_id(ecs_f32_t)},
		);

		reflect(EcsScale,
			(ecs_member_t){.name = "x", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "z", .type = ecs_id(ecs_f32_t)},
		);

		reflect(EcsProjection,
			(ecs_member_t){.name = "rebuild", .type = ecs_id(ecs_bool_t)},
			(ecs_member_t){.name = "value", .type = ecs_id(ecs_f32_t), .count = 16},
		);

		reflect(EcsWorldTransform,
			(ecs_member_t){.name = "value", .type = ecs_id(ecs_f32_t), .count = 16},
		);

		reflect(EcsError,
			(ecs_member_t){.name = "title", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "message", .type = ecs_id(ecs_string_t)},
		);

		reflect(EcsScriptEngine,
			(ecs_member_t){.name = "vm_index", .type = ecs_id(ecs_i32_t)},
		);

		reflect(EcsArgs,
			(ecs_member_t){.name = "prefer_low_power", .type = ecs_id(ecs_bool_t)},
		);

		reflect(EcsModelInstance,
			(ecs_member_t){.name = "name", .type = ecs_id(ecs_string_t)},
		);

#endif

		create_pipeline();
	}

	ecs_scope("ChirpEvent")
	{
		add_events();
	}

	ecs_scope("ChirpInput")
	{
		add_input();
	};
}

static void on_init_set([[maybe_unused]] ecs_iter_t *iter)
{
	ecs_os_api_t os_api = ecs_os_api_create();
	ecs_os_set_api(&os_api);

	const int cores = SDL_GetNumLogicalCPUCores();
	ecs_set_threads(world, cores);
	SDL_LogDebug(LOG_CATEGORY_ECS, "Using %d threads", cores);

#ifdef FLECS_REST
	ecs_singleton_set(world, EcsRest, {0});
#endif

#ifdef FLECS_STATS
	ECS_IMPORT(world, FlecsStats);
#endif
}

void ecs_create()
{
	if (world != nullptr)
	{
		return;
	}

	log_debug_info();

	world = ecs_init();
	ecs_import(world, module, "chirp");

	// SDL has to initialise before we set up OS-specific stuff
	ecs_observer_init(world, &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsInit}
		},
		.events = {EcsOnSet},
		.callback = on_init_set,
	});
}

void ecs_destroy()
{
	ecs_fini(world);
	world = nullptr;
}

ecs_world_t *ecs_world()
{
	return world;
}

ecs_entity_t ecs_phase(const phase_t phase)
{
	const ecs_entity_t entity = phases[phase];
	SDL_assert(entity != 0);
	return entity;
}

ecs_entity_t ecs_set_error(const char *title, const char *message)
{
	const ecs_entity_t entity = ecs_new(world);

	const error_t error = {
		.title = SDL_strdup(title),
		.message = SDL_strdup(message),
	};

	ecs_set_id(world, entity, EcsError,
		sizeof(error_t), &error);

	return entity;
}

void *ecs_get_id_ptr(const ecs_id_t component)
{
	const void *data = ecs_get_id(world, EcsEngine, component);
	return data == nullptr ? nullptr : *(void**) data;
}
