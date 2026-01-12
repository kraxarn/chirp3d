set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

target_compile_definitions(${PROJECT_NAME} PRIVATE
	# Fonts
	FONT_MONOGRAM_TTF_PATH="${RESOURCE_DIR}/fonts/monogram.ttf"
	FONT_MAPLE_MONO_NL_REGULAR_TTF_PATH="${maple_font_SOURCE_DIR}/MapleMonoNL-Regular.ttf"
	# Shaders
	SHADER_DEFAULT_VERT_DXIL_PATH="${RESOURCE_DIR}/shaders/dxil/default.vert.dxil"
	SHADER_DEFAULT_FRAG_DXIL_PATH="${RESOURCE_DIR}/shaders/dxil/default.frag.dxil"
	SHADER_DEFAULT_VERT_MSL_PATH="${RESOURCE_DIR}/shaders/msl/default.vert.msl"
	SHADER_DEFAULT_FRAG_MSL_PATH="${RESOURCE_DIR}/shaders/msl/default.frag.msl"
	SHADER_DEFAULT_VERT_SPV_PATH="${RESOURCE_DIR}/shaders/spv/default.vert.spv"
	SHADER_DEFAULT_FRAG_SPV_PATH="${RESOURCE_DIR}/shaders/spv/default.frag.spv"
)