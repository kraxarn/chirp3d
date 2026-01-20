include(FetchContent)

FetchContent_Declare(stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG c39c7023ebb833ce099750fe35509aca5662695e
)

message(STATUS "Downloading stb")
FetchContent_MakeAvailable(stb)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${stb_SOURCE_DIR}"
)
