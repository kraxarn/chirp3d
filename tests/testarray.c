#include "tests.h"

#include "array.h"

#include <assert.h>

static void test_array_push()
{
	int *items = nullptr;

	array_push(items, 1);
	array_push(items, 2);
	array_push(items, 3);
	array_push(items, 4);
	array_push(items, 5);

	assert(items[0] == 1);
	assert(items[1] == 2);
	assert(items[2] == 3);
	assert(items[3] == 4);
	assert(items[4] == 5);

	array_destroy(items);
}

static void test_array_reserve()
{
	int *items = nullptr;
	array_reserve(items, 1);

	assert(array_capacity(items) == 1);

	array_push(items, 1);
	array_push(items, 2);
	array_push(items, 3);

	assert(array_capacity(items) > 1);
}

static void test_array_create_zero()
{
	int *items = nullptr;
	array_reserve(items, 3);

	assert(items[0] == 0);
	assert(items[1] == 0);
	assert(items[2] == 0);
}

static void test_array_resize_zero()
{
	int *items = nullptr;
	array_reserve(items, 1);
	array_reserve(items, 3);

	assert(items[0] == 0);
	assert(items[1] == 0);
	assert(items[2] == 0);
}

void test_array()
{
	test_array_push();
	test_array_reserve();
	test_array_create_zero();
	test_array_resize_zero();
}
