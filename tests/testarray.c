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

void test_array()
{
	test_array_push();
}
