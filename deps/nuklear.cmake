include(FetchContent)

FetchContent_Declare(nuklear
	GIT_REPOSITORY https://github.com/Immediate-Mode-UI/Nuklear.git
	GIT_TAG v4.13.3
)

message(STATUS "Downloading nuklear")
FetchContent_MakeAvailable(nuklear)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${nuklear_SOURCE_DIR}"
)
