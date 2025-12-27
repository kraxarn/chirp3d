#pragma once

#include <SDL3/SDL_stdinc.h>

const Uint8 shader_default_vert_dxil[] =
{
#embed "resources/shaders/dxil/default.vert.dxil"
};

const Uint8 shader_default_frag_dxil[] =
{
#embed "resources/shaders/dxil/default.frag.dxil"
};

const Uint8 shader_default_vert_msl[] =
{
#embed "resources/shaders/msl/default.vert.msl"
};

const Uint8 shader_default_frag_msl[] =
{
#embed "resources/shaders/msl/default.frag.msl"
};

const Uint8 shader_default_vert_spv[] =
{
#embed "resources/shaders/spv/default.vert.spv"
};

const Uint8 shader_default_frag_spv[] =
{
#embed "resources/shaders/spv/default.frag.spv"
};

const Uint8 texture_wall_qoi[] =
{
#embed "resources/textures/wall.qoi"
};
