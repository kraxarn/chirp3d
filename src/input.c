#include "input.h"
#include "logcategory.h"
#include "map.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

typedef enum key_state_t : Sint64
{
	STATE_UP,
	STATE_PRESSED,
	STATE_DOWN,
} key_state_t;

// key name -> key state
static map_t key_map = 0;

// input name -> keycode
static map_t input_map = 0;

static void update_keyboard_event(const SDL_KeyboardEvent event)
{
	const char *key_name = SDL_GetKeyName(event.key);

	// Events get repeatedly triggered when key is held down
	const key_state_t state = map_get(key_map, key_name, STATE_UP);
	if (state != STATE_UP && event.down)
	{
		return;
	}

	map_set(key_map, key_name, event.down ? STATE_PRESSED : STATE_UP);
}

static bool init()
{
	if (key_map == 0)
	{
		key_map = map_create();
	}

	if (input_map == 0)
	{
		input_map = map_create();
	}

	if (key_map != 0 && input_map != 0)
	{
		return true;
	}

	SDL_LogError(LOG_CATEGORY_INPUT, "Property error: %s", SDL_GetError());
	return false;
}

void input_update(const SDL_Event *event)
{
	if (!init())
	{
		return;
	}

	if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
	{
		update_keyboard_event(event->key);
	}
}

bool input_add(const char *name, const input_config_t config)
{
	if (!init())
	{
		return false;
	}

	if (map_contains(input_map, name))
	{
		return SDL_SetError("Property already exists");
	}

	return map_set(input_map, name, config.keycode);
}

[[nodiscard]]
static key_state_t input_state(const char *name)
{
	const SDL_Keycode keycode = map_get(input_map, name, SDLK_UNKNOWN);
	if (keycode == SDLK_UNKNOWN)
	{
		SDL_LogWarn(LOG_CATEGORY_INPUT, "Unmapped input: %s", name);
		return STATE_UP;
	}

	const char *key_name = SDL_GetKeyName(keycode);
	const key_state_t state = map_get(key_map, key_name, STATE_UP);

	if (state == STATE_PRESSED)
	{
		map_set(key_map, key_name, STATE_DOWN);
	}

	return state;
}

bool input_is_pressed(const char *name)
{
	return input_state(name) == STATE_PRESSED;
}

bool input_is_down(const char *name)
{
	return input_state(name) != STATE_UP;
}
