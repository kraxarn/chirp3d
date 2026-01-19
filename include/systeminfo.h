#pragma once

#include <SDL3/SDL_gpu.h>

[[nodiscard]]
const char *system_info_platform();

[[nodiscard]]
const char *system_info_cpu_name();

[[nodiscard]]
const char *system_info_gpu_name(SDL_GPUDevice *device);

[[nodiscard]]
const char *system_info_gpu_driver(SDL_GPUDevice *device);
