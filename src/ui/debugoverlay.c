#include "ui/debugoverlay.h"
#include "audiodriver.h"
#include "camera.h"
#include "ecs.h"
#include "gpudevicedriver.h"
#include "nkui.h"
#include "systeminfo.h"
#include "videodriver.h"

#include "flecs.h"
#include "box3d/box3d.h"
#include "box3d/id.h"
#include "box3d/math_functions.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

static void draw_system_info(nk_context_t *ctx, SDL_GPUDevice *device)
{
	nk_label(ctx, "Platform", NK_TEXT_LEFT);
	nk_label(ctx, system_info_platform(), NK_TEXT_LEFT);

	nk_label(ctx, "CPU", NK_TEXT_LEFT);
	nk_label(ctx, system_info_cpu_name(), NK_TEXT_LEFT);

	nk_label(ctx, "GPU", NK_TEXT_LEFT);
	nk_label(ctx, system_info_gpu_name(device), NK_TEXT_LEFT);

	nk_label(ctx, "Driver", NK_TEXT_LEFT);
	nk_label(ctx, system_info_gpu_driver(device), NK_TEXT_LEFT);

	nk_label(ctx, "Video", NK_TEXT_LEFT);
	nk_label(ctx, video_driver_display_name(SDL_GetCurrentVideoDriver()), NK_TEXT_LEFT);

	nk_label(ctx, "Audio", NK_TEXT_LEFT);
	nk_label(ctx, audio_driver_display_name(SDL_GetCurrentAudioDriver()), NK_TEXT_LEFT);

	nk_label(ctx, "Renderer", NK_TEXT_LEFT);
	nk_label(ctx, gpu_device_driver_display_name(SDL_GetGPUDeviceDriver(device)), NK_TEXT_LEFT);
}

static void draw_camera_info(nk_context_t *ctx, const camera_t *camera)
{
	nk_label(ctx, "Camera", NK_TEXT_LEFT);
	nk_labelf(ctx, NK_TEXT_LEFT, "%-6.2f %-6.2f %-6.2f",
		camera->position.x, camera->position.y, camera->position.z);

	nk_label(ctx, "Target", NK_TEXT_LEFT);
	nk_labelf(ctx, NK_TEXT_LEFT, "%-6.2f %-6.2f %-6.2f",
		camera->target.x, camera->target.y, camera->target.z);
}

static void draw_physics_info(nk_context_t *ctx, const b3BodyId body_id)
{
	const b3Vec3 position = b3Body_GetPosition(body_id);
	const b3Vec3 velocity = b3Body_GetLinearVelocity(body_id);

	nk_label(ctx, "Position", NK_TEXT_LEFT);
	nk_labelf(ctx, NK_TEXT_LEFT, "%-6.2f %-6.2f %-6.2f",
		position.x, position.y, position.z);

	nk_label(ctx, "Velocity", NK_TEXT_LEFT);
	nk_labelf(ctx, NK_TEXT_LEFT, "%-6.2f %-6.2f %-6.2f",
		velocity.x, velocity.y, velocity.z);
}

