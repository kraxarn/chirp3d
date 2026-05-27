#pragma once

#include <SDL3/SDL_iostream.h>

[[nodiscard]]
SDL_IOStream *asset_stream_open_io(SDL_IOStream *stream, Sint64 offset, Sint64 size);
