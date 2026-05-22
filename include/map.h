#pragma once

#include <SDL3/SDL_properties.h>

typedef SDL_PropertiesID map_t;

#define map_create   SDL_CreateProperties
#define map_contains SDL_HasProperty

// Custom callback to directly support common "destroy" functions
typedef void (*map_cleanup_callback_t)(void *value);

#define map_get(map,name,fallback)		\
	_Generic((fallback),				\
		void*:  SDL_GetPointerProperty,	\
		char*:  SDL_GetStringProperty,	\
		Sint64: SDL_GetNumberProperty,	\
		int:    SDL_GetNumberProperty,	\
		Uint32: SDL_GetNumberProperty,	\
		float:  SDL_GetFloatProperty,	\
		bool:   SDL_GetBooleanProperty	\
	)(map,name,fallback)

#define map_set(map,name,value)			\
	_Generic((value),					\
		void*:  SDL_SetPointerProperty,	\
		char*:  SDL_SetStringProperty,	\
		Sint64: SDL_SetNumberProperty,	\
		int:    SDL_SetNumberProperty,	\
		Uint32: SDL_SetNumberProperty,	\
		float:  SDL_SetFloatProperty,	\
		bool:   SDL_SetBooleanProperty	\
	)(map,name,value)

bool map_set_with_cleanup(map_t map, const char *name,
	void *value, map_cleanup_callback_t callback);
