include(FetchContent)

FetchContent_Declare(tomlc17
	GIT_REPOSITORY https://github.com/kraxarn/tomlc17.git
	GIT_TAG db740476cf8efd312c53800bb153ea3b7dcc56f8
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
