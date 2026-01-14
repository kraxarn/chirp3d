include(FetchContent)

FetchContent_Declare(dcimgui
	GIT_REPOSITORY https://github.com/dearimgui/dear_bindings.git
	GIT_TAG DearBindings_v0.17_ImGui_v1.92.5
)

message(STATUS "Downloading dcimgui")
FetchContent_MakeAvailable(dcimgui)

add_library(dcimgui STATIC)

find_package(Python 3.10 COMPONENTS Interpreter REQUIRED)

set(VENV "${dcimgui_SOURCE_DIR}/venv")

if (WIN32)
	set(VENV_PIP "${VENV}/scripts/pip.exe")
	set(VENV_PYTHON "${VENV}/scripts/python.exe")
else ()
	set(VENV_PIP "${VENV}/bin/pip")
	set(VENV_PYTHON "${VENV}/bin/python")
endif ()

add_custom_command(OUTPUT
	"${dcimgui_SOURCE_DIR}/generated/dcimgui.cpp"
	"${dcimgui_SOURCE_DIR}/generated/dcimgui.h"
	"${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdl3.cpp"
	"${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdl3.h"
	"${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdlgpu3.cpp"
	"${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdlgpu3.h"

	COMMAND "${Python_EXECUTABLE}" -m venv "${VENV}"
	COMMAND "${VENV_PIP}" install -r "${dcimgui_SOURCE_DIR}/requirements.txt"

	COMMAND "${VENV_PYTHON}" "${dcimgui_SOURCE_DIR}/dear_bindings.py"
	--output "${dcimgui_SOURCE_DIR}/generated/dcimgui"
	"${imgui_SOURCE_DIR}/imgui.h"

	COMMAND "${VENV_PYTHON}" "${dcimgui_SOURCE_DIR}/dear_bindings.py"
	--backend
	--include "${imgui_SOURCE_DIR}/imgui.h"
	--imconfig-path "${imgui_SOURCE_DIR}/imconfig.h"
	--output "${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdl3"
	"${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h"

	COMMAND "${VENV_PYTHON}" "${dcimgui_SOURCE_DIR}/dear_bindings.py"
	--backend
	--include "${imgui_SOURCE_DIR}/imgui.h"
	--imconfig-path "${imgui_SOURCE_DIR}/imconfig.h"
	--output "${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdlgpu3"
	"${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.h"

	VERBATIM
)

file(REMOVE_RECURSE "${VENV}")

target_link_libraries(dcimgui PRIVATE imgui SDL3::SDL3)

target_include_directories(dcimgui PRIVATE
	"${imgui_SOURCE_DIR}/backends"
)

target_sources(dcimgui PRIVATE
	"${dcimgui_SOURCE_DIR}/generated/dcimgui.cpp"
	"${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdl3.cpp"
	"${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdlgpu3.cpp"
)

target_include_directories(dcimgui PUBLIC
	"${dcimgui_SOURCE_DIR}/generated"
)

# Workaround for some weird CI behaviour
file(WRITE
	"${dcimgui_SOURCE_DIR}/generated/backends/dcimgui_impl_sdl3.h"
	""
)

file(COPY_FILE
	"${imgui_SOURCE_DIR}/imconfig.h"
	"${dcimgui_SOURCE_DIR}/generated/imconfig.h"
	ONLY_IF_DIFFERENT
)

target_link_libraries(${PROJECT_NAME} PRIVATE dcimgui)
