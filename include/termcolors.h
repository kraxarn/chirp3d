#pragma once

#include <SDL3/SDL_platform_defines.h>

// Assume this only works on unix systems
#ifdef SDL_PLATFORM_UNIX

#define COLOR_FG_RESET "\x1b[0m"

#define COLOR_FG_BOLD(str) "\x1b[1m" str "\x1b[22m"

#define COLOR_FG_RED(str)     "\x1b[31m" str COLOR_FG_RESET
#define COLOR_FG_GREEN(str)   "\x1b[32m" str COLOR_FG_RESET
#define COLOR_FG_YELLOW(str)  "\x1b[33m" str COLOR_FG_RESET
#define COLOR_FG_BLUE(str)    "\x1b[34m" str COLOR_FG_RESET
#define COLOR_FG_MAGENTA(str) "\x1b[35m" str COLOR_FG_RESET
#define COLOR_FG_CYAN(str)    "\x1b[36m" str COLOR_FG_RESET
#define COLOR_FG_WHITE(str)   "\x1b[37m" str COLOR_FG_RESET

#else

#define COLOR_FG_BOLD(str) str

#define COLOR_FG_RED(str)     str
#define COLOR_FG_GREEN(str)   str
#define COLOR_FG_YELLOW(str)  str
#define COLOR_FG_BLUE(str)    str
#define COLOR_FG_MAGENTA(str) str
#define COLOR_FG_CYAN(str)    str
#define COLOR_FG_WHITE(str)   str

#endif
