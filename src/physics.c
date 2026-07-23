#include "physics.h"
#include "logcategory.h"

#include "box3d/base.h"
#include "box3d/box3d.h"
#include "box3d/id.h"
#include "box3d/types.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_thread.h>

typedef struct
{
	b3TaskCallback *callback;
	void *context;
} task_info_t;

static void b3_log(const char *message)
{
	SDL_LogInfo(LOG_CATEGORY_PHYSICS, "%s", message);
}

static int b3_assert(const char *condition,
	const char *filename, const int line_number)
{
	SDL_AssertData assert_data = {
		.always_ignore = false,
		.trigger_count = 0,
		.condition = condition,
		.filename = filename,
		.linenum = line_number,
		.function = nullptr,
		.next = nullptr,
	};

	const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&assert_data,
		assert_data.function, assert_data.filename, assert_data.linenum);

	if (sdl_assert_state == SDL_ASSERTION_RETRY)
	{
		return 0;
	}

	if (sdl_assert_state == SDL_ASSERTION_BREAK)
	{
		SDL_AssertBreakpoint();
	}

	return 1;
}

static int task_fn(void *data)
{
	const task_info_t *info = data;
	info->callback(info->context);
	SDL_free(data);
	return 0;
}

static void *enqueue_task(b3TaskCallback *task, void *task_context,
	[[maybe_unused]] void *user_context, const char *task_name)
{
	// SDL uses a different signature than Box3D expects :/
	task_info_t *info = SDL_malloc(sizeof(task_info_t));
	info->callback = task;
	info->context = task_context;

	SDL_Thread *thread = SDL_CreateThread(task_fn, task_name, info);
	if (thread == nullptr)
	{
		// TODO: Fatal error?
		SDL_LogError(LOG_CATEGORY_PHYSICS,
			"Failed to create thread: %s", SDL_GetError());
	}

	return thread;
}

static void finish_task(void *task, [[maybe_unused]] void *context)
{
	SDL_Thread *thread = task;

	int status = -1;
	SDL_WaitThread(thread, &status);
	if (status != 0)
	{
		// TODO: Fatal error?
		SDL_LogError(LOG_CATEGORY_PHYSICS,
			"Failed to wait for thread to finish: %d", status);
	}
}

void physics_ctor(void *ptr, const Sint32 count,
	[[maybe_unused]] const ecs_type_info_t *type_info)
{
	SDL_assert(count == 1);
	SDL_assert(b3GetWorldCount() == 0);

	b3SetLogFcn(b3_log);
	b3SetAssertFcn(b3_assert);

	b3WorldDef world_def = b3DefaultWorldDef();

	// TODO: Do we want to respect --threads here?
	world_def.workerCount = SDL_GetNumLogicalCPUCores();
	world_def.enqueueTask = enqueue_task;
	world_def.finishTask = finish_task;

	b3WorldId *world = ptr;
	*world = b3CreateWorld(&world_def);
	SDL_assert(world->index1 != 0);

	// Should be the same as world_def.workerCount
	SDL_LogInfo(LOG_CATEGORY_PHYSICS, "Using %d workers",
		b3World_GetWorkerCount(*world));
}

void physics_dtor(void *ptr, const Sint32 count,
	[[maybe_unused]] const ecs_type_info_t *type_info)
{
	SDL_assert(count == 1);
	b3DestroyWorld(*(b3WorldId*) ptr);
}
