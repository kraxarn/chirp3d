include(FetchContent)

FetchContent_Declare(stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG 28d546d5eb77d4585506a20480f4de2e706dff4c
)

message(STATUS "Downloading stb")
FetchContent_MakeAvailable(stb)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${stb_SOURCE_DIR}"
)
