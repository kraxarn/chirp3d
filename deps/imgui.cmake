include(FetchContent)

FetchContent_Declare(imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG v1.92.5
)

message(STATUS "Downloading imgui")
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC)

target_sources(imgui PRIVATE
	# ImGui core
	"${imgui_SOURCE_DIR}/imgui.cpp"
	"${imgui_SOURCE_DIR}/imgui_draw.cpp"
	"${imgui_SOURCE_DIR}/imgui_tables.cpp"
	"${imgui_SOURCE_DIR}/imgui_widgets.cpp"
	"${imgui_SOURCE_DIR}/imgui_demo.cpp"
	# ImGui SDL3 GPU backend
	"${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
	"${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp"
)

target_include_directories(imgui PUBLIC
	"${imgui_SOURCE_DIR}"
)

file(WRITE "${imgui_SOURCE_DIR}/imconfig.h"
	"#pragma once\n"
	"#define IMGUI_DISABLE_DEFAULT_FONT\n"
	"#define IMGUI_STB_TRUETYPE_FILENAME \"${stb_SOURCE_DIR}/stb_truetype.h\"\n"
	"#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS\n"
)

if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	file(APPEND "${imgui_SOURCE_DIR}/imconfig.h"
		"#define IMGUI_DISABLE_DEMO_WINDOWS\n"
	)
endif ()

# For SDL3 backend
target_link_libraries(imgui PRIVATE SDL3::SDL3)
