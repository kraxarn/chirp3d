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

target_compile_definitions(imgui PUBLIC
	IMGUI_DISABLE_DEFAULT_FONT
	IMGUI_STB_TRUETYPE_FILENAME="${stb_SOURCE_DIR}/stb_truetype.h"
)

if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	target_compile_definitions(imgui PUBLIC
		IMGUI_DISABLE_DEMO_WINDOWS
		IMGUI_DISABLE_DEBUG_TOOLS
	)
endif ()

# For SDL3 backend
target_link_libraries(imgui PRIVATE SDL3::SDL3)

# For testing
option(BUILD_IMGUI_DEMO "Build ImGui demo app" OFF)
if (BUILD_IMGUI_DEMO)
	add_executable(imgui-sdl3gpu-example
		"${imgui_SOURCE_DIR}/examples/example_sdl3_sdlgpu3/main.cpp"
	)
	target_include_directories(imgui-sdl3gpu-example PRIVATE
		"${imgui_SOURCE_DIR}"
		"${imgui_SOURCE_DIR}/backends"
	)
	target_link_libraries(imgui-sdl3gpu-example PRIVATE SDL3::SDL3 imgui)
endif ()
