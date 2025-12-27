#pragma once

#include <SDL3/SDL_stdinc.h>

const Uint8 shader_default_vert_dxil[] =
{
#embed SHADER_DEFAULT_VERT_DXIL_PATH
};

const Uint8 shader_default_frag_dxil[] =
{
#embed SHADER_DEFAULT_FRAG_DXIL_PATH
};

const Uint8 shader_default_vert_msl[] =
{
#embed SHADER_DEFAULT_VERT_MSL_PATH
};

const Uint8 shader_default_frag_msl[] =
{
#embed SHADER_DEFAULT_FRAG_MSL_PATH
};

const Uint8 shader_default_vert_spv[] =
{
#embed SHADER_DEFAULT_VERT_SPV_PATH
};

const Uint8 shader_default_frag_spv[] =
{
#embed SHADER_DEFAULT_FRAG_SPV_PATH
};

const Uint8 texture_wall_qoi[] =
{
#embed TEXTURE_WALL_QOI_PATH
};
