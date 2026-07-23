include(FetchContent)

FetchContent_Declare(box3d
	GIT_REPOSITORY https://github.com/erincatto/box3d.git
	GIT_TAG v0.1.0
)

set(BOX3D_SANITIZE OFF)
set(BOX3D_DISABLE_SIMD OFF)
set(BOX3D_COMPILE_WARNING_AS_ERROR OFF)
set(BOX3D_DOUBLE_PRECISION OFF)

message(STATUS "Downloading box3d")
FetchContent_MakeAvailable(box3d)

target_link_libraries(${PROJECT_NAME} PRIVATE
	box3d::box3d
)
