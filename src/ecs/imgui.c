#include "ecs.h"
#include "resources.h"
#include "ecs/components.h"
#include "ecs/tags.h"

#include "dcimgui.h"
#include "flecs.h"
#include "backends/dcimgui_impl_sdl3.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

static void on_window_gpu_set(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);
	SDL_GPUDevice *gpu_device = *ecs_field(iter, gpu_device_t*, 1);

	CIMGUI_CHECKVERSION();

	ImGuiContext *context = ImGui_CreateContext(nullptr);
	if (context == nullptr)
	{
		ecs_set_error("ImGui error", "Failed to initialise context");
		return;
	}

	ImGuiIO *im_io = ImGui_GetIO();
	im_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

	// ImGui wants to own and free the data
	SDL_IOStream *font_stream = res_font_maple_mono_nl_regular_ttf();
	size_t font_size = 0;
	void *font_data = SDL_LoadFile_IO(font_stream, &font_size, true);

	if (ImFontAtlas_AddFontFromMemoryTTF(im_io->Fonts, font_data, (int) font_size,
		16.F, nullptr, nullptr) == nullptr)
	{
		ecs_set_error("ImGui error", "Failed to load font");
		return;
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

	if (!cImGui_ImplSDL3_InitForSDLGPU(window))
	{
		ecs_set_error("ImGui error", "Failed to initialise SDL3 backend");
		return;
	}

	ImGui_ImplSDLGPU3_InitInfo init_info = {
		.Device = gpu_device,
		.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
		.MSAASamples = SDL_GPU_SAMPLECOUNT_1,
		.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		.PresentMode = SDL_GPU_PRESENTMODE_VSYNC,
	};

	if (!cImGui_ImplSDLGPU3_Init(&init_info))
	{
		ecs_set_error("ImGui error", "Failed to initialise SDL3 GPU backend");
		return;
	}

	ecs_set_id(ecs_world(), EcsEngine, EcsImGuiContext,
		sizeof(ImGuiContext*), (void*) &context);
}

static void render(ecs_iter_t *iter)
{
	SDL_GPUCommandBuffer *command_buffer = *ecs_field(iter, gpu_command_buffer_t*, 0);
	SDL_GPURenderPass *render_pass = *ecs_field(iter, gpu_render_pass_t*, 1);
	ImDrawData *draw_data = ecs_field(iter, imgui_draw_data_t, 2);

	cImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
}

void ecs_add_imgui()
{
	const ecs_observer_desc_t observer_desc = {
		.query.terms = {
			(ecs_term_t){.id = EcsWindow},
			(ecs_term_t){.id = EcsGpuDevice},
		},
		.events = {EcsOnSet},
		.callback = on_window_gpu_set,
	};
	ecs_observer_init(ecs_world(), &observer_desc);

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "RenderScene",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_RENDER))),
		}),
		.query.expr = "[in] chirp.GpuCommandBuffer, [in] chirp.GpuRenderPass, [in] chirp.ImGuiDrawData",
		.callback = render,
	});
}
