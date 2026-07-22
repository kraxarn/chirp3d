#pragma once

#include <SDL3/SDL_iostream.h>

[[nodiscard]] SDL_IOStream *res_font_cousine_ttf();

[[nodiscard]] SDL_IOStream *res_shader_default_vert_dxil();
[[nodiscard]] SDL_IOStream *res_shader_default_frag_dxil();
[[nodiscard]] SDL_IOStream *res_shader_default_vert_msl();
[[nodiscard]] SDL_IOStream *res_shader_default_frag_msl();
[[nodiscard]] SDL_IOStream *res_shader_default_vert_spv();
[[nodiscard]] SDL_IOStream *res_shader_default_frag_spv();

[[nodiscard]] SDL_IOStream *res_shader_nkui_vert_dxil();
[[nodiscard]] SDL_IOStream *res_shader_nkui_frag_dxil();
[[nodiscard]] SDL_IOStream *res_shader_nkui_vert_msl();
[[nodiscard]] SDL_IOStream *res_shader_nkui_frag_msl();
[[nodiscard]] SDL_IOStream *res_shader_nkui_vert_spv();
[[nodiscard]] SDL_IOStream *res_shader_nkui_frag_spv();
