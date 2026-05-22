#pragma once

#include <SDL3/SDL_stdinc.h>

typedef struct array_header_t
{
	size_t count;
	size_t capacity;
} array_header_t;

void impl_array_destroy(void *arr);
void *impl_array_reserve(void *arr, size_t capacity, size_t item_size);
void *impl_array_push(void *arr, size_t item_size);

#define _array_header(arr) (((array_header_t*) (arr)) - 1)

#define array_destroy(arr)       impl_array_destroy((void *) arr)
#define array_reserve(arr, size) arr = (typeof(arr)) impl_array_reserve((void *) (arr), size, sizeof(*arr))
#define array_size(arr)          _array_header(arr)->count
#define array_capacity(arr)      _array_header(arr)->capacity
#define array_at(arr, index)     (index >= 0 && index < array_size(arr) ? arr[index] : (typeof(*arr)) 0)

#define array_push(arr, item)												\
	do {																	\
		(arr) = (typeof(arr)) impl_array_push((void *) arr, sizeof(*arr));	\
		(arr)[array_size(arr)++] = (item);									\
	} while (false)
