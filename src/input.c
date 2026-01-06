#include "input.h"
#include "logcategory.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_properties.h>

static SDL_PropertiesID props = 0;

static void update_keyboard_event(const SDL_KeyboardEvent event)
{
	const char *key_name = SDL_GetKeyName(event.key);
	SDL_SetBooleanProperty(props, key_name, event.down);
}

static void cleanup_free([[maybe_unused]] void *userdata, void *value)
{
	SDL_free(value);
}

static bool init()
{
	if (props == 0)
	{
		props = SDL_CreateProperties();
	}

	if (props != 0)
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

	if (SDL_HasProperty(props, name))
	{
		return SDL_SetError("Property already exists");
	}

	// TODO: This shouldn't be set in the main 'props' as it may cause conflicts
	return SDL_SetNumberProperty(props, name, config.keycode);
}

bool input_is_key_down(const SDL_Keycode keycode)
{
	const char *key_name = SDL_GetKeyName(keycode);
	return SDL_GetBooleanProperty(props, key_name, false);
}

bool input_is_down(const char *name)
{
	const SDL_Keycode keycode = SDL_GetNumberProperty(props, name, SDLK_UNKNOWN);
	if (keycode == SDLK_UNKNOWN)
	{
		return false;
	}

	const char *key_name = SDL_GetKeyName(keycode);
	return SDL_GetBooleanProperty(props, key_name, false);
}
