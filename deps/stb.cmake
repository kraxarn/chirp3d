include(FetchContent)

FetchContent_Declare(stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG f1c79c02822848a9bed4315b12c8c8f3761e1296
)

message(STATUS "Downloading stb")
FetchContent_MakeAvailable(stb)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${stb_SOURCE_DIR}"
)
