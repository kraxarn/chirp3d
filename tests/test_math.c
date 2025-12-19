#include "math.h"

#include <assert.h>

static void test_clamp()
{
	assert(clamp(5, 1, 10) == 5);
	assert(clamp(5, 10, 15) == 10);
	assert(clamp(10, 1, 5) == 5);
}

int main()
{
	test_clamp();

	return 0;
}
