#include "ui/imgui.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"

auto imgui_create_context(const imgui_config_flags_t config_flags) -> bool
{
	IMGUI_CHECKVERSION();

	if (ImGui::CreateContext() == nullptr)
	{
		return false;
	}

	ImGuiIO &ig_io = ImGui::GetIO();
	ig_io.ConfigFlags |= config_flags;

	return true;
}

void imgui_destroy_context()
{
	ImGui::DestroyContext();
}

void imgui_shutdown()
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplSDLGPU3_Shutdown();
}

void imgui_style_colors_dark()
{
	ImGui::StyleColorsDark();
}

void imgui_style_colors_light()
{
	ImGui::StyleColorsLight();
}

void imgui_set_scale(const float scale)
{
	ImGuiStyle &style = ImGui::GetStyle();

	style.ScaleAllSizes(scale);
	style.FontScaleDpi = scale;
}

auto imgui_init_for_sdl3gpu(SDL_Window *window, SDL_GPUDevice *device) -> bool
{
	if (!ImGui_ImplSDL3_InitForSDLGPU(window))
	{
		return false;
	}

	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device = device;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
	init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;

	return ImGui_ImplSDLGPU3_Init(&init_info);
}

auto imgui_process_event(const SDL_Event *event) -> bool
{
	return ImGui_ImplSDL3_ProcessEvent(event);
}

void imgui_new_frame()
{
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void imgui_show_demo_window(bool *open)
{
	ImGui::ShowDemoWindow(open);
}

void imgui_render()
{
	ImGui::Render();
}

auto imgui_draw_data() -> imgui_draw_data_t *
{
	return ImGui::GetDrawData();
}

void imgui_prepare_draw_data(imgui_draw_data_t *draw_data, SDL_GPUCommandBuffer *command_buffer)
{
	ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
}

void imgui_render_draw_data(imgui_draw_data_t *draw_data,
	SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass)
{
	ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
}

auto imgui_want_capture_mouse() -> bool
{
	const ImGuiIO &ig_io = ImGui::GetIO();
	return ig_io.WantCaptureMouse;
}
