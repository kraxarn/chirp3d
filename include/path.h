#pragma once

#ifdef SDL_PLATFORM_WINDOWS
constexpr auto path_separator = '\\';
#else
constexpr auto path_separator = '/';
#endif
