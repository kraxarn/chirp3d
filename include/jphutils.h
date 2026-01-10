#pragma once

#include "physics.h"
#include "vector.h"

typedef struct JPH_Vec3 JPH_Vec3;
typedef struct JPH_Quat JPH_Quat;

typedef enum JPH_MotionType JPH_MotionType;
typedef enum JPH_AllowedDOFs JPH_AllowedDOFs;
typedef enum JPH_Activation JPH_Activation;

#define jph_vec3(x) _Generic((x),     \
	vector3f_t*:       jph_mut_vec3,  \
	const vector3f_t*: jph_const_vec3 \
)(x)

[[nodiscard]]
JPH_Vec3 *jph_mut_vec3(vector3f_t *vec);

[[nodiscard]]
const JPH_Vec3 *jph_const_vec3(const vector3f_t *vec);

[[nodiscard]]
JPH_Quat *jph_quat(vector4f_t *vec);

[[nodiscard]]
JPH_MotionType jph_motion_type(physics_motion_type_t motion_type);

[[nodiscard]]
JPH_AllowedDOFs jph_allowed_dof(physics_allowed_dof_t allowed_dof);

[[nodiscard]]
JPH_Activation jph_activation(bool activate);
