#pragma once

#include "physics.h"
#include "vector.h"

#include "joltc.h"

[[nodiscard]]
JPH_Vec3 *jph_vec3(const vector3f_t *vec);

[[nodiscard]]
JPH_Quat *jph_quat(const vector4f_t *vec);

[[nodiscard]]
JPH_MotionType jph_motion_type(physics_motion_type_t motion_type);

[[nodiscard]]
JPH_AllowedDOFs jph_allowed_dof(physics_allowed_dof_t allowed_dof);

[[nodiscard]]
JPH_Activation jph_activation(bool activate);
