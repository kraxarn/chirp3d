#include "ecs.h"
#include "logcategory.h"
#include "systems.h"
#include "systeminfo.h"
#include "gpudriver.h"
#include "gpushaderformat.h"
#include "shader.h"
#include "resources.h"
#include "model.h"

#include "flecs.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_properties.h>

static constexpr auto debug_mode =
#ifdef NDEBUG
	false;
#else
	true;
#endif

static void create_gpu_device(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);

	const SDL_PropertiesID props = SDL_CreateProperties();
	if (props == 0)
	{
		ecs_set_error("Memory error", SDL_GetError());
		return;
	}

	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debug_mode);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VERBOSE_BOOLEAN, debug_mode);

	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);

	// Disable unused features for higher compatibility
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, true);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN, false);

	SDL_GPUDevice *device = SDL_CreateGPUDeviceWithProperties(props);
	if (device == nullptr)
	{
		ecs_set_error("GPU error", SDL_GetError());
		return;
	}

	if (!SDL_ClaimWindowForGPUDevice(device, window))
	{
		ecs_set_error("Window error", SDL_GetError());
		SDL_DestroyGPUDevice(device);
		return;
	}

	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	const ecs_entity_t gpu_device_id = ecs_lookup(ecs_world(), "chirp.GpuDevice");

	ecs_set_id(ecs_world(), engine, gpu_device_id,
		sizeof(SDL_GPUDevice*), (const void*) &device);
}

static void log_gpu_info(ecs_iter_t *iter)
{
	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 0);

	SDL_LogInfo(LOG_CATEGORY_CORE, "Platform: %s",
		system_info_platform());

	SDL_LogInfo(LOG_CATEGORY_CORE, "CPU: %s",
		system_info_cpu_name());

	SDL_LogInfo(LOG_CATEGORY_CORE, "GPU: %s (%s)",
		system_info_gpu_name(device), system_info_gpu_driver(device));

	char *gpu_drivers = gpu_driver_names();
	SDL_LogInfo(LOG_CATEGORY_CORE, "Available GPU drivers: %s", gpu_drivers);

	char *shader_formats = shader_format_names(device);
	if (shader_formats != nullptr)
	{
		SDL_LogInfo(LOG_CATEGORY_CORE, "Available shader formats for %s: %s",
			SDL_GetGPUDeviceDriver(device), shader_formats);
	}
}

static void enable_vsync(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);
	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 1);

	if (!SDL_SetGPUSwapchainParameters(device, window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC))
	{
		SDL_LogWarn(LOG_CATEGORY_CORE, "VSync not supported: %s", SDL_GetError());
	}
}

static void create_depth_texture(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);
	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 1);

	vector2i_t depth_size;
	if (!SDL_GetWindowSize(window, &depth_size.x, &depth_size.y))
	{
		ecs_set_error("Window error", SDL_GetError());
		return;
	}

	const SDL_GPUTextureCreateInfo create_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.width = depth_size.x,
		.height = depth_size.y,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
	};

	SDL_GPUTexture *depth_texture = SDL_CreateGPUTexture(device, &create_info);
	if (depth_texture == nullptr)
	{
		ecs_set_error("Depth texture error", SDL_GetError());
		return;
	}

	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	const ecs_id_t depth_texture_id = ecs_lookup(ecs_world(), "chirp.DepthTexture");

	ecs_set_id(ecs_world(), engine, depth_texture_id,
		sizeof(SDL_GPUTexture*), (const void*) &depth_texture);
}

