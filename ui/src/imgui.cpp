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

void imgui_shutdown()
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplSDLGPU3_Shutdown();
}
