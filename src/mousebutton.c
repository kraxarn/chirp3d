#include "mousebutton.h"

#define button_left   "Left"
#define button_middle "Middle"
#define button_right  "Right"
#define button_side1  "Side1"
#define button_side2  "Side2"

const char *mouse_button_name(const SDL_MouseButtonFlags flags)
{
	switch (flags)
	{
		case SDL_BUTTON_LEFT:
			return button_left;

		case SDL_BUTTON_MIDDLE:
			return button_middle;

		case SDL_BUTTON_RIGHT:
			return button_right;

		case SDL_BUTTON_X1:
			return button_side1;

		case SDL_BUTTON_X2:
			return button_side2;

		default:
			return "";
	}
}

SDL_MouseButtonFlags mouse_button_from_name(const char *name)
{
	if (SDL_strcmp(name, button_left) == 0)
	{
		return SDL_BUTTON_LEFT;
	}

	if (SDL_strcmp(name, button_middle) == 0)
	{
		return SDL_BUTTON_MIDDLE;
	}

	if (SDL_strcmp(name, button_right) == 0)
	{
		return SDL_BUTTON_RIGHT;
	}

	if (SDL_strcmp(name, button_side1) == 0)
	{
		return SDL_BUTTON_X1;
	}

	if (SDL_strcmp(name, button_side2) == 0)
	{
		return SDL_BUTTON_X2;
	}

	return 0;
}
