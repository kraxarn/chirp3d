#include "input.h"
#include "logcategory.h"
#include "map.h"
#include "mousebutton.h"
#include "ecs/components.h"

#include "flecs.h"

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

static void update_keyboard_event(const input_t *input, const SDL_KeyboardEvent event)
{
	const char *key_name = SDL_GetKeyName(event.key);

	// Events get repeatedly triggered when key is held down
	const input_state_t state = map_get(input->key_map, key_name, STATE_UP);
	if (state != STATE_UP && event.down)
	{
		return;
	}

	map_set(input->key_map, key_name, event.down ? STATE_PRESSED : STATE_UP);
}

static void update_mouse_button_event(const input_t *input, const SDL_MouseButtonEvent event)
{
	const char *button_name = mouse_button_name(event.button);
	map_set(input->button_map, button_name, event.down ? STATE_PRESSED : STATE_UP);
}

bool input_create(input_t *input)
{
	input->key_map = map_create();
	input->button_map = map_create();
	input->name_map = map_create();

	input->entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
		.name = "Input",
	});

	return (bool) (input->key_map != 0
		&& input->button_map != 0
		&& input->name_map != 0
		&& input->entity != 0);
}

void input_update(const input_t *input, const SDL_Event *event)
{
	switch (event->type)
	{
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			update_keyboard_event(input, event->key);
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			update_mouse_button_event(input, event->button);
			break;

		default:
			break;
	}
}

static void input_map_cleanup([[maybe_unused]] void *userdata, void *value)
{
	SDL_free(value);
}

bool input_add(const input_t *input, const char *name, const input_config_t config)
{
	if (map_contains(input->name_map, name))
	{
		return SDL_SetError("Property already exists");
	}

	const ecs_entity_t entity = ecs_new_w_parent(ecs_world(), input->entity, name);

	input_map_t *map = SDL_malloc(sizeof(input_map_t));

	if (!SDL_SetPointerPropertyWithCleanup(input->name_map, name, map,
		input_map_cleanup, nullptr))
	{
		return false;
	}

	if (config.keycode != SDLK_UNKNOWN)
	{
		map->type = TYPE_KEYBOARD;
		map->keycode = config.keycode;

		ecs_set_id(ecs_world(), entity, EcsKeycode,
			sizeof(SDL_Keycode), &config.keycode);
	}
	else if (config.mouse_button > 0)
	{
		map->type = TYPE_MOUSE_BUTTON;
		map->mouse_button = config.mouse_button;

		ecs_set_id(ecs_world(), entity, EcsMouseButtonFlags,
			sizeof(SDL_MouseButtonFlags), &config.mouse_button);
	}
	else
	{
		SDL_ClearProperty(input->name_map, name);
		return SDL_SetError("Unknown input mapping");
	}

	return true;
}

[[nodiscard]]
static input_state_t input_state(const input_t *input, const char *name)
{
	const input_map_t *input_map = map_get(input->name_map, name, nullptr);
	if (input_map == nullptr)
	{
		SDL_LogWarn(LOG_CATEGORY_INPUT, "Unmapped input: %s", name);
		return STATE_UP;
	}

	map_t map;
	const char *key;

	switch (input_map->type)
	{
		case TYPE_KEYBOARD:
			map = input->key_map;
			key = SDL_GetKeyName(input_map->keycode);
			break;

		case TYPE_MOUSE_BUTTON:
			map = input->button_map;
			key = mouse_button_name(input_map->mouse_button);
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

bool input_is_pressed(const input_t *input, const char *name)
{
	return input_state(input, name) == STATE_PRESSED;
}

bool input_is_down(const input_t *input, const char *name)
{
	return input_state(input, name) != STATE_UP;
}
