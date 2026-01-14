include(FetchContent)

FetchContent_Declare(cpuinfo
	GIT_REPOSITORY https://github.com/pytorch/cpuinfo.git
	GIT_TAG 0fea7f5f88243ee354df0e0082b5f27d13fc9551
)

message(STATUS "Downloading cpuinfo")

set(BUILD_SHARED_LIBS OFF)
set(CPUINFO_BUILD_TOOLS OFF)
set(CPUINFO_BUILD_UNIT_TESTS OFF)
set(CPUINFO_BUILD_MOCK_TESTS OFF)
set(CPUINFO_BUILD_BENCHMARKS OFF)
set(CPUINFO_BUILD_PKG_CONFIG OFF)
set(USE_SYSTEM_LIBS ON)

FetchContent_MakeAvailable(cpuinfo)

target_link_libraries(${PROJECT_NAME} PRIVATE cpuinfo)
