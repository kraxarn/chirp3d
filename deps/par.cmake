include(FetchContent)

FetchContent_Declare(par
	GIT_REPOSITORY https://github.com/prideout/par.git
	GIT_TAG b3571fdf83518d921af3b69d13ea0cb8749147aa
)

message(STATUS "Downloading par")
FetchContent_MakeAvailable(par)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${par_SOURCE_DIR}"
)
