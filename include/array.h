#pragma once

#include <SDL3/SDL_stdinc.h>

typedef struct array_header_t
{
	size_t count;
	size_t capacity;
} array_header_t;

#define _array_default_capacity 32

void *impl_array_create(void *arr, size_t size, size_t capacity);
void impl_array_destroy(void *arr);
void *impl_array_resize(void *arr, size_t elem_size, size_t size);
void *impl_array_reserve(void *arr, size_t size);

#define _array_header(arr)            (((array_header_t*) (arr)) - 1)

#define _array_create(arr, size, cap) arr = impl_array_create(arr, size, cap)
#define _array_resize(arr, size)      arr = impl_array_resize(arr, sizeof(*arr), size)

#define array_destroy(arr)       impl_array_destroy(arr)
#define array_reserve(arr, size) arr = impl_array_reserve(arr, size)
#define array_size(arr)          _array_header(arr)->count
#define array_capacity(arr)      _array_header(arr)->capacity

#define array_push(arr, item)											\
	do {																\
		_array_create((arr), sizeof(*(arr)), _array_default_capacity);	\
		if (array_size(arr) == array_capacity(arr)) {					\
			_array_resize((arr), array_capacity(arr) * 2);				\
		}																\
		(arr)[array_size(arr)++] = (item);								\
	} while (false)
