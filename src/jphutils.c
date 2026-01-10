#include "jphutils.h"
#include "physics.h"
#include "vector.h"

#include "joltc.h"

JPH_Vec3 *jph_vec3(vector3f_t *vec)
{
	static_assert(sizeof(vector3f_t) == sizeof(JPH_Vec3));
	static_assert(sizeof(vector3f_t) == sizeof(JPH_RVec3));

	return (JPH_Vec3 *) vec;
}

JPH_Quat *jph_quat(vector4f_t *vec)
{
	static_assert(sizeof(vector4f_t) == sizeof(JPH_Quat));

	return (JPH_Quat *) vec;
}

JPH_MotionType jph_motion_type(const physics_motion_type_t motion_type)
{
	return (JPH_MotionType) motion_type;
}

JPH_AllowedDOFs jph_allowed_dof(const physics_allowed_dof_t allowed_dof)
{
	return (JPH_AllowedDOFs) allowed_dof;
}

JPH_Activation jph_activation(const bool activate)
{
	return (int) activate
		? JPH_Activation_Activate
		: JPH_Activation_DontActivate;
}
