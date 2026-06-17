include(FetchContent)

FetchContent_Declare(flecs
	GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
	GIT_TAG v4.1.5
)

message(STATUS "Downloading flecs")

set(FLECS_STATIC ON)
set(FLECS_SHARED OFF)
set(FLECS_PIC OFF)
set(FLECS_TESTS OFF)

FetchContent_MakeAvailable(flecs)

target_compile_definitions(flecs_static PUBLIC
	FLECS_CUSTOM_BUILD
	FLECS_NO_OS_API_IMPL
	FLECS_MODULE
	FLECS_SYSTEM
	FLECS_PIPELINE
	FLECS_TIMER
	FLECS_QUERY_DSL
	FLECS_LOG
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(flecs_static PUBLIC
		FLECS_REST
		FLECS_STATS
		FLECS_METRICS
	)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE flecs_static)
