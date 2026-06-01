include(FetchContent)

FetchContent_Declare(qoi
	GIT_REPOSITORY https://github.com/phoboslab/qoi.git
	GIT_TAG 97bacc86a9c4abf5a2d452102dc26546c4c670b9
)

message(STATUS "Downloading qoi")
FetchContent_MakeAvailable(qoi)

target_include_directories(${PROJECT_NAME} PUBLIC
	"${qoi_SOURCE_DIR}"
)
