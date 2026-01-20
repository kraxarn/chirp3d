include(FetchContent)

FetchContent_Declare(joltc
	GIT_REPOSITORY https://github.com/amerkoleci/joltc.git
	GIT_TAG cdab3860b22d56df7abe585b891c4fb88519c7a5
)

message(STATUS "Downloading joltc")
FetchContent_MakeAvailable(joltc)

# Jolt is written in C++
enable_language(CXX)

target_link_libraries(${PROJECT_NAME} PRIVATE joltc)
