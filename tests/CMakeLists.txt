include(CTest)
enable_testing()

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

add_test_executable(test-math "math.c")