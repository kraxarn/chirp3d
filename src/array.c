#include "array.h"

#include <SDL3/SDL_stdinc.h>

void *impl_array_create(void *arr, const size_t size, const size_t capacity)
{
	if (arr != nullptr)
	{
		return arr;
	}

	const size_t init_size = sizeof(array_header_t) + (size * capacity);
	array_header_t *header = SDL_malloc(init_size);
	header->count = 0;
	header->capacity = capacity;
	return header + 1;
}

void impl_array_destroy(void *arr)
{
	if (arr == nullptr)
	{
		return;
	}

	SDL_free(_array_header(arr));
}

void *impl_array_resize(void *arr, const size_t elem_size, const size_t size)
{
	array_capacity(arr) = size;
	const size_t new_size = sizeof(array_header_t) + (elem_size * array_capacity(arr));
	array_header_t *header = SDL_realloc(_array_header(arr), new_size);
	return header + 1;
}

void *impl_array_reserve(void *arr, const size_t size)
{
	if (arr == nullptr)
	{
		return _array_create((arr), sizeof(*(arr)), (size));
	}

	if (array_capacity(arr) < size)
	{
		return _array_resize(arr, size);
	}

	return arr;
}
