#include "resources.h"

#include <SDL3/SDL_iostream.h>

static const Uint8 font_monogram_ttf[] =
{
#embed FONT_MONOGRAM_TTF_PATH
};

static const Uint8 font_maple_mono_nl_regular_ttf[] =
{
#embed FONT_MAPLE_MONO_NL_REGULAR_TTF_PATH
};

static const Uint8 shader_default_vert_dxil[] =
{
#embed SHADER_DEFAULT_VERT_DXIL_PATH
};

static const Uint8 shader_default_frag_dxil[] =
{
#embed SHADER_DEFAULT_FRAG_DXIL_PATH
};

static const Uint8 shader_default_vert_msl[] =
{
#embed SHADER_DEFAULT_VERT_MSL_PATH
};

static const Uint8 shader_default_frag_msl[] =
{
#embed SHADER_DEFAULT_FRAG_MSL_PATH
};

static const Uint8 shader_default_vert_spv[] =
{
#embed SHADER_DEFAULT_VERT_SPV_PATH
};

static const Uint8 shader_default_frag_spv[] =
{
#embed SHADER_DEFAULT_FRAG_SPV_PATH
};

SDL_IOStream *res_font_monogram_ttf()
{
	return SDL_IOFromConstMem(font_monogram_ttf, sizeof(font_monogram_ttf));
}

SDL_IOStream *res_font_maple_mono_nl_regular_ttf()
{
	return SDL_IOFromConstMem(font_maple_mono_nl_regular_ttf, sizeof(font_maple_mono_nl_regular_ttf));
}

SDL_IOStream *res_shader_default_vert_dxil()
{
	return SDL_IOFromConstMem(shader_default_vert_dxil, sizeof(shader_default_vert_dxil));
}

SDL_IOStream *res_shader_default_frag_dxil()
{
	return SDL_IOFromConstMem(shader_default_frag_dxil, sizeof(shader_default_frag_dxil));
}

SDL_IOStream *res_shader_default_vert_msl()
{
	return SDL_IOFromConstMem(shader_default_vert_msl, sizeof(shader_default_vert_msl));
}

SDL_IOStream *res_shader_default_frag_msl()
{
	return SDL_IOFromConstMem(shader_default_frag_msl, sizeof(shader_default_frag_msl));
}

SDL_IOStream *res_shader_default_vert_spv()
{
	return SDL_IOFromConstMem(shader_default_vert_spv, sizeof(shader_default_vert_spv));
}

SDL_IOStream *res_shader_default_frag_spv()
{
	return SDL_IOFromConstMem(shader_default_frag_spv, sizeof(shader_default_frag_spv));
}