static void load_default_shaders(ecs_iter_t *iter)
{
	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 0);

	SDL_IOStream *vertex_source;
	SDL_IOStream *fragment_source;

	switch (shader_format(device))
	{
		case SDL_GPU_SHADERFORMAT_MSL:
			vertex_source = res_shader_default_vert_msl();
			fragment_source = res_shader_default_frag_msl();
			break;

		case SDL_GPU_SHADERFORMAT_SPIRV:
			vertex_source = res_shader_default_vert_spv();
			fragment_source = res_shader_default_frag_spv();
			break;

		case SDL_GPU_SHADERFORMAT_DXIL:
			vertex_source = res_shader_default_vert_dxil();
			fragment_source = res_shader_default_frag_dxil();
			break;

		default:
			ecs_set_error("Shader error", SDL_GetError());
			return;
	}

	SDL_GPUShader *vertex_shader = load_shader(device, vertex_source,
		SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);

	if (vertex_shader == nullptr)
	{
		ecs_set_error("Shader error", SDL_GetError());
		return;
	}

	SDL_GPUShader *fragment_shader = load_shader(device, fragment_source,
		SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);

	if (fragment_shader == nullptr)
	{
		ecs_set_error("Shader error", SDL_GetError());
		return;
	}

	const ecs_id_t vertex_shader_id = ecs_lookup(ecs_world(), "chirp.VertexShader");
	const ecs_id_t fragment_shader_id = ecs_lookup(ecs_world(), "chirp.FragmentShader");

	const ecs_entity_t entity = ecs_new(ecs_world());

	ecs_set_id(ecs_world(), entity, vertex_shader_id,
		sizeof(SDL_GPUShader*), (const void*) &vertex_shader);

	ecs_set_id(ecs_world(), entity, fragment_shader_id,
		sizeof(SDL_GPUShader*), (const void*) &fragment_shader);
}

static void create_default_pipeline(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);
	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 1);
	SDL_GPUShader *vertex_shader = *ecs_field(iter, vertex_shader_t*, 2);
	SDL_GPUShader *fragment_shader = *ecs_field(iter, fragment_shader_t*, 3);

	const SDL_GPUGraphicsPipelineCreateInfo create_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){
				(SDL_GPUColorTargetDescription){
					.format = SDL_GetGPUSwapchainTextureFormat(device, window),
					.blend_state = (SDL_GPUColorTargetBlendState){
						.enable_blend = true,
						.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.color_blend_op = SDL_GPU_BLENDOP_ADD,
						.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					},
				}
			},
			.has_depth_stencil_target = true,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){
				(SDL_GPUVertexBufferDescription){
					.slot = 0,
					.pitch = sizeof(vertex_t),
					.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				},
			},
			.num_vertex_attributes = 4,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){
				// Position
				(SDL_GPUVertexAttribute){
					.location = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(vertex_t, position),
				},
				// Normal
				(SDL_GPUVertexAttribute){
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(vertex_t, normal),
				},
				// Texture coordinate
				(SDL_GPUVertexAttribute){
					.location = 2,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = offsetof(vertex_t, tex_coord),
				},
				// Colour
				(SDL_GPUVertexAttribute){
					.location = 3,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = offsetof(vertex_t, color),
				},
			},
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_BACK,
		},
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
	};

	SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);

	// TODO: Delete entity instead
	SDL_ReleaseGPUShader(device, vertex_shader);
	SDL_ReleaseGPUShader(device, fragment_shader);

	if (pipeline == nullptr)
	{
		ecs_set_error("Pipeline error", SDL_GetError());
		return;
	}

	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	const ecs_id_t pipeline_id = ecs_lookup(ecs_world(), "chirp.GpuGraphicsPipeline");

	ecs_set_id(ecs_world(), engine, pipeline_id,
		sizeof(SDL_GPUGraphicsPipeline*), (const void*) &pipeline);
}

void system_register_gpu()
{
	const ecs_id_t window_id = ecs_lookup(ecs_world(), "chirp.Window");
	const ecs_id_t gpu_device_id = ecs_lookup(ecs_world(), "chirp.GpuDevice");
	const ecs_id_t vertex_shader_id = ecs_lookup(ecs_world(), "chirp.VertexShader");
	const ecs_id_t fragment_shader_id = ecs_lookup(ecs_world(), "chirp.FragmentShader");

	const ecs_observer_desc_t observer_desc[] = {
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = window_id}
			},
			.events = {EcsOnSet},
			.callback = create_gpu_device,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = gpu_device_id}
			},
			.events = {EcsOnSet},
			.callback = log_gpu_info,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = window_id},
				(ecs_term_t){.id = gpu_device_id}
			},
			.events = {EcsOnSet},
			.callback = enable_vsync,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = window_id},
				(ecs_term_t){.id = gpu_device_id}
			},
			.events = {EcsOnSet},
			.callback = create_depth_texture,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = gpu_device_id}
			},
			.events = {EcsOnSet},
			.callback = load_default_shaders,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = window_id},
				(ecs_term_t){.id = gpu_device_id},
				(ecs_term_t){.id = vertex_shader_id},
				(ecs_term_t){.id = fragment_shader_id},
			},
			.events = {EcsOnSet},
			.callback = create_default_pipeline,
		},
	};

	ecs_observer_init_all(observer_desc);
}
