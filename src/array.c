#include "array.h"

#include <SDL3/SDL_stdinc.h>

static constexpr size_t default_capacity = 32;

static void *array_create(void *arr, const size_t capacity, const size_t item_size)
{
	if (arr != nullptr)
	{
		return arr;
	}

	const size_t items_size = item_size * capacity;
	array_header_t *header = SDL_malloc(sizeof(array_header_t) + items_size);
	SDL_memset(header + 1, 0, items_size);

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

static void *array_resize(void *arr, const size_t new_capacity, const size_t item_size)
{
	array_capacity(arr) = new_capacity;
	const size_t new_size = sizeof(array_header_t) + (item_size * array_capacity(arr));
	array_header_t *header = SDL_realloc(_array_header(arr), new_size);
	return header + 1;
}

void *impl_array_reserve(void *arr, const size_t capacity, const size_t item_size)
{
	if (capacity == 0)
	{
		return nullptr;
	}

	if (arr == nullptr)
	{
		return array_create(arr, capacity, item_size);
	}

	if (array_capacity(arr) < capacity)
	{
		return array_resize(arr, capacity, item_size);
	}

	return arr;
}

void *impl_array_push(void *arr, const size_t item_size)
{
	arr = array_create(arr, default_capacity, item_size);

	if (array_size(arr) == array_capacity(arr))
	{
		arr = array_resize(arr, array_capacity(arr) * 2, item_size);
	}

	return arr;
}
