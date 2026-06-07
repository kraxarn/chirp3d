#include "scriptengine.h"
#include "logcategory.h"
#include "modules.h"

#include "pocketpy.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>

static char *importfile(const char *path, int *data_size) // TODO
{
	SDL_LogWarn(LOG_CATEGORY_SCRIPT, "Failed to import '%s': Not supported", path);
	*data_size = 0;
	return nullptr;
}

static py_GlobalRef lazyimport(const char *path) // TODO
{
	SDL_LogWarn(LOG_CATEGORY_SCRIPT, "Failed to lazily import '%s': Not supported", path);
	return nullptr;
}

static void print(const char *message)
{
	const size_t length = SDL_strlen(message);
	if (length == 1 && message[0] == '\n')
	{
		return;
	}


	// Strip final '\n' as SDL already inserts it automatically
	const int s_length = (int) (message[length - 1] == '\n' ? length - 1 : length);

	const SDL_LogPriority priority = (int) py_checkexc()
		? SDL_LOG_PRIORITY_ERROR
		: SDL_LOG_PRIORITY_INFO;

	SDL_LogMessage(LOG_CATEGORY_SCRIPT, priority, "%.*s", s_length, message);
}

static void flush()
{
	// Flushing isn't needed
}

static int get_chr() // TODO
{
	SDL_LogWarn(LOG_CATEGORY_SCRIPT, "Input is currently not supported");
	return 0;
}

static void gc_mark(void (*func)(py_Ref val, void *ctx), void *ctx)
{
	// We probably want to do stuff here later
}

void script_engine_create()
{
	py_initialize();

	py_Callbacks *callbacks = py_callbacks();
	callbacks->importfile = importfile;
	callbacks->lazyimport = lazyimport;
	callbacks->print = print;
	callbacks->flush = flush;
	callbacks->getchr = get_chr;
	callbacks->gc_mark = gc_mark;

	add_module_ecs();
	add_module_math();

	char *path = nullptr;
	SDL_asprintf(&path, "%s../example/assets/scripts/main.py", SDL_GetBasePath());

	size_t data_size = 0;
	void *data = SDL_LoadFile(path, &data_size);
	SDL_free(path);

	if (data == nullptr)
	{
		SDL_LogError(LOG_CATEGORY_SCRIPT, "%s", SDL_GetError());
		return;
	}

	if (!py_exec(data, "main", EXEC_MODE, nullptr))
	{
		py_printexc();
	}

	SDL_free(data);
}

void script_engine_destroy()
{
	py_finalize();
}
