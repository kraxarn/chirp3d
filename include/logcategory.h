#pragma once
#include <SDL3/SDL_log.h>

typedef enum log_category_t
{
	LOG_CATEGORY_CORE = SDL_LOG_CATEGORY_CUSTOM,
	LOG_CATEGORY_RENDER,
	LOG_CATEGORY_FONT,
} log_category_t;
