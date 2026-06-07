#include "modules.h"
#include "vector.h"

#include "pocketpy.h"

static bool vector2_new(const int argc, py_TValue *argv)
{
	PY_CHECK_ARGC(3);

	vector2f_t *vec2 = py_newobject(py_retval(), py_totype(argv), 0, sizeof(vector2f_t));

	return (bool) ((int) py_castfloat32(py_arg(1), &vec2->x)
		&& (int) py_castfloat32(py_arg(2), &vec2->y));
}

static void add_vector2(py_TValue *mod)
{
	const py_Type type = py_newtype("Vector2",
		tp_object, mod, nullptr);

	py_bindmagic(type, py_name("__new__"), vector2_new);
}

void add_module_math()
{
	py_TValue *mod = py_getmodule("math");

	add_vector2(mod);
}
