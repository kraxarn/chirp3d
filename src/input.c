#include "input.h"

#include "logcategory.h"
#include "mousebutton.h"
#include "ecs/components.h"
#include "ecs/entities.h"

#include "flecs.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

static constexpr Uint16 crc_mouse_button = 0xE05E;
static constexpr Uint16 crc_keycode = 0x4ECD;
static constexpr Uint16 crc_input_name = 0x1407;

#define make_alive(f, v, n, p)											\
	do {																\
		const bool was_alive = ecs_is_alive(ecs_world(), entity);		\
		ecs_make_alive(ecs_world(), entity);							\
		if (!was_alive) {												\
			const char *name = f(v);									\
			SDL_LogDebug(LOG_CATEGORY_INPUT, "New "n": %s (%u / %u)",	\
				name, v, crc);											\
			ecs_add_pair(ecs_world(), entity, EcsChildOf, p);			\
			ecs_set_name(ecs_world(), entity, name);					\
		}																\
	} while (false)

static ecs_entity_t set_keycode_state(const SDL_Keycode keycode, const input_state_t input_state)
{
	const Uint16 crc = SDL_crc16(crc_keycode, &keycode, sizeof(SDL_Keycode) / sizeof(Uint8));
	const ecs_entity_t entity = ecs_offset_input + crc;
	make_alive(SDL_GetKeyName, keycode, "keycode", EcsKeycodeStates);

	ecs_set_id(ecs_world(), entity, EcsInputState,
		sizeof(input_state_t), &input_state);

	return entity;
}

static ecs_entity_t set_mouse_button_state(const Uint8 button, const input_state_t input_state)
{
	const Uint16 crc = SDL_crc16(crc_mouse_button, &button, 1);
	const ecs_entity_t entity = ecs_offset_input + crc;
	make_alive(mouse_button_name, button, "mouse button", EcsMouseButtonStates);

	ecs_set_id(ecs_world(), entity, EcsInputState,
		sizeof(input_state_t), &input_state);

	return entity;
}

static void update_keyboard_event(const SDL_KeyboardEvent event)
{
	input_state_t input_state;
	if (event.repeat) // TODO: I think we can just do this instead of getting the current value
	{
		input_state = STATE_DOWN;
	}
	else if (event.down)
	{
		input_state = STATE_PRESSED;
	}
	else
	{
		input_state = STATE_UP;
	}

	set_keycode_state(event.key, input_state);
}

static void update_mouse_button_event(const SDL_MouseButtonEvent event)
{
	const input_state_t input_state = (int) event.down ? STATE_DOWN : STATE_UP;
	set_mouse_button_state(event.button, input_state);
}

void input_update(const SDL_Event *event)
{
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

static ecs_entity_t input_entity(const char *name)
{
	const Uint16 crc = SDL_crc16(crc_input_name, name, SDL_strlen(name));
	return ecs_offset_input + crc;
}

bool input_add(const char *name, const input_config_t config)
{
	const ecs_entity_t entity = input_entity(name);
	ecs_make_alive(ecs_world(), entity);
	ecs_add_pair(ecs_world(), entity, EcsChildOf, EcsInput);
	ecs_set_name(ecs_world(), entity, name);

	if (config.keycode != SDLK_UNKNOWN)
	{
		const ecs_entity_t target = set_keycode_state(config.keycode, STATE_UP);
		ecs_add_pair(ecs_world(), entity, EcsMapsTo, target);
	}

	if (config.mouse_button > 0)
	{
		const ecs_entity_t target = set_mouse_button_state(config.mouse_button, STATE_UP);
		ecs_add_pair(ecs_world(), entity, EcsMapsTo, target);
	}

	// TODO: Maybe check if any component was actually set and emit a warning?

	return true;
}

[[nodiscard]]
static input_state_t input_state(const char *name)
{
	const ecs_entity_t entity = input_entity(name);
	if (!ecs_is_alive(ecs_world(), entity))
	{
		SDL_LogWarn(LOG_CATEGORY_INPUT, "Invalid input: %s", name);
		return STATE_UP;
	}

	// TODO: We could have it mapped to multiple keys, don't assume index 0 only
	const ecs_entity_t target = ecs_get_target(ecs_world(), entity, EcsMapsTo, 0);
	if (target == 0)
	{
		SDL_LogWarn(LOG_CATEGORY_INPUT, "Unmapped input: %s", name);
		return STATE_UP;
	}

	const input_state_t *input_state = ecs_get_id(ecs_world(),
		target, EcsInputState);

	SDL_assert(input_state != nullptr);
	return *input_state;
}

bool input_is_pressed(const char *name)
{
	return input_state(name) == STATE_PRESSED;
}

bool input_is_down(const char *name)
{
	return input_state(name) != STATE_UP;
}
