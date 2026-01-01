#include "audiodriver.h"
#include "logcategory.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_properties.h>

static SDL_PropertiesID props = 0;

static bool update_cache()
{
	props = SDL_CreateProperties();
	if (props == 0)
	{
		SDL_LogWarn(LOG_CATEGORY_CORE, "Cache error: %s", SDL_GetError());
		return false;
	}

	SDL_SetStringProperty(props, "pipewire", "PipeWire");
	SDL_SetStringProperty(props, "pulseaudio", "PulseAudio");
	SDL_SetStringProperty(props, "alsa", "ALSA");
	SDL_SetStringProperty(props, "netbsd", "NetBSD");
	SDL_SetStringProperty(props, "wasapi", "WASAPI");
	SDL_SetStringProperty(props, "directsound", "DirectSound");
	SDL_SetStringProperty(props, "haiku", "BSoundPlayer");
	SDL_SetStringProperty(props, "coreaudio", "CoreAudio");
	SDL_SetStringProperty(props, "openslES", "OpenSL ES");
	SDL_SetStringProperty(props, "emscripten", "Web Audio API");
	SDL_SetStringProperty(props, "jack", "JACK");
	SDL_SetStringProperty(props, "disk", "Disk");
	SDL_SetStringProperty(props, "dummy", "No audio");

	return true;
}

const char *audio_driver_display_name(const char *name)
{
	if (props == 0 && !update_cache())
	{
		return name;
	}

	return SDL_GetStringProperty(props, name, name);
}
