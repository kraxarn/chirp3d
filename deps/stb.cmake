include(FetchContent)

FetchContent_Declare(stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG f1c79c02822848a9bed4315b12c8c8f3761e1296
)

message(STATUS "Downloading stb")
FetchContent_MakeAvailable(stb)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${stb_SOURCE_DIR}"
)

function(add_stb_library NAME)
	string(TOUPPER "${NAME}" NAME_UPPER)
	add_library(${NAME} SHARED)
	set_target_properties(${NAME} PROPERTIES LINKER_LANGUAGE C)
	target_compile_definitions(${NAME} PRIVATE "${NAME_UPPER}_IMPLEMENTATION")
	target_sources(${NAME} PRIVATE "${stb_SOURCE_DIR}/${NAME}.h")
	target_link_libraries(${PROJECT_NAME} PRIVATE "${NAME}")
endfunction()

add_stb_library(stb_ds)