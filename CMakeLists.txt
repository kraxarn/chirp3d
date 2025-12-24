cmake_minimum_required(VERSION 3.22)
project(chirp3d LANGUAGES C VERSION 0.0.1)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (APPLE)
	# Metal requires macOS 10.14 / iOS 13
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14")
endif ()

add_executable(${PROJECT_NAME})
add_subdirectory(src)

target_include_directories(${PROJECT_NAME} PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
	ENGINE_NAME="${PROJECT_NAME}"
	ENGINE_VERSION="${PROJECT_VERSION}"
)

include(deps/sdl3.cmake)
