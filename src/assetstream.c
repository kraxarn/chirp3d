#include "assetstream.h"

#include <SDL3/SDL_iostream.h>

typedef struct
{
	SDL_IOStream *stream;
	Sint64 offset;
	Sint64 size;
	Sint64 current;
} stream_info_t;

static Sint64 stream_size(void *userdata)
{
	return ((stream_info_t*) userdata)->size;
}

static Sint64 stream_seek(void *userdata, const Sint64 offset, const SDL_IOWhence whence)
{
	const auto info = (stream_info_t*) userdata;

	switch (whence)
	{
		case SDL_IO_SEEK_SET:
			info->current = offset;
			break;

		case SDL_IO_SEEK_CUR:
			info->current += offset;
			break;

		case SDL_IO_SEEK_END:
			info->current = info->offset - offset;
			break;
	}

	return info->current;
}

static size_t stream_read(void *userdata, void *ptr, const size_t size, SDL_IOStatus *status)
{
	const auto info = (stream_info_t*) userdata;

	const size_t read_size = (info->current + size) > info->size
		? info->size - info->current
		: size;

	SDL_SeekIO(info->stream, info->offset + info->current, SDL_IO_SEEK_SET);

	if (SDL_ReadIO(info->stream, ptr, read_size) != read_size)
	{
		*status = SDL_IO_STATUS_ERROR;
		return 0;
	}

	return read_size;
}

static size_t stream_write([[maybe_unused]] void *userdata, [[maybe_unused]] const void *ptr,
	[[maybe_unused]] size_t size, SDL_IOStatus *status)
{
	*status = SDL_IO_STATUS_READONLY;
	return 0;
}

static bool stream_flush([[maybe_unused]] void *userdata, SDL_IOStatus *status)
{
	*status = SDL_IO_STATUS_READONLY;
	return false;
}

static bool stream_close(void *userdata)
{
	SDL_free(userdata);
	return true;
}

SDL_IOStream *asset_stream_open_io(SDL_IOStream *stream,
	const Sint64 offset, const Sint64 size)
{
	const SDL_IOStreamInterface interface = {
		.version = sizeof(interface),
		.size = stream_size,
		.seek = stream_seek,
		.read = stream_read,
		.write = stream_write,
		.flush = stream_flush,
		.close = stream_close,
	};

	stream_info_t *info = SDL_malloc(sizeof(stream_info_t));
	info->stream = stream;
	info->offset = offset;
	info->size = size;
	info->current = 0;

	return SDL_OpenIO(&interface, info);
}
