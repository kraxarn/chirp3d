#include "ecsosapi.h"
#include "logcategory.h"

#include "flecs.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_timer.h>

// SDL doesn't support sint64 atomics, but stdlib does
// (we could also use full mutexes)
#include <stdatomic.h>

static void *malloc_(const ecs_size_t size)
{
	return SDL_malloc((size_t) size);
}

static void *realloc_(void *ptr, const ecs_size_t size)
{
	return SDL_realloc(ptr, (size_t) size);
}

static void *calloc_(const ecs_size_t size)
{
	return SDL_calloc(1, (size_t) size);
}

static char *strdup_(const char *str)
{
	if (str == nullptr)
	{
		return nullptr;
	}

	return SDL_strdup(str);
}

static void get_time(ecs_time_t *time)
{
	const Uint64 now = SDL_GetTicksNS();
	const Uint64 sec = now / SDL_NS_PER_SECOND;

	time->sec = (uint32_t) sec;
	time->nanosec = (uint32_t) (now - (sec * SDL_NS_PER_SECOND));
}

static void log_(const Sint32 level, const char *file, const Sint32 line, const char *msg)
{
	SDL_LogPriority priority;
	switch (level)
	{
		case 0:
			priority = SDL_LOG_PRIORITY_DEBUG;
			break;

		case -2:
			priority = SDL_LOG_PRIORITY_WARN;
			break;

		case -3:
			priority = SDL_LOG_PRIORITY_ERROR;
			break;

		case -4:
			priority = SDL_LOG_PRIORITY_CRITICAL;
			break;

		default:
			priority = SDL_LOG_PRIORITY_INFO;
			break;
	}

	SDL_LogMessage(LOG_CATEGORY_ECS, priority, "%s:%d: %s", file, line, msg);
}

static ecs_os_thread_t thread_new(const ecs_os_thread_callback_t callback, void *param)
{
	return (ecs_os_thread_t) SDL_CreateThread((SDL_ThreadFunction) callback, nullptr, param);
}

static void *thread_join(const ecs_os_thread_t thread)
{
	int status;
	SDL_WaitThread((SDL_Thread*) thread, &status);
	return (void*) (uintptr_t) status;
}

static ecs_os_thread_id_t thread_self()
{
	return SDL_GetCurrentThreadID();
}

static Sint32 ainc(Sint32 *count)
{
	return atomic_fetch_add((_Atomic Sint32*) count, 1) + 1;
}

static Sint32 adec(Sint32 *count)
{
	return atomic_fetch_sub((_Atomic Sint32*) count, 1) - 1;
}

static Sint64 lainc(Sint64 *count)
{
	return atomic_fetch_add((_Atomic Sint64*) count, 1) + 1;
}

static Sint64 ladec(Sint64 *count)
{
	return atomic_fetch_sub((_Atomic Sint64*) count, 1) - 1;
}

static ecs_os_mutex_t mutex_new()
{
	return (ecs_os_mutex_t) SDL_CreateMutex();
}

static void mutex_free(const ecs_os_mutex_t mutex)
{
	SDL_DestroyMutex((SDL_Mutex*) mutex);
}

static void mutex_lock(const ecs_os_mutex_t mutex)
{
	SDL_LockMutex((SDL_Mutex*) mutex);
}

static void mutex_unlock(const ecs_os_mutex_t mutex)
{
	SDL_UnlockMutex((SDL_Mutex*) mutex);
}

static ecs_os_cond_t cond_new()
{
	return (ecs_os_cond_t) SDL_CreateCondition();
}

static void cond_free(const ecs_os_cond_t cond)
{
	SDL_DestroyCondition((SDL_Condition*) cond);
}

static void cond_signal(const ecs_os_cond_t cond)
{
	SDL_SignalCondition((SDL_Condition*) cond);
}

static void cond_broadcast(const ecs_os_cond_t cond)
{
	SDL_BroadcastCondition((SDL_Condition*) cond);
}

static void cond_wait(const ecs_os_cond_t cond, const ecs_os_mutex_t mutex)
{
	SDL_WaitCondition((SDL_Condition*) cond, (SDL_Mutex*) mutex);
}

static void sleep(const Sint32 sec, const Sint32 nano_sec)
{
	SDL_DelayNS((sec * SDL_NS_PER_SECOND) + nano_sec);
}

ecs_os_api_t ecs_os_api_create()
{
	ecs_os_api_t api = {0};

	api.malloc_ = malloc_;
	api.free_ = SDL_free;
	api.realloc_ = realloc_;
	api.calloc_ = calloc_;
	api.strdup_ = strdup_;
	api.get_time_ = get_time;
	api.log_ = log_;
	api.abort_ = abort;

	api.thread_new_ = thread_new;
	api.thread_join_ = thread_join;
	api.thread_self_ = thread_self;

	api.task_new_ = thread_new;
	api.task_join_ = thread_join;

	api.ainc_ = ainc;
	api.adec_ = adec;
	api.lainc_ = lainc;
	api.ladec_ = ladec;

	api.mutex_new_ = mutex_new;
	api.mutex_free_ = mutex_free;
	api.mutex_lock_ = mutex_lock;
	api.mutex_unlock_ = mutex_unlock;

	api.cond_new_ = cond_new;
	api.cond_free_ = cond_free;
	api.cond_signal_ = cond_signal;
	api.cond_broadcast_ = cond_broadcast;
	api.cond_wait_ = cond_wait;

	api.sleep_ = sleep;
	api.now_ = SDL_GetTicksNS;

	return api;
}
