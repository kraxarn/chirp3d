include(FetchContent)

FetchContent_Declare(joltc
	GIT_REPOSITORY https://github.com/amerkoleci/joltc.git
	GIT_TAG cedc753d4cc9a1be565298b293dd09af44240074
)

message(STATUS "Downloading joltc")
FetchContent_MakeAvailable(joltc)

# Jolt is written in C++
enable_language(CXX)

target_link_libraries(${PROJECT_NAME} PRIVATE joltc)