void draw_debug_overlay(ecs_iter_t *iter)
{
	nk_context_t *ctx = &ecs_field(iter, nkui_context_t, 0)->nk;
	const camera_t *camera = ecs_field(iter, camera_t, 1);
	const b3BodyId player_body_id = *ecs_field(iter, b3BodyId, 2);
	SDL_Window *window = *ecs_field(iter, SDL_Window*, 3);
	SDL_GPUDevice *device = *ecs_field(iter, SDL_GPUDevice*, 4);

#ifdef FLECS_STATS
	const EcsWorldSummary *world_summary = ecs_get_id(ecs_world(),
		EcsWorld, ecs_id(EcsWorldSummary));
#endif

	constexpr auto padding = 16.F;
	constexpr auto alpha = 0.75F;
	constexpr auto row_height = 18.F;

	const nk_rect_t window_bounds = {
		.x = padding,
		.y = padding,
		.w = 300.F,
		.h = 180.F,
	};

	constexpr nk_panel_flags_t window_flags =
		NK_WINDOW_BORDER;

	static double fps = 0.0;
	static Uint32 fps_timestamp = 0;

	// TODO: Use app_state_t fps
#ifdef FLECS_STATS
	if (fps == 0.0 || world_summary->uptime != fps_timestamp)
	{
		fps = world_summary->fps;
		fps_timestamp = world_summary->uptime;
	}
#endif

	nk_style_t *style = &ctx->style;
	nk_style_item_t *window_style = &style->window.fixed_background;

	SDL_assert(window_style->type == NK_STYLE_ITEM_COLOR);
	nk_color_t color = window_style->data.color;
	color.a = (nk_byte) ((float) color.a * alpha);

	nk_style_push_style_item(ctx, window_style, nk_style_item_color(color));

	if (nk_begin(ctx, "Debug overlay", window_bounds, window_flags))
	{
		constexpr auto ms_s = 1'000.F;

#ifndef NDEBUG
		nk_layout_row_dynamic(ctx, row_height, 1);
		nk_label(ctx, "- debug mode -", NK_TEXT_CENTERED);
		nk_label(ctx, ENGINE_NAME " " ENGINE_VERSION, NK_TEXT_LEFT);
#endif

		nk_layout_row(ctx, NK_DYNAMIC, row_height, 2, (float[]){0.3F, 0.7F});

		nk_label(ctx, "FPS", NK_TEXT_LEFT);
		nk_labelf(ctx, NK_TEXT_LEFT, "%3.0f (%5.2f ms)",
			fps, iter->delta_time * ms_s);

		draw_camera_info(ctx, camera);
		draw_physics_info(ctx, player_body_id);
	}
	nk_end(ctx);

#ifdef FLECS_STATS
	if (nk_begin(ctx, "World overlay", (nk_rect_t){
		.w = 300.F,
		.h = 150.F,
		.x = padding,
		.y = (padding * 2) + 180.F,
	}, NK_WINDOW_BORDER))
	{
		nk_layout_row(ctx, NK_DYNAMIC, row_height, 2, (float[]){0.4F, 0.6F});

		nk_label(ctx, "Entities", NK_TEXT_LEFT);
		nk_labelf(ctx, NK_TEXT_LEFT, "%ld",
			world_summary->entity_count);

		nk_label(ctx, "Tables", NK_TEXT_LEFT);
		nk_labelf(ctx, NK_TEXT_LEFT, "%ld",
			world_summary->table_count);

		nk_label(ctx, "Systems", NK_TEXT_LEFT);
		nk_labelf(ctx, NK_TEXT_LEFT, "%ld",
			world_summary->systems_ran_frame);

		nk_label(ctx, "Observers", NK_TEXT_LEFT);
		nk_labelf(ctx, NK_TEXT_LEFT, "%ld",
			world_summary->observers_ran_frame);

		nk_label(ctx, "Queries", NK_TEXT_LEFT);
		nk_labelf(ctx, NK_TEXT_LEFT, "%ld",
			world_summary->queries_ran_frame);

		nk_label(ctx, "Commands", NK_TEXT_LEFT);
		nk_labelf(ctx, NK_TEXT_LEFT, "%ld",
			world_summary->command_count_frame);
	}
	nk_end(ctx);
#endif

	if (!SDL_GetWindowRelativeMouseMode(window))
	{
		int width = 0;
		SDL_GetWindowSize(window, &width, nullptr);

		constexpr float window_width = 300;
		const float window_x = (float) width - window_width - padding;

		if (nk_begin(ctx, "Debug lock info", (nk_rect_t){
				.x = window_x,
				.y = padding,
				.w = window_width,
				.h = 40.F,
			}, NK_WINDOW_BORDER
		))
		{
			nk_layout_row_dynamic(ctx, row_height, 1);
			nk_labelf(ctx, NK_TEXT_CENTERED, "Press %s to lock cursor",
				SDL_GetKeyName(SDLK_ESCAPE));
		}
		nk_end(ctx);

		if (nk_begin(ctx, "System info", (nk_rect_t){
			.x = window_x,
			.y = (padding * 2.F) + 40.F,
			.w = window_width,
			.h = 210.F,
		}, NK_WINDOW_BORDER | NK_WINDOW_TITLE))
		{
			nk_layout_row(ctx, NK_DYNAMIC, row_height, 2, (float[]){0.3F, 0.7F});
			draw_system_info(ctx, device);
		}
		nk_end(ctx);

		if (nk_begin(ctx, "Build info", (nk_rect_t){
			.x = window_x,
			.y = (padding * 3.F) + 40.F + 210.F,
			.w = window_width,
			.h = 120.F,
		}, NK_WINDOW_BORDER | NK_WINDOW_TITLE))
		{
			nk_layout_row(ctx, NK_DYNAMIC, row_height, 2, (float[]){0.3F, 0.7F});

			nk_label(ctx, "Date", NK_TEXT_LEFT);
			nk_label(ctx, __DATE__ " " __TIME__, NK_TEXT_LEFT);

			nk_label(ctx, "Compiler", NK_TEXT_LEFT);
			nk_label(ctx, ENGINE_COMPILER, NK_TEXT_LEFT);

			nk_label(ctx, "Type", NK_TEXT_LEFT);
			nk_label(ctx, ENGINE_BUILD_TYPE, NK_TEXT_LEFT);
		}
		nk_end(ctx);
	}

	nk_style_pop_style_item(ctx);
}
