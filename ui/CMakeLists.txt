cmake_minimum_required(VERSION 3.22)
project(chirp3d-ui LANGUAGES C CXX)

# ImGui requires C++11 (I think?)
set(CMAKE_CXX_STANDARD 11)

add_library(${PROJECT_NAME} STATIC)
add_subdirectory(src)

target_include_directories(${PROJECT_NAME} PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

include(../deps/imgui.cmake)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
