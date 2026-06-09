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

static bool vector2_repr(const int argc, py_TValue *argv)
{
	PY_CHECK_ARGC(1);

	const py_Type baseType = py_gettype("math", py_name("Vector2"));
	if (!py_issubclass(argv->type, baseType))
	{
		return py_checktype(argv, baseType);
	}

	const vector2f_t *vec = py_touserdata(argv);

	constexpr size_t buf_size = 32;
	char buf[buf_size];

	const int length = SDL_snprintf(buf, buf_size, "<Vector2(%.f, %.f)>", vec->x, vec->y);

	py_newstrv(py_retval(), (c11_sv){.data = buf, .size = length});
	return true;
}

static void add_vector2(py_TValue *mod)
{
	const py_Type type = py_newtype("Vector2",
		tp_object, mod, nullptr);

	py_bindmagic(type, py_name("__new__"), vector2_new);
	py_bindmagic(type, py_name("__repr__"), vector2_repr);
}

void add_module_math()
{
	py_TValue *mod = py_getmodule("math");

	add_vector2(mod);
}
