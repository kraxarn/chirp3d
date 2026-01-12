include(FetchContent)

set(MAPLE_FONT_VERSION "v7.9")
set(MAPLE_FONT_HASH "2f7f702a121cd910751849dde76f431d81a5be1618507f5724606d3147028b88")

if (CMAKE_VERSION VERSION_LESS "3.24.0")
	FetchContent_Declare(maple_font
		URL "https://github.com/subframe7536/maple-font/releases/download/${MAPLE_FONT_VERSION}/MapleMonoNL-TTF.zip"
	)
else ()
	FetchContent_Declare(maple_font
		URL "https://github.com/subframe7536/maple-font/releases/download/${MAPLE_FONT_VERSION}/MapleMonoNL-TTF.zip"
		URL_HASH "SHA256=${MAPLE_FONT_HASH}"
		DOWNLOAD_EXTRACT_TIMESTAMP true
	)
endif ()

message(STATUS "Downloading maple fonts")
FetchContent_MakeAvailable(maple_font)
