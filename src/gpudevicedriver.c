#include "gpudevicedriver.h"
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

	SDL_SetStringProperty(props, "vulkan", "Vulkan");
	SDL_SetStringProperty(props, "direct3d12", "Direct3D 12");
	SDL_SetStringProperty(props, "metal", "Metal");

	return true;
}

const char *gpu_device_driver_display_name(const char *name)
{
	if (props == 0 && !update_cache())
	{
		return name;
	}

	return SDL_GetStringProperty(props, name, name);
}
