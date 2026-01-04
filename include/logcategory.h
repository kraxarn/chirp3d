#pragma once
#include <SDL3/SDL_log.h>

typedef enum log_category_t
{
	LOG_CATEGORY_CORE = SDL_LOG_CATEGORY_CUSTOM,
	LOG_CATEGORY_RENDER,
	LOG_CATEGORY_FONT,
	LOG_CATEGORY_ASSETS,
	LOG_CATEGORY_INPUT,
} log_category_t;
