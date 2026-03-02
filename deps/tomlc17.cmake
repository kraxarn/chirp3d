include(FetchContent)

FetchContent_Declare(tomlc17
	GIT_REPOSITORY https://github.com/kraxarn/tomlc17.git
	GIT_TAG f68f9f26f648387f2675df67c19fb6820a9f0cc1
)

message(STATUS "Downloading tomlc17")
FetchContent_MakeAvailable(tomlc17)

add_library(tomlc17 STATIC)

target_sources(tomlc17 PRIVATE
	"${tomlc17_SOURCE_DIR}/src/tomlc17.c"
)

target_include_directories(tomlc17 PUBLIC
	"${tomlc17_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME} PRIVATE tomlc17)
