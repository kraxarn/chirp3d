include(FetchContent)

FetchContent_Declare(stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
)

message(STATUS "Downloading stb")
FetchContent_MakeAvailable(stb)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${stb_SOURCE_DIR}"
)
