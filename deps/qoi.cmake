include(FetchContent)

FetchContent_Declare(qoi
	GIT_REPOSITORY https://github.com/phoboslab/qoi.git
	GIT_TAG c2edcd3d7a164d81ed149073602e0ac3842442a4
)

message(STATUS "Downloading qoi")
FetchContent_MakeAvailable(qoi)

target_include_directories(${PROJECT_NAME} PUBLIC
	"${qoi_SOURCE_DIR}"
)
