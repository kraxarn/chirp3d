#include "map.h"

static void cleanup(void *userdata, void *value)
{
	((map_cleanup_callback_t) userdata)(value);
}

bool map_set_with_cleanup(const map_t map, const char *name, void *value,
	const map_cleanup_callback_t callback)
{
	return SDL_SetPointerPropertyWithCleanup(map, name, value, cleanup, callback);
}
