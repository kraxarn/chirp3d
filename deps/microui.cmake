include(FetchContent)

FetchContent_Declare(microui
	GIT_REPOSITORY https://github.com/microui-community/microui.git
	GIT_TAG a8ac577f5ed2a1568fd4f2af0864816928700e89
)

message(STATUS "Downloading microui")
FetchContent_MakeAvailable(microui)

add_library(microui STATIC
	"${microui_SOURCE_DIR}/src/microui.c"
)

target_include_directories(microui PUBLIC
	"${microui_SOURCE_DIR}/src"
)

option(BUILD_MU_DEMO "Build microui demo app" OFF)
if (BUILD_MU_DEMO)
	add_executable(microui-example
		"${CMAKE_CURRENT_SOURCE_DIR}/src/ui/demo.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/ui/renderer.c"
	)
	target_include_directories(microui-example PRIVATE
		"${microui_SOURCE_DIR}/demo"
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	)
	target_link_libraries(microui-example PRIVATE microui)
	target_link_libraries(microui-example PRIVATE SDL3::SDL3)
	find_package(OpenGL REQUIRED)
	target_link_libraries(microui-example PRIVATE OpenGL::GL)
endif ()
