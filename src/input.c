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

void input_update(const SDL_Event *event)
{
	if (props == 0)
	{
		props = SDL_CreateProperties();
		if (props == 0)
		{
			SDL_LogError(LOG_CATEGORY_INPUT, "Property error: %s", SDL_GetError());
			return;
		}
	}

	if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
	{
		update_keyboard_event(event->key);
	}
}

bool input_is_key_down(SDL_Keycode keycode)
{
	const char *key_name = SDL_GetKeyName(keycode);
	return SDL_GetBooleanProperty(props, key_name, false);
}
