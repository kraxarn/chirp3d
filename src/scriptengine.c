#include "scriptengine.h"

#include "pocketpy.h"

void script_engine_create()
{
	py_initialize();
}

void script_engine_destroy()
{
	py_finalize();
}
