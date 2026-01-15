#include "ui/debugoverlay.h"
#include "appstate.h"
#include "audiodriver.h"
#include "gpudevicedriver.h"
#include "physics.h"
#include "videodriver.h"
#include "systeminfo.h"

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

void draw_debug_overlay(const app_state_t *state)
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

		ImGui_Text("%s %s", ENGINE_NAME, ENGINE_VERSION); // NOLINT(*-include-cleaner)

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
				ImGui_TableNextColumn();
				ImGui_Text("CPU");
				ImGui_TableNextColumn();
				ImGui_Text("%s", system_info_cpu_name());

				ImGui_TableNextColumn();
				ImGui_Text("GPU");
				ImGui_TableNextColumn();
				ImGui_Text("%s", system_info_gpu_name(state->device));

				ImGui_TableNextColumn();
				ImGui_Text("Driver");
				ImGui_TableNextColumn();
				ImGui_Text("%s", system_info_gpu_driver(state->device));

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
				demo_open = (int) demo_open == 0;
			}

			ImGui_EndPopup();
		}

		ImGui_PopFont();
	}
	ImGui_End();
}
