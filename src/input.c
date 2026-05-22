#include "input.h"
#include "logcategory.h"
#include "map.h"
#include "mousebutton.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

typedef enum : Sint64
{
	STATE_UP,
	STATE_PRESSED,
	STATE_DOWN,
} input_state_t;

typedef enum : Uint8
{
	TYPE_UNKNOWN,
	TYPE_KEYBOARD,
	TYPE_MOUSE_BUTTON,
} input_type_t;

typedef struct
{
	input_type_t type;

	union
	{
		SDL_Keycode keycode;
		SDL_MouseButtonFlags mouse_button;
	};
} input_map_t;

// key name -> input state
static map_t key_map = 0;

// mouse button name -> input state
static map_t button_map = 0;

// input name -> input map
static map_t input_map = 0;

static void update_keyboard_event(const SDL_KeyboardEvent event)
{
	const char *key_name = SDL_GetKeyName(event.key);

	// Events get repeatedly triggered when key is held down
	const input_state_t state = map_get(key_map, key_name, STATE_UP);
	if (state != STATE_UP && event.down)
	{
		return;
	}

	map_set(key_map, key_name, event.down ? STATE_PRESSED : STATE_UP);
}

static void update_mouse_button_event(const SDL_MouseButtonEvent event)
{
	const char *button_name = mouse_button_name(event.button);
	map_set(button_map, button_name, event.down ? STATE_PRESSED : STATE_UP);
}

static bool init()
{
	if (key_map == 0)
	{
		key_map = map_create();
	}

	if (button_map == 0)
	{
		button_map = map_create();
	}

	if (input_map == 0)
	{
		input_map = map_create();
	}

	if (key_map != 0 && button_map != 0 && input_map != 0)
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

	switch (event->type)
	{
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			update_keyboard_event(event->key);
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			update_mouse_button_event(event->button);
			break;

		default:
			break;
	}
}

static void input_map_cleanup([[maybe_unused]] void *userdata, void *value)
{
	SDL_free(value);
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

	input_map_t *map = SDL_malloc(sizeof(input_map_t));

	if (!SDL_SetPointerPropertyWithCleanup(input_map, name, map,
		input_map_cleanup, nullptr))
	{
		SDL_free(map);
		return false;
	}

	if (config.keycode != SDLK_UNKNOWN)
	{
		map->type = TYPE_KEYBOARD;
		map->keycode = config.keycode;
	}
	else if (config.mouse_button > 0)
	{
		map->type = TYPE_MOUSE_BUTTON;
		map->mouse_button = config.mouse_button;
	}
	else
	{
		SDL_ClearProperty(input_map, name);
		return SDL_SetError("Unknown input mapping");
	}

	return true;
}

[[nodiscard]]
static input_state_t input_state(const char *name)
{
	const input_map_t *input = map_get(input_map, name, nullptr);
	if (input == nullptr)
	{
		SDL_LogWarn(LOG_CATEGORY_INPUT, "Unmapped input: %s", name);
		return STATE_UP;
	}

	map_t map;
	const char *key;

	switch (input->type)
	{
		case TYPE_KEYBOARD:
			map = key_map;
			key = SDL_GetKeyName(input->keycode);
			break;

		case TYPE_MOUSE_BUTTON:
			map = button_map;
			key = mouse_button_name(input->mouse_button);
			break;

		default:
			return STATE_UP;
	}

	const input_state_t state = map_get(map, key, STATE_UP);
	if (state == STATE_PRESSED)
	{
		map_set(map, key, STATE_DOWN);
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
