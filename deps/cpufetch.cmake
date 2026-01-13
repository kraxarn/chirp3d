include(FetchContent)

FetchContent_Declare(cpufetch
	GIT_REPOSITORY https://github.com/Dr-Noob/cpufetch.git
	GIT_TAG v1.07
)

message(STATUS "Downloading cpufetch")
FetchContent_MakeAvailable(cpufetch)

add_library(cpufetch STATIC)

if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^x86_64|AMD64$")
	set(ARCH "x86_64")
	target_compile_definitions(cpufetch PRIVATE ARCH_X86)
elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^aarch64|arm64|ARM64$")
	set(ARCH "aarch64")
	target_compile_definitions(cpufetch PRIVATE ARCH_ARM)
else ()
	message(FATAL_ERROR "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

target_sources(cpufetch PRIVATE
	"${cpufetch_SOURCE_DIR}/src/common/cpu.c"
	"${cpufetch_SOURCE_DIR}/src/common/udev.c"
	"${cpufetch_SOURCE_DIR}/src/common/printer.c"
	"${cpufetch_SOURCE_DIR}/src/common/global.c"
)

if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
		target_sources(cpufetch PRIVATE
			"${cpufetch_SOURCE_DIR}/src/common/freq.c"
		)
	endif ()

	if ("${ARCH}" STREQUAL "x86_64")
		target_sources(cpufetch PRIVATE
			"${cpufetch_SOURCE_DIR}/src/x86/cpuid.c"
			"${cpufetch_SOURCE_DIR}/src/x86/apic.c"
			"${cpufetch_SOURCE_DIR}/src/x86/cpuid_asm.c"
			"${cpufetch_SOURCE_DIR}/src/x86/uarch.c"
		)

		if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
			target_sources(cpufetch PRIVATE
				"${cpufetch_SOURCE_DIR}/src/x86/freq/freq.c"
				"${cpufetch_SOURCE_DIR}/src/x86/freq/freq_nov.c"
			)
		endif ()
	elseif ("${ARCH}" STREQUAL "aarch64")
		target_sources(cpufetch PRIVATE
			"${cpufetch_SOURCE_DIR}/src/arm/midr.c"
			"${cpufetch_SOURCE_DIR}/src/arm/uarch.c"
			"${cpufetch_SOURCE_DIR}/src/common/soc.c"
			"${cpufetch_SOURCE_DIR}/src/arm/soc.c"
			"${cpufetch_SOURCE_DIR}/src/common/pci.c"
			"${cpufetch_SOURCE_DIR}/src/arm/udev.c"
			"${cpufetch_SOURCE_DIR}/src/arm/sve.c"
		)

		if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
			target_sources(cpufetch PRIVATE
				"${cpufetch_SOURCE_DIR}/src/common/sysctl.c"
			)
		endif ()
	endif ()
else ()
	if ("${ARCH}" STREQUAL "x86_64")
		target_sources(cpufetch PRIVATE
			"${cpufetch_SOURCE_DIR}/src/x86/cpuid.c"
			"${cpufetch_SOURCE_DIR}/src/x86/apic.c"
			"${cpufetch_SOURCE_DIR}/src/x86/cpuid_asm.c"
			"${cpufetch_SOURCE_DIR}/src/x86/uarch.c"
		)
	elseif ("${ARCH}" STREQUAL "aarch64")
		target_sources(cpufetch PRIVATE
			"${cpufetch_SOURCE_DIR}/src/arm/midr.c"
			"${cpufetch_SOURCE_DIR}/src/arm/uarch.c"
			"${cpufetch_SOURCE_DIR}/src/common/soc.c"
			"${cpufetch_SOURCE_DIR}/src/arm/soc.c"
			"${cpufetch_SOURCE_DIR}/src/common/pci.c"
			"${cpufetch_SOURCE_DIR}/src/arm/udev.c"
			"${cpufetch_SOURCE_DIR}/src/arm/sve.c"
		)
	endif ()
endif ()

target_include_directories(cpufetch PUBLIC
	"${cpufetch_SOURCE_DIR}/src/common"
)

target_link_libraries(${PROJECT_NAME} PRIVATE cpufetch)
