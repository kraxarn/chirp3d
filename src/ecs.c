#include "ecs.h"
#include "assets.h"
#include "camera.h"
#include "ecsosapi.h"
#include "input.h"
#include "logcategory.h"
#include "physics.h"
#include "physicsconfig.h"
#include "windowconfig.h"
#include "ecs/events.h"

#include "flecs.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

static ecs_world_t *world = nullptr;
static ecs_entity_t phases[PHASE_COUNT];

ecs_entity_t EcsOnMouseButton = 0;
ecs_id_t EcsMouseButtonEvent = 0;
ecs_entity_t EcsOnKeyboard = 0;
ecs_id_t EcsKeyboardEvent = 0;

static void log_debug_info()
{
	constexpr size_t temp_len = 160;
	char temp[temp_len] = {0};

#define append(str) if(temp[0] != '\0') SDL_strlcat(temp, ", ", temp_len); SDL_strlcat(temp ,str, temp_len)

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

	SDL_LogDebug(LOG_CATEGORY_ECS, "ECS addons: %s", temp);
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

#define reflect(n, ...)							\
	do {										\
		const ecs_struct_desc_t struct_desc = {	\
			.entity = ecs_lookup(world, n),		\
			.members = {__VA_ARGS__},			\
		};										\
		ecs_struct_init(world, &struct_desc);	\
	} while (false)

#define scope(name)	\
	for (const ecs_entity_t mod = ecs_module_init(world, name, &(ecs_component_desc_t){}),	\
		scope = ecs_set_scope(world, mod);													\
		ecs_get_scope(world) == mod;														\
		ecs_set_scope(world, scope))

static void add_events()
{
	const ecs_id_t event_type = component("EventType", SDL_EventType);

	EcsOnMouseButton = entity("OnMouseButton");
	EcsMouseButtonEvent = component("MouseButtonEvent", SDL_MouseButtonEvent);

	EcsOnKeyboard = entity("OnKeyboard");
	EcsKeyboardEvent = component("KeyboardEvent", SDL_KeyboardEvent);

#ifndef NDEBUG
	ecs_enum_init(world, &(ecs_enum_desc_t){
		.entity = event_type,
		// By default, only 32 values are allowed, so ignore unused values
		.constants = {
			// Application
			(ecs_enum_constant_t){.name = "Quit", .value = SDL_EVENT_QUIT},
			// iOS / Android
			// (ecs_enum_constant_t){.name = "Terminating", .value = SDL_EVENT_TERMINATING},
			// (ecs_enum_constant_t){.name = "LowMemory", .value = SDL_EVENT_LOW_MEMORY},
			// (ecs_enum_constant_t){.name = "WillEnterBackground", .value = SDL_EVENT_WILL_ENTER_BACKGROUND},
			// (ecs_enum_constant_t){.name = "DidEnterBackground", .value = SDL_EVENT_DID_ENTER_BACKGROUND},
			// (ecs_enum_constant_t){.name = "WillEnterForeground", .value = SDL_EVENT_WILL_ENTER_FOREGROUND},
			// (ecs_enum_constant_t){.name = "DidEnterForeground", .value = SDL_EVENT_DID_ENTER_FOREGROUND},
			// (ecs_enum_constant_t){.name = "LocaleChanged", .value = SDL_EVENT_LOCALE_CHANGED},
			// (ecs_enum_constant_t){.name = "SystemThemeChanged", .value = SDL_EVENT_SYSTEM_THEME_CHANGED},
			// Display
			// (ecs_enum_constant_t){.name = "DisplayOrientation", .value = SDL_EVENT_DISPLAY_ORIENTATION},
			// (ecs_enum_constant_t){.name = "DisplayAdded", .value = SDL_EVENT_DISPLAY_ADDED},
			// (ecs_enum_constant_t){.name = "DisplayRemoved", .value = SDL_EVENT_DISPLAY_REMOVED},
			// (ecs_enum_constant_t){.name = "DisplayMoved", .value = SDL_EVENT_DISPLAY_MOVED},
			// (ecs_enum_constant_t){.name = "DisplayDesktopModeChanged", .value = SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED},
			// (ecs_enum_constant_t){.name = "DisplayCurrentModeChanged", .value = SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED},
			// (ecs_enum_constant_t){.name = "DisplayContentScaleChanged", .value = SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED},
			// (ecs_enum_constant_t){.name = "DisplayUsableBoundsChanged", .value = SDL_EVENT_DISPLAY_USABLE_BOUNDS_CHANGED},
			// Window
			// (ecs_enum_constant_t){.name = "WindowShown", .value = SDL_EVENT_WINDOW_SHOWN},
			// (ecs_enum_constant_t){.name = "WindowHidden", .value = SDL_EVENT_WINDOW_HIDDEN},
			// (ecs_enum_constant_t){.name = "WindowExposed", .value = SDL_EVENT_WINDOW_EXPOSED},
			// (ecs_enum_constant_t){.name = "WindowMoved", .value = SDL_EVENT_WINDOW_MOVED},
			// (ecs_enum_constant_t){.name = "WindowResized", .value = SDL_EVENT_WINDOW_RESIZED},
			// (ecs_enum_constant_t){.name = "WindowPixelSizeChanged", .value = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED},
			// (ecs_enum_constant_t){.name = "WindowMetalViewResized", .value = SDL_EVENT_WINDOW_METAL_VIEW_RESIZED},
			// (ecs_enum_constant_t){.name = "WindowMinimized", .value = SDL_EVENT_WINDOW_MINIMIZED},
			// (ecs_enum_constant_t){.name = "WindowMaximized", .value = SDL_EVENT_WINDOW_MAXIMIZED},
			// (ecs_enum_constant_t){.name = "WindowRestored", .value = SDL_EVENT_WINDOW_RESTORED},
			// (ecs_enum_constant_t){.name = "WindowMouseEnter", .value = SDL_EVENT_WINDOW_MOUSE_ENTER},
			// (ecs_enum_constant_t){.name = "WindowMouseLeave", .value = SDL_EVENT_WINDOW_MOUSE_LEAVE},
			// (ecs_enum_constant_t){.name = "WindowFocusGained", .value = SDL_EVENT_WINDOW_FOCUS_GAINED},
			// (ecs_enum_constant_t){.name = "WindowFocusLost", .value = SDL_EVENT_WINDOW_FOCUS_LOST},
			// (ecs_enum_constant_t){.name = "WindowCloseRequested", .value = SDL_EVENT_WINDOW_CLOSE_REQUESTED},
			// (ecs_enum_constant_t){.name = "WindowHitTest", .value = SDL_EVENT_WINDOW_HIT_TEST},
			// (ecs_enum_constant_t){.name = "WindowIccProfChanged", .value = SDL_EVENT_WINDOW_ICCPROF_CHANGED},
			// (ecs_enum_constant_t){.name = "WindowDisplayChanged", .value = SDL_EVENT_WINDOW_DISPLAY_CHANGED},
			// (ecs_enum_constant_t){.name = "WindowDisplayScaleChanged", .value = SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED},
			// (ecs_enum_constant_t){.name = "WindowSafeAreaChanged", .value = SDL_EVENT_WINDOW_SAFE_AREA_CHANGED},
			// (ecs_enum_constant_t){.name = "WindowOccluded", .value = SDL_EVENT_WINDOW_OCCLUDED},
			// (ecs_enum_constant_t){.name = "WindowEnterFullscreen", .value = SDL_EVENT_WINDOW_ENTER_FULLSCREEN},
			// (ecs_enum_constant_t){.name = "WindowLeaveFullscreen", .value = SDL_EVENT_WINDOW_LEAVE_FULLSCREEN},
			// (ecs_enum_constant_t){.name = "WindowDestroyed", .value = SDL_EVENT_WINDOW_DESTROYED},
			// (ecs_enum_constant_t){.name = "WindowHdrStateChanged", .value = SDL_EVENT_WINDOW_HDR_STATE_CHANGED},
			// Keyboard
			// (ecs_enum_constant_t){.name = "KeyDown", .value = SDL_EVENT_KEY_DOWN},
			// (ecs_enum_constant_t){.name = "KeyUp", .value = SDL_EVENT_KEY_UP},
			// (ecs_enum_constant_t){.name = "TextEditing", .value = SDL_EVENT_TEXT_EDITING},
			// (ecs_enum_constant_t){.name = "TextInput", .value = SDL_EVENT_TEXT_INPUT},
			// (ecs_enum_constant_t){.name = "KeymapChanged", .value = SDL_EVENT_KEYMAP_CHANGED},
			// (ecs_enum_constant_t){.name = "KeyboardAdded", .value = SDL_EVENT_KEYBOARD_ADDED},
			// (ecs_enum_constant_t){.name = "KeyboardRemoved", .value = SDL_EVENT_KEYBOARD_REMOVED},
			// (ecs_enum_constant_t){.name = "TextEditingCandidates", .value = SDL_EVENT_TEXT_EDITING_CANDIDATES},
			// (ecs_enum_constant_t){.name = "ScreenKeyboardShown", .value = SDL_EVENT_SCREEN_KEYBOARD_SHOWN},
			// (ecs_enum_constant_t){.name = "ScreenKeyboardHidden", .value = SDL_EVENT_SCREEN_KEYBOARD_HIDDEN},
			// Mouse
			// (ecs_enum_constant_t){.name = "MouseMotion", .value = SDL_EVENT_MOUSE_MOTION},
			(ecs_enum_constant_t){.name = "MouseButtonDown", .value = SDL_EVENT_MOUSE_BUTTON_DOWN},
			(ecs_enum_constant_t){.name = "MouseButtonUp", .value = SDL_EVENT_MOUSE_BUTTON_UP},
			// (ecs_enum_constant_t){.name = "MouseWheel", .value = SDL_EVENT_MOUSE_WHEEL},
			// (ecs_enum_constant_t){.name = "MouseAdded", .value = SDL_EVENT_MOUSE_ADDED},
			// (ecs_enum_constant_t){.name = "MouseRemoved", .value = SDL_EVENT_MOUSE_REMOVED},
			// Joystick
			// (ecs_enum_constant_t){.name = "JoystickAxisMotion", .value = SDL_EVENT_JOYSTICK_AXIS_MOTION},
			// (ecs_enum_constant_t){.name = "JoystickBallMotion", .value = SDL_EVENT_JOYSTICK_BALL_MOTION},
			// (ecs_enum_constant_t){.name = "JoystickHatMotion", .value = SDL_EVENT_JOYSTICK_HAT_MOTION},
			// (ecs_enum_constant_t){.name = "JoystickButtonDown", .value = SDL_EVENT_JOYSTICK_BUTTON_DOWN},
			// (ecs_enum_constant_t){.name = "JoystickButtonUp", .value = SDL_EVENT_JOYSTICK_BUTTON_UP},
			// (ecs_enum_constant_t){.name = "JoystickAdded", .value = SDL_EVENT_JOYSTICK_ADDED},
			// (ecs_enum_constant_t){.name = "JoystickRemoved", .value = SDL_EVENT_JOYSTICK_REMOVED},
			// (ecs_enum_constant_t){.name = "JoystickBatteryUpdated", .value = SDL_EVENT_JOYSTICK_BATTERY_UPDATED},
			// (ecs_enum_constant_t){.name = "JoystickUpdateComplete", .value = SDL_EVENT_JOYSTICK_UPDATE_COMPLETE},
			// Gamepad
			// (ecs_enum_constant_t){.name = "GamepadAxisMotion", .value = SDL_EVENT_GAMEPAD_AXIS_MOTION},
			// (ecs_enum_constant_t){.name = "GamepadButtonDown", .value = SDL_EVENT_GAMEPAD_BUTTON_DOWN},
			// (ecs_enum_constant_t){.name = "GamepadButtonUp", .value = SDL_EVENT_GAMEPAD_BUTTON_UP},
			// (ecs_enum_constant_t){.name = "GamepadAdded", .value = SDL_EVENT_GAMEPAD_ADDED},
			// (ecs_enum_constant_t){.name = "GamepadRemoved", .value = SDL_EVENT_GAMEPAD_REMOVED},
			// (ecs_enum_constant_t){.name = "GamepadRemapped", .value = SDL_EVENT_GAMEPAD_REMAPPED},
			// (ecs_enum_constant_t){.name = "GamepadTouchpadDown", .value = SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN},
			// (ecs_enum_constant_t){.name = "GamepadTouchpadMotion", .value = SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION},
			// (ecs_enum_constant_t){.name = "GamepadTouchpadUp", .value = SDL_EVENT_GAMEPAD_TOUCHPAD_UP},
			// (ecs_enum_constant_t){.name = "GamepadSensorUpdate", .value = SDL_EVENT_GAMEPAD_SENSOR_UPDATE},
			// (ecs_enum_constant_t){.name = "GamepadUpdateComplete", .value = SDL_EVENT_GAMEPAD_UPDATE_COMPLETE},
			// (ecs_enum_constant_t){.name = "GamepadSteamHandleUpdated", .value = SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED},
			// Touch
			// (ecs_enum_constant_t){.name = "FingerDown", .value = SDL_EVENT_FINGER_DOWN},
			// (ecs_enum_constant_t){.name = "FingerUp", .value = SDL_EVENT_FINGER_UP},
			// (ecs_enum_constant_t){.name = "FingerMotion", .value = SDL_EVENT_FINGER_MOTION},
			// (ecs_enum_constant_t){.name = "FingerCanceled", .value = SDL_EVENT_FINGER_CANCELED},
			// Pinch
			// (ecs_enum_constant_t){.name = "PinchBegin", .value = SDL_EVENT_PINCH_BEGIN},
			// (ecs_enum_constant_t){.name = "PinchUpdate", .value = SDL_EVENT_PINCH_UPDATE},
			// (ecs_enum_constant_t){.name = "PinchEnd", .value = SDL_EVENT_PINCH_END},
			// Clipboard
			// (ecs_enum_constant_t){.name = "ClipboardUpdate", .value = SDL_EVENT_CLIPBOARD_UPDATE},
			// Drag and drop
			// (ecs_enum_constant_t){.name = "DropFile", .value = SDL_EVENT_DROP_FILE},
			// (ecs_enum_constant_t){.name = "DropText", .value = SDL_EVENT_DROP_TEXT},
			// (ecs_enum_constant_t){.name = "DropBegin", .value = SDL_EVENT_DROP_BEGIN},
			// (ecs_enum_constant_t){.name = "DropComplete", .value = SDL_EVENT_DROP_COMPLETE},
			// (ecs_enum_constant_t){.name = "DropPosition", .value = SDL_EVENT_DROP_POSITION},
			// Audio hotplug
			// (ecs_enum_constant_t){.name = "AudioDeviceAdded", .value = SDL_EVENT_AUDIO_DEVICE_ADDED},
			// (ecs_enum_constant_t){.name = "AudioDeviceRemoved", .value = SDL_EVENT_AUDIO_DEVICE_REMOVED},
			// (ecs_enum_constant_t){.name = "AudioDeviceFormatChanged", .value = SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED},
			// Sensor
			// (ecs_enum_constant_t){.name = "SensorUpdate", .value = SDL_EVENT_SENSOR_UPDATE},
			// Pen
			// (ecs_enum_constant_t){.name = "PenProximityIn", .value = SDL_EVENT_PEN_PROXIMITY_IN},
			// (ecs_enum_constant_t){.name = "PenProximityOut", .value = SDL_EVENT_PEN_PROXIMITY_OUT},
			// (ecs_enum_constant_t){.name = "PenDown", .value = SDL_EVENT_PEN_DOWN},
			// (ecs_enum_constant_t){.name = "PenUp", .value = SDL_EVENT_PEN_UP},
			// (ecs_enum_constant_t){.name = "PenButtonDown", .value = SDL_EVENT_PEN_BUTTON_DOWN},
			// (ecs_enum_constant_t){.name = "PenButtonUp", .value = SDL_EVENT_PEN_BUTTON_UP},
			// (ecs_enum_constant_t){.name = "PenMotion", .value = SDL_EVENT_PEN_MOTION},
			// (ecs_enum_constant_t){.name = "PenAxis", .value = SDL_EVENT_PEN_AXIS},
			// Camera hotplug
			// (ecs_enum_constant_t){.name = "CameraDeviceAdded", .value = SDL_EVENT_CAMERA_DEVICE_ADDED},
			// (ecs_enum_constant_t){.name = "CameraDeviceRemoved", .value = SDL_EVENT_CAMERA_DEVICE_REMOVED},
			// (ecs_enum_constant_t){.name = "CameraDeviceApproved", .value = SDL_EVENT_CAMERA_DEVICE_APPROVED},
			// (ecs_enum_constant_t){.name = "CameraDeviceDenied", .value = SDL_EVENT_CAMERA_DEVICE_DENIED},
			// Render
			// (ecs_enum_constant_t){.name = "RenderTargetsReset", .value = SDL_EVENT_RENDER_TARGETS_RESET},
			// (ecs_enum_constant_t){.name = "RenderDeviceReset", .value = SDL_EVENT_RENDER_DEVICE_RESET},
			// (ecs_enum_constant_t){.name = "RenderDeviceLost", .value = SDL_EVENT_RENDER_DEVICE_LOST},
		},
	});
#endif
}

static void module([[maybe_unused]] ecs_world_t *unused)
{
	scope("Chirp")
	{
		tag("Engine");

		component("Assets", assets_t);
		component("Init", init_flags_t);
		component("WindowConfig", window_config_t);
		component("Window", window_t*);
		component("GpuDevice", gpu_device_t*);
		component("GpuGraphicsPipeline", gpu_graphics_pipeline_t*);
		component("DepthTexture", depth_texture_t*);
		component("GpuCommandBuffer", gpu_command_buffer_t*);
		component("GpuRenderPass", gpu_render_pass_t*);
		component("SwapchainTexture", swapchain_texture_t*);
		component("SwapchainTextureSize", swapchain_texture_size_t);
		component("Camera", camera_t);
		component("PhysicsConfig", physics_config_t);
		component("PhysicsEngine", physics_engine_t);
		component("Model", model_t);
		component("InstanceOf", instance_of_index_t);
		component("PhysicsBody", physics_body_id_t);
		component("Rotation", rotation_t);
		component("Position", position_t);
		component("Scale", scale_t);
		component("Projection", projection_t);
		component("ImGuiContext", imgui_context_t*);
		component("ImGuiDrawData", imgui_draw_data_t);
		component("VertexShader", vertex_shader_t*);
		component("FragmentShader", fragment_shader_t*);
		component("ClearColor", clear_color_t);
		component("ViewProjection", view_projection_t);
		component("Error", error_t);
		component("Input", input_t);
		component("ScriptEngine", py_vm_index_t);

#ifndef NDEBUG
		reflect("chirp.Init",
			(ecs_member_t){.name = "flags", .type = ecs_id(ecs_u32_t)},
		);

		reflect("chirp.WindowConfig",
			(ecs_member_t){.name = "title", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "size", .type = ecs_id(ecs_i32_t), .count = 2},
			(ecs_member_t){.name = "fullscreen", .type = ecs_id(ecs_bool_t)},
		);

		reflect("chirp.Camera",
			(ecs_member_t){.name = "position", .type = ecs_id(ecs_f32_t), .count = 3},
			(ecs_member_t){.name = "target", .type = ecs_id(ecs_f32_t), .count = 3},
			(ecs_member_t){.name = "up", .type = ecs_id(ecs_f32_t), .count = 3},
			(ecs_member_t){.name = "fov_y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "near_plane", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "far_plane", .type = ecs_id(ecs_f32_t)},
		);

		reflect("chirp.PhysicsConfig",
			(ecs_member_t){.name = "move_speed", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "max_move_speed", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "gravity_y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "jump_speed", .type = ecs_id(ecs_f32_t)},
		);

		reflect("chirp.InstanceOf",
			(ecs_member_t){.name = "index", .type = ecs_id(ecs_uptr_t)},
		);

		reflect("chirp.PhysicsBody",
			(ecs_member_t){.name = "id", .type = ecs_id(ecs_u32_t)},
		);

		reflect("chirp.Rotation",
			(ecs_member_t){.name = "x", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "z", .type = ecs_id(ecs_f32_t)},
		);

		reflect("chirp.Position",
			(ecs_member_t){.name = "x", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "z", .type = ecs_id(ecs_f32_t)},
		);

		reflect("chirp.Scale",
			(ecs_member_t){.name = "x", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "y", .type = ecs_id(ecs_f32_t)},
			(ecs_member_t){.name = "z", .type = ecs_id(ecs_f32_t)},
		);

		reflect("chirp.Projection",
			(ecs_member_t){.name = "rebuild", .type = ecs_id(ecs_bool_t)},
			(ecs_member_t){.name = "value", .type = ecs_id(ecs_f32_t), .count = 16},
		);

		reflect("chirp.Error",
			(ecs_member_t){.name = "title", .type = ecs_id(ecs_string_t)},
			(ecs_member_t){.name = "message", .type = ecs_id(ecs_string_t)},
		);

		reflect("chirp.Input",
			(ecs_member_t){.name = "key_map", .type = ecs_id(ecs_u32_t)},
			(ecs_member_t){.name = "button_map", .type = ecs_id(ecs_u32_t)},
			(ecs_member_t){.name = "name_map", .type = ecs_id(ecs_u32_t)},
		);

		reflect("chirp.ScriptEngine",
			(ecs_member_t){.name = "vm_index", .type = ecs_id(ecs_i32_t)},
		);
#endif

		tag("Scene");

		create_pipeline();
	}

	scope("ChirpEvent")
	{
		add_events();
	}
}

static void on_init_set([[maybe_unused]] ecs_iter_t *iter)
{
	ecs_os_api_t os_api = ecs_os_api_create();
	ecs_os_set_api(&os_api);

	const int cores = SDL_GetNumLogicalCPUCores();
	ecs_set_threads(world, cores);
	SDL_LogDebug(LOG_CATEGORY_ECS, "Using %d ECS threads", cores);

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
			(ecs_term_t){
				.id = ecs_lookup(world, "chirp.Init"),
			}
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

	const ecs_id_t error_id = ecs_lookup(world, "chirp.Error");
	ecs_set_id(world, entity, error_id, sizeof(error_t), &error);

	return entity;
}

const void *ecs_const_data(const char *name)
{
	const ecs_entity_t entity = ecs_lookup(world, "chirp.Engine");
	const ecs_id_t component = ecs_lookup(world, name);

	if (entity == 0 || component == 0)
	{
		SDL_LogWarn(LOG_CATEGORY_ECS, "Unknown component: %s", name);
		return nullptr;
	}

	return ecs_get_id(world, entity, component);
}

void *ecs_mut_data_ptr(const char *name)
{
	const void *data = ecs_const_data(name);
	if (data == nullptr)
	{
		return nullptr;
	}

	return *((void**) data);
}

void *ecs_mut_data(const char *name)
{
	const ecs_entity_t entity = ecs_lookup(world, "chirp.Engine");
	const ecs_id_t component = ecs_lookup(world, name);

	if (entity == 0 || component == 0)
	{
		SDL_LogWarn(LOG_CATEGORY_ECS, "Unknown component: %s", name);
		return nullptr;
	}

	return ecs_get_mut_id(world, entity, component);
}
