#include "args.h"
#include "ecs.h"
#include "gpudriver.h"
#include "gpushaderformat.h"
#include "logcategory.h"
#include "model.h"
#include "resources.h"
#include "shader.h"
#include "systeminfo.h"
#include "ecs/components.h"
#include "ecs/events.h"
#include "ecs/tags.h"

#include "flecs.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_properties.h>

[[nodiscard]]
static bool debug_mode(const arg_option_t option)
{
	switch (option)
	{
		case OPT_NOT_SET:
#ifdef NDEBUG
			return false;
#else
			return true;
#endif

		case OPT_DISABLE:
			return false;

		case OPT_ENABLE:
			return true;

		default:
			return false;
	}
}

static void create_gpu_device(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);
	const args_t args = *ecs_field(iter, args_t, 1);

	const SDL_PropertiesID props = SDL_CreateProperties();
	if (props == 0)
	{
		ecs_set_error("Memory error", SDL_GetError());
		return;
	}

	if (args.prefer_low_power != OPT_NOT_SET)
	{
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN,
			args.prefer_low_power == OPT_ENABLE);
	}

	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN,
		debug_mode(args.gpu_debug_mode));

	SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VERBOSE_BOOLEAN,
		debug_mode(args.gpu_debug_mode));

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

	ecs_set_id(ecs_world(), EcsEngine, EcsGpuDevice,
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

static SDL_GPUTexture *create_depth_texture(SDL_GPUDevice *device, const vector2i_t size)
{
	const SDL_GPUTextureCreateInfo create_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.width = size.x,
		.height = size.y,
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
		return nullptr;
	}

	return depth_texture;
}

static void set_depth_texture(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);
	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 1);

	vector2i_t size;
	if (!SDL_GetWindowSize(window, &size.x, &size.y))
	{
		ecs_set_error("Window error", SDL_GetError());
		return;
	}

	SDL_GPUTexture *depth_texture = create_depth_texture(device, size);
	if (depth_texture == nullptr)
	{
		return;
	}

	ecs_set_id(ecs_world(), EcsEngine, EcsDepthTexture,
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

	const ecs_entity_t entity = ecs_new(ecs_world());

	ecs_set_id(ecs_world(), entity, EcsVertexShader,
		sizeof(SDL_GPUShader*), (const void*) &vertex_shader);

	ecs_set_id(ecs_world(), entity, EcsFragmentShader,
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

	ecs_set_id(ecs_world(), EcsEngine, EcsGpuGraphicsPipeline,
		sizeof(SDL_GPUGraphicsPipeline*), (const void*) &pipeline);
}

static void resize_depth_texture(ecs_iter_t *iter)
{
	const SDL_WindowEvent *event = ecs_field(iter, SDL_WindowEvent, 0);
	const vector2i_t size = {
		.x = event->data1,
		.y = event->data2,
	};

	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 1);
	SDL_GPUTexture **depth_texture = ecs_field(iter, depth_texture_t*, 2);

	SDL_ReleaseGPUTexture(device, *depth_texture);
	*depth_texture = create_depth_texture(device, size);
}

void ecs_add_gpu()
{
	const ecs_observer_desc_t observer_desc[] = {
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsWindow, .inout = EcsInOut},
				(ecs_term_t){.id = EcsArgs, .src.name = "$args", .inout = EcsIn},
			},
			.events = {EcsOnSet},
			.callback = create_gpu_device,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsGpuDevice}
			},
			.events = {EcsOnSet},
			.callback = log_gpu_info,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsWindow},
				(ecs_term_t){.id = EcsGpuDevice}
			},
			.events = {EcsOnSet},
			.callback = enable_vsync,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsWindow},
				(ecs_term_t){.id = EcsGpuDevice}
			},
			.events = {EcsOnSet},
			.callback = set_depth_texture,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsWindowEvent, .inout = EcsIn},
				(ecs_term_t){.id = EcsGpuDevice, .src.name = "$g", .inout = EcsIn},
				(ecs_term_t){.id = EcsDepthTexture, .src.name = "$d", .inout = EcsInOut},
			},
			.events = {EcsOnWindowResized},
			.callback = resize_depth_texture,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsGpuDevice}
			},
			.events = {EcsOnSet},
			.callback = load_default_shaders,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsWindow, .src.name = "$engine", .inout = EcsIn},
				(ecs_term_t){.id = EcsGpuDevice, .src.name = "$engine", .inout = EcsIn},
				(ecs_term_t){.id = EcsVertexShader, .inout = EcsIn},
				(ecs_term_t){.id = EcsFragmentShader, .inout = EcsIn},
			},
			.events = {EcsOnSet},
			.callback = create_default_pipeline,
		},
	};

	ecs_observer_init_all(observer_desc);
}
