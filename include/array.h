#pragma once

#include <SDL3/SDL_stdinc.h>

typedef struct array_header_t
{
	size_t count;
	size_t capacity;
} array_header_t;

#define _array_default_capacity 32

#define _array_create(arr, size, cap)												\
	do {																			\
		if ((arr) == nullptr)	{													\
			constexpr size_t init_size = sizeof(array_header_t) + ((size) * (cap));	\
			array_header_t *header = SDL_malloc(init_size);							\
			header->count = 0;														\
			header->capacity = (cap);												\
			(arr) = (void*) (header + 1);											\
		}																			\
	} while (false)

#define _array_header(arr) (((array_header_t*) (arr)) - 1)

#define array_destroy(arr)					\
	do {									\
		if ((arr) != nullptr) {				\
			SDL_free(_array_header(arr));	\
		}									\
	} while (false)

#define array_size(arr)     _array_header(arr)->count
#define array_capacity(arr) _array_header(arr)->capacity

#define _array_resize(arr, size)																	\
	do {																							\
		array_capacity(arr) = (size);																\
		const size_t new_size = sizeof(array_header_t) + (sizeof(*(arr)) * array_capacity(arr));	\
		array_header_t *header = SDL_realloc(_array_header(arr), new_size);							\
		(arr) = (void*) (header + 1);																\
	} while (false)

#define array_reserve(arr, size)							\
	do {													\
		if ((arr) == nullptr) {								\
			_array_create((arr), sizeof(*(arr)), (size));	\
		} else if (array_capacity(arr) < (size)) {			\
			_array_resize((arr), (size));					\
		}													\
	} while (false)

#define array_push(arr, item)											\
	do {																\
		_array_create((arr), sizeof(*(arr)), _array_default_capacity);	\
		if (array_size(arr) == array_capacity(arr)) {					\
			_array_resize((arr), array_capacity(arr) * 2);				\
		}																\
		(arr)[array_size(arr)++] = (item);								\
	} while (false)
