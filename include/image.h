#pragma once

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_surface.h>

SDL_Surface *load_qoi(SDL_IOStream *source);
