#include "ui/debugoverlay.h"
#include "appstate.h"
#include "audiodriver.h"
#include "camera.h"
#include "gpudevicedriver.h"
#include "physics.h"
#include "physicsconfig.h"
#include "systeminfo.h"
#include "videodriver.h"

#include "dcimgui.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

typedef enum [[clang::flag_enum]] debug_overlay_elements_t
{
	DEBUG_OVERLAY_DELTA   = 1 << 0,
	DEBUG_OVERLAY_SYSTEM  = 1 << 1,
	DEBUG_OVERLAY_CAMERA  = 1 << 2,
	DEBUG_OVERLAY_PHYSICS = 1 << 3,
} debug_overlay_elements_t;

static void draw_delta(const float delta)
{
	constexpr auto ms_s = 1'000.F;

	ImGui_TableNextColumn();
	ImGui_Text("Delta");
	ImGui_TableNextColumn();
	ImGui_Text("%.2f ms", delta * ms_s);
}

static void draw_system_info(SDL_GPUDevice *device)
{
	ImGui_TableNextColumn();
	ImGui_Text("CPU");
	ImGui_TableNextColumn();
	ImGui_Text("%s", system_info_cpu_name());

	ImGui_TableNextColumn();
	ImGui_Text("GPU");
	ImGui_TableNextColumn();
	ImGui_Text("%s", system_info_gpu_name(device));

	ImGui_TableNextColumn();
	ImGui_Text("Driver");
	ImGui_TableNextColumn();
	ImGui_Text("%s", system_info_gpu_driver(device));

	ImGui_TableNextColumn();
	ImGui_Text("Video");
	ImGui_TableNextColumn();
	ImGui_Text("%s", video_driver_display_name(SDL_GetCurrentVideoDriver()));

	ImGui_TableNextColumn();
	ImGui_Text("Audio");
	ImGui_TableNextColumn();
	ImGui_Text("%s", audio_driver_display_name(SDL_GetCurrentAudioDriver()));

	ImGui_TableNextColumn();
	ImGui_Text("Renderer");
	ImGui_TableNextColumn();
	ImGui_Text("%s", gpu_device_driver_display_name(SDL_GetGPUDeviceDriver(device)));
}

static void draw_camera_info(const camera_t camera)
{
	ImGui_TableNextColumn();
	ImGui_Text("Camera");
	ImGui_TableNextColumn();
	ImGui_Text("%-6.2f %-6.2f %-6.2f",
		camera.position.x, camera.position.y, camera.position.z);

	ImGui_TableNextColumn();
	ImGui_Text("Target");
	ImGui_TableNextColumn();
	ImGui_Text("%-6.2f %-6.2f %-6.2f",
		camera.target.x, camera.target.y, camera.target.z);
}

static void draw_physics_info(const physics_engine_t *physics_engine,
	const physics_body_id_t body_id)
{
	const vector3f_t position = physics_body_position(physics_engine, body_id);
	const vector3f_t velocity = physics_body_linear_velocity(physics_engine, body_id);

	ImGui_TableNextColumn();
	ImGui_Text("Position");
	ImGui_TableNextColumn();
	ImGui_Text("%-6.2f %-6.2f %-6.2f", position.x, position.y, position.z);

	ImGui_TableNextColumn();
	ImGui_Text("Velocity");
	ImGui_TableNextColumn();
	ImGui_Text("%-6.2f %-6.2f %-6.2f", velocity.x, velocity.y, velocity.z);
}

static void menu_element_item(const char *label,
	debug_overlay_elements_t *elements, const debug_overlay_elements_t element)
{
	const bool selected = (*elements & element) > 0;
	if (ImGui_MenuItemEx(label, nullptr, selected, true))
	{
		*elements ^= element;
	}
}

static void show_physics_properties(bool *open,
	const physics_engine_t *engine, physics_config_t *config)
{
	constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	ImGui_SetNextWindowBgAlpha(0.75F);

	ImGui_Begin("physics properties", open, flags);
	{
		constexpr auto step = 10.F;
		constexpr float min_value = -1'000.F;
		constexpr float max_value = +1'000.F;

		ImGui_DragFloatEx("Move speed", &config->move_speed, step,
			min_value, max_value, "%.f", ImGuiSliderFlags_None);

		ImGui_DragFloatEx("Max move speed", &config->max_move_speed, step,
			min_value, max_value, "%.f", ImGuiSliderFlags_None);

		if (ImGui_DragFloatEx("Gravity", &config->gravity_y, step,
			min_value, max_value, "%.f", ImGuiSliderFlags_None))
		{
			const vector3f_t gravity = {.y = -config->gravity_y};
			physics_set_gravity(engine, gravity);
		}

		ImGui_DragFloatEx("Jump speed", &config->jump_speed, step,
			min_value, max_value, "%.f", ImGuiSliderFlags_None);
	}
	ImGui_End();
}

void draw_debug_overlay(app_state_t *state)
{
	static auto open = true;
	static auto physics_open = false;

#ifndef IMGUI_DISABLE_DEMO_WINDOWS
	static auto demo_open = false;
#endif

#ifdef NDEBUG
	static debug_overlay_elements_t elements = 0;
#else
	static debug_overlay_elements_t elements = DEBUG_OVERLAY_CAMERA | DEBUG_OVERLAY_PHYSICS;
#endif

#ifndef IMGUI_DISABLE_DEMO_WINDOWS
	if (demo_open)
	{
		ImGui_ShowDemoWindow(&demo_open);
	}
#endif

	if (physics_open)
	{
		show_physics_properties(&physics_open,
			state->physics_engine, &state->physics_config);
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

		ImGui_Text("%s %s", ENGINE_NAME, ENGINE_VERSION); // NOLINT(*-include-cleaner)

		if (ImGui_BeginTable("table_debug", 2, ImGuiTableFlags_None))
		{
			ImGui_TableNextColumn();
			ImGui_Text("FPS");
			ImGui_TableNextColumn();
			ImGui_Text("%u", state->time.fps);

			if ((elements & DEBUG_OVERLAY_DELTA) > 0)
			{
				draw_delta(state->dt);
			}

			if ((elements & DEBUG_OVERLAY_SYSTEM) > 0)
			{
				draw_system_info(state->device);
			}

			if ((elements & DEBUG_OVERLAY_CAMERA) > 0)
			{
				draw_camera_info(state->camera);
			}

			if ((elements & DEBUG_OVERLAY_PHYSICS) > 0)
			{
				draw_physics_info(state->physics_engine, state->player_body_id);
			}

			ImGui_EndTable();
		}

		if (ImGui_BeginPopupContextWindow())
		{
			menu_element_item("Delta time",
				&elements, DEBUG_OVERLAY_DELTA);

			menu_element_item("System info",
				&elements, DEBUG_OVERLAY_SYSTEM);

			menu_element_item("Camera positions",
				&elements, DEBUG_OVERLAY_CAMERA);

			menu_element_item("Player physics",
				&elements, DEBUG_OVERLAY_PHYSICS);

			ImGui_Separator();

#ifndef IMGUI_DISABLE_DEMO_WINDOWS
			if (ImGui_MenuItemEx("ImGui demo", nullptr, demo_open, true))
			{
				demo_open = (int) demo_open == 0;
			}
#endif

			if (ImGui_MenuItemEx("Physics properties", nullptr, physics_open, true))
			{
				physics_open = (int) physics_open == 0;
			}

			ImGui_EndPopup();
		}

		ImGui_PopFont();
	}
	ImGui_End();
}
