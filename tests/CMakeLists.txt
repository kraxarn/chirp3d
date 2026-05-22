include(CTest)
enable_testing()

# TODO: This is very messy
function(add_test_executable name src)
	string(REPLACE "-" "_" TEST_SRC "${name}")
	add_executable(${PROJECT_NAME}-${name}
		"${CMAKE_CURRENT_SOURCE_DIR}/../src/${src}"
		"${TEST_SRC}.c"
	)
	target_include_directories(${PROJECT_NAME}-${name} PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/../include"
	)
	target_link_libraries(${PROJECT_NAME}-${name} PRIVATE SDL3::SDL3)
	add_test(NAME ${name} COMMAND ${PROJECT_NAME}-${name} ${ARGN})
endfunction()

#add_test_executable(test-math "math.c")

set(TESTS_TARGET "${PROJECT_NAME}_tests")
set(ENGINE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_executable(${TESTS_TARGET}
	main.c
	testarray.c
)

# TODO: Maybe move stuff we test to a separate library
target_sources(${TESTS_TARGET} PRIVATE
	"${ENGINE_SRC}/array.c"
)

target_include_directories(${TESTS_TARGET} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../include"
)

add_test(NAME test_array COMMAND ${TESTS_TARGET} 1)

target_link_libraries(${TESTS_TARGET} PRIVATE SDL3::SDL3)

if (TARGET SDL3-shared)
	add_custom_command(
		TARGET ${TESTS_TARGET} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:${TESTS_TARGET}>
		VERBATIM
	)
endif ()
