include(FetchContent)

FetchContent_Declare(sdl
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG prerelease-3.3.6
)

find_package(SDL3 3.4 QUIET)

if (SDL3_FOUND)
	message(STATUS "Using system sdl3")
else ()
	message(STATUS "Downloading sdl3")
	set(SDL_AUDIO ON)
	set(SDL_CAMERA OFF)
	set(SDL_DIALOG ON)
	set(SDL_GPU ON)
	set(SDL_HAPTIC OFF)
	set(SDL_HIDAPI OFF)
	set(SDL_JOYSTICK ON)
	set(SDL_POWER OFF)
	set(SDL_RENDER ON)
	set(SDL_SENSOR OFF)
	set(SDL_VIDEO ON)
	FetchContent_MakeAvailable(sdl)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
