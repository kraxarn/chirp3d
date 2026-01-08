cmake_minimum_required(VERSION 3.22)
project(chirp3d LANGUAGES C VERSION 0.1.0)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (APPLE)
	# Metal requires macOS 10.14 / iOS 13
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14")
endif ()

add_executable(${PROJECT_NAME})
add_subdirectory(src)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	ENGINE_NAME="${PROJECT_NAME}"
	ENGINE_VERSION="${PROJECT_VERSION}"
	ENGINE_IDENTIFIER="com.kraxarn.${PROJECT_NAME}"
)

include(resources.cmake)

include(deps/sdl3.cmake)
include(deps/qoi.cmake)
include(deps/stb.cmake)
include(deps/tomlc17.cmake)
include(deps/joltc.cmake)

set_target_properties(${PROJECT_NAME} PROPERTIES
	# macOS
	MACOSX_BUNDLE TRUE
	MACOSX_BUNDLE_GUI_IDENTIFIER "${ENGINE_IDENTIFIER}"
	MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
	MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
	# Windows
	WIN32_EXECUTABLE TRUE
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	include(CheckIPOSupported)
	check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
	if (ipo_supported)
		set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
		message(STATUS "LTO enabled")
	else ()
		message(WARNING "LTO error: ${ipo_error}")
	endif ()
endif ()

if (TARGET SDL3-shared)
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${PROJECT_NAME}>
		VERBATIM
	)
endif ()
