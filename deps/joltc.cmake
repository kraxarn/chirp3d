include(FetchContent)

FetchContent_Declare(joltc
	GIT_REPOSITORY https://github.com/amerkoleci/joltc.git
	GIT_TAG 52d8c98df523f449eb3e01b1060a0fde052970d1
)

message(STATUS "Downloading joltc")
FetchContent_MakeAvailable(joltc)

# Jolt is written in C++
enable_language(CXX)

target_link_libraries(${PROJECT_NAME} PRIVATE joltc)
