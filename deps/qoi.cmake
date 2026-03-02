include(FetchContent)

FetchContent_Declare(qoi
	GIT_REPOSITORY https://github.com/phoboslab/qoi.git
	GIT_TAG 6fff9b70dd79b12f808b0acc5cb44fde9998725e
)

message(STATUS "Downloading qoi")
FetchContent_MakeAvailable(qoi)

target_include_directories(${PROJECT_NAME} PUBLIC
	"${qoi_SOURCE_DIR}"
)
