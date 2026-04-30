include(FetchContent)

FetchContent_Declare(qoi
	GIT_REPOSITORY https://github.com/phoboslab/qoi.git
	GIT_TAG e084ec009b38c755acc40fe31d3f83ee17935b9d
)

message(STATUS "Downloading qoi")
FetchContent_MakeAvailable(qoi)

target_include_directories(${PROJECT_NAME} PUBLIC
	"${qoi_SOURCE_DIR}"
)
