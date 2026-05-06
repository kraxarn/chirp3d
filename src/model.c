#include "model.h"
#include "logcategory.h"
#include "uniformdata.h"
#include "vector.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#define CGLTF_MALLOC(size) SDL_malloc(size)
#define CGLTF_FREE(ptr)    SDL_free(ptr)
#define CGLTF_ATOI(str)    SDL_atoi(str)
#define CGLTF_ATOF(str)    SDL_atof(str)
#define CGLTF_ATOLL(str)   SDL_strtoll(str,nullptr,10)

#ifndef NDEBUG
#define CGLTF_VALIDATE_ENABLE_ASSERTS 1
#endif

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

// TODO: Duplicated from mesh/mesh_info

typedef Uint16 mesh_index_t;

typedef struct vertex_t
{
	vector3f_t position;
	vector3f_t normal;
	vector2f_t tex_coord;
} vertex_t;

typedef struct mesh_primitive_t
{
	vertex_t *vertices;
	size_t vertex_count;

	mesh_index_t *indices;
	size_t index_count;

	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUBuffer *index_buffer;
} mesh_primitive_t;

typedef struct material_t
{
	const char *name;
	SDL_FColor *color;
} material_t;

typedef struct model_t
{
	cgltf_data *data;

	SDL_GPUDevice *device;

	material_t *materials;
	size_t material_count;

	mesh_primitive_t *primitives;
	size_t primitive_count;

	// TODO: We probably don't want a single sampler/texture for the entire model
	SDL_GPUSampler *sampler;
	SDL_GPUTexture *texture;
} model_t;

[[nodiscard]]
static const char *cgltf_error_string(const cgltf_result result)
{
	switch (result)
	{
		case cgltf_result_data_too_short: return "Data too short";
		case cgltf_result_unknown_format: return "Unknown format";
		case cgltf_result_invalid_json: return "Invalid JSON";
		case cgltf_result_invalid_gltf: return "Invalid glTF";
		case cgltf_result_invalid_options: return "Invalid options";
		case cgltf_result_file_not_found: return "File not found";
		case cgltf_result_io_error: return "I/O error";
		case cgltf_result_out_of_memory: return "Out of memory";
		case cgltf_result_legacy_gltf: return "Legacy glTF";
		default: return "Unknown error";
	}
}

[[nodiscard]]
static const char *cgltf_type_string(const cgltf_type type)
{
	switch (type)
	{
		case cgltf_type_invalid: return "invalid";
		case cgltf_type_scalar: return "scalar";
		case cgltf_type_vec2: return "vector2";
		case cgltf_type_vec3: return "vector3";
		case cgltf_type_vec4: return "vector4";
		case cgltf_type_mat2: return "matrix2x2";
		case cgltf_type_mat3: return "matrix3x3";
		case cgltf_type_mat4: return "matrix4x4";
		default: return "unknown";
	}
}

[[nodiscard]]
static const char *cgltf_primitive_type_string(const cgltf_primitive_type type)
{
	switch (type)
	{
		case cgltf_primitive_type_invalid: return "invalid";
		case cgltf_primitive_type_points: return "points";
		case cgltf_primitive_type_lines: return "lines";
		case cgltf_primitive_type_line_loop: return "line loop";
		case cgltf_primitive_type_line_strip: return "line strip";
		case cgltf_primitive_type_triangles: return "triangles";
		case cgltf_primitive_type_triangle_strip: return "triangle strip";
		case cgltf_primitive_type_triangle_fan: return "triangle fan";
		default: return "unknown";
	}
}

[[nodiscard]]
static const char *cgltf_component_type_string(const cgltf_component_type type)
{
	switch (type)
	{
		case cgltf_component_type_invalid: return "invalid";
		case cgltf_component_type_r_8: return "byte";
		case cgltf_component_type_r_8u: return "unsigned byte";
		case cgltf_component_type_r_16: return "short";
		case cgltf_component_type_r_16u: return "unsigned short";
		case cgltf_component_type_r_32u: return "unsigned int";
		case cgltf_component_type_r_32f: return "float";
		default: return "unknown";
	}
}

[[nodiscard]]
static const char *cgltf_attribute_type_string(const cgltf_attribute_type type)
{
	switch (type)
	{
		case cgltf_attribute_type_invalid: return "invalid";
		case cgltf_attribute_type_position: return "position";
		case cgltf_attribute_type_normal: return "normal";
		case cgltf_attribute_type_tangent: return "tangent";
		case cgltf_attribute_type_texcoord: return "tex coord";
		case cgltf_attribute_type_color: return "color";
		case cgltf_attribute_type_joints: return "joints";
		case cgltf_attribute_type_weights: return "weights";
		case cgltf_attribute_type_custom: return "custom";
		default: return "unknown";
	}
}

static size_t append_debug_info(char *str, const size_t str_len,
	const char *key, const cgltf_size value)
{
	if (value <= 0)
	{
		return 0;
	}

	constexpr size_t temp_len = 32;
	char temp[temp_len];
	if (SDL_snprintf(temp, temp_len, ", %s: %zu", key, value) < 0)
	{
		return 0;
	}

	return SDL_strlcat(str, temp, str_len);
}

static void log_debug_info(const cgltf_data *data)
{
	constexpr size_t type_str_len = 5;
	char type_str[type_str_len];

	switch (data->file_type)
	{
		case cgltf_file_type_gltf:
			SDL_strlcpy(type_str, "gltf", type_str_len);
			break;

		case cgltf_file_type_glb:
			SDL_strlcpy(type_str, "glb", type_str_len);
			break;

		default:
			SDL_LogWarn(LOG_CATEGORY_MODEL, "Unknown model format");
			return;
	}

	constexpr size_t model_info_len = 256;
	char model_info[model_info_len];
	SDL_zeroa(model_info);

	append_debug_info(model_info, model_info_len, "meshes", data->meshes_count);
	append_debug_info(model_info, model_info_len, "materials", data->materials_count);
	append_debug_info(model_info, model_info_len, "accessors", data->accessors_count);
	append_debug_info(model_info, model_info_len, "buffer views", data->buffer_views_count);
	append_debug_info(model_info, model_info_len, "buffers", data->buffers_count);
	append_debug_info(model_info, model_info_len, "images", data->images_count);
	append_debug_info(model_info, model_info_len, "textures", data->textures_count);
	append_debug_info(model_info, model_info_len, "samplers", data->samplers_count);
	append_debug_info(model_info, model_info_len, "skins", data->skins_count);
	append_debug_info(model_info, model_info_len, "cameras", data->cameras_count);
	append_debug_info(model_info, model_info_len, "lights", data->lights_count);
	append_debug_info(model_info, model_info_len, "nodes", data->nodes_count);
	append_debug_info(model_info, model_info_len, "scenes", data->scenes_count);
	append_debug_info(model_info, model_info_len, "animations", data->animations_count);
	append_debug_info(model_info, model_info_len, "variants", data->variants_count);
	append_debug_info(model_info, model_info_len, "extensions", data->data_extensions_count);
	append_debug_info(model_info, model_info_len, "extensions used", data->extensions_used_count);
	append_debug_info(model_info, model_info_len, "extensions required", data->extensions_required_count);

	SDL_LogDebug(LOG_CATEGORY_MODEL, "Loaded model (type: %s%s)", type_str, model_info);
}

static SDL_Surface *default_texture()
{
	constexpr int size = 2;

	SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_ABGR8888);
	if (surface == nullptr)
	{
		return nullptr;
	}

	for (int i = 0; i < 4; i++)
	{
		constexpr Uint32 black = 0xff'00'00'00;
		constexpr Uint32 pink = 0xff'ff'00'ff;

		const SDL_Rect rect = {
			.x = (i == 0 || i == 3) ? 0 : (size / 2),
			.y = (i < 2) ? 0 : (size / 2),
			.w = size / 2,
			.h = size / 2,
		};

		SDL_FillSurfaceRect(surface, &rect, (i % 2 == 0) ? black : pink);
	}

	return surface;
}

static void materials_print(const material_t *materials, const size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		const material_t *material = materials + i;

		const SDL_FColor *color = material->color;

		SDL_LogDebug(LOG_CATEGORY_MODEL, "Material: %s\t%02x%02x%02x%02x", material->name,
			(Uint8) (color->r * SDL_ALPHA_OPAQUE),
			(Uint8) (color->g * SDL_ALPHA_OPAQUE),
			(Uint8) (color->b * SDL_ALPHA_OPAQUE),
			(Uint8) (color->a * SDL_ALPHA_OPAQUE)
		);
	}
}

static bool load_materials(model_t *model)
{
	model->material_count = model->data->materials_count;
	model->materials = SDL_malloc(sizeof(material_t) * model->material_count);

	if (model->materials == nullptr)
	{
		return false;
	}

	for (cgltf_size i = 0; i < model->data->materials_count; i++)
	{
		const cgltf_material *material = model->data->materials + i;

		model->materials[i].name = material->name;
		model->materials[i].color = (SDL_FColor*) material->pbr_metallic_roughness.base_color_factor;
	}

	materials_print(model->materials, model->material_count);

	return true;
}

#define model_property_t      cgltf_attribute_type
#define prop_vertex_position  cgltf_attribute_type_position
#define prop_vertex_normal    cgltf_attribute_type_normal
#define prop_vertex_tex_coord cgltf_attribute_type_texcoord
#define prop_index            cgltf_attribute_type_custom

static bool load_buffer_data(const cgltf_accessor *accessor, mesh_primitive_t *primitive,
	const model_property_t property)
{
	cgltf_type expected_type;
	cgltf_component_type expected_component_type;

	switch (property)
	{
		case prop_index:
			expected_type = cgltf_type_scalar;
			expected_component_type = cgltf_component_type_r_16u;
			break;

		case prop_vertex_position:
		case prop_vertex_normal:
			expected_type = cgltf_type_vec3;
			expected_component_type = cgltf_component_type_r_32f;
			break;

		case prop_vertex_tex_coord:
			expected_type = cgltf_type_vec2;
			expected_component_type = cgltf_component_type_r_32f;
			break;

		default:
			return SDL_SetError("Invalid property: %d", property);
	}

	if (accessor->type != expected_type
		|| accessor->component_type != expected_component_type)
	{
		return SDL_SetError("Invalid accessor type: %s %s, expected %s %s",
			cgltf_type_string(accessor->type),
			cgltf_component_type_string(accessor->component_type),
			cgltf_type_string(expected_type),
			cgltf_component_type_string(expected_component_type)
		);
	}

	if (property == prop_index
		&& primitive->indices == nullptr)
	{
		primitive->index_count = accessor->count;
		primitive->indices = SDL_calloc(primitive->index_count, sizeof(mesh_index_t));
	}

	if (property != prop_index
		&& primitive->vertices == nullptr)
	{
		primitive->vertex_count = accessor->count;
		primitive->vertices = SDL_calloc(primitive->vertex_count, sizeof(vertex_t));
	}

	void *buffer = accessor->buffer_view->buffer->data
		+ accessor->buffer_view->offset
		+ accessor->offset;

	for (size_t i = 0; i < accessor->count; i++)
	{
		switch (property)
		{
			case prop_index:
				primitive->indices[i] = ((mesh_index_t*) buffer)[i];
				break;

			case prop_vertex_position:
				primitive->vertices[i].position = ((vector3f_t*) buffer)[i];
				break;

			case prop_vertex_normal:
				primitive->vertices[i].normal = ((vector3f_t*) buffer)[i];
				break;

			case prop_vertex_tex_coord:
				primitive->vertices[i].tex_coord = ((vector2f_t*) buffer)[i];
				break;

			default:
				break;
		}
	}

	return true;
}

[[nodiscard]]
static bool supported_attribute(const cgltf_attribute_type type)
{
	return (bool) (type == prop_vertex_position
		|| type == prop_vertex_normal
		|| type == prop_vertex_tex_coord
		|| type == prop_index);
}

static bool load_model_data(model_t *model)
{
	if (model->data->nodes_count != 1)
	{
		return SDL_SetError("Only one node is supported, found %zu",
			model->data->nodes_count);
	}

	const cgltf_node *node = model->data->nodes;
	const cgltf_mesh *mesh = node->mesh;

	if (mesh == nullptr)
	{
		return SDL_SetError("No or invalid mesh");
	}

	model->primitive_count = mesh->primitives_count;
	model->primitives = SDL_calloc(model->primitive_count, sizeof(mesh_primitive_t));

	for (size_t i = 0; i < mesh->primitives_count; i++)
	{
		const cgltf_primitive *primitive = mesh->primitives + i;

		if (primitive->type != cgltf_primitive_type_triangles)
		{
			return SDL_SetError("Invalid primitive: %s",
				cgltf_primitive_type_string(primitive->type));
		}

		if (primitive->has_draco_mesh_compression)
		{
			return SDL_SetError("Draco compression is not supported");
		}

		mesh_primitive_t *model_primitive = model->primitives + i;

		if (primitive->indices != nullptr
			&& !load_buffer_data(primitive->indices, model_primitive, prop_index))
		{
			return false;
		}

		for (cgltf_size j = 0; j < primitive->attributes_count; j++)
		{
			const cgltf_attribute *attribute = primitive->attributes + j;

			if (!supported_attribute(attribute->type)
				|| !load_buffer_data(attribute->data, model_primitive, attribute->type))
			{
				return SDL_SetError("Unsupported attribute: %s (%s %s)",
					cgltf_attribute_type_string(attribute->type),
					cgltf_type_string(attribute->data->type),
					cgltf_component_type_string(attribute->data->component_type)
				);
			}
		}
	}

	return true;
}

static bool upload_texture(model_t *model)
{
	// TODO: Temporary
	// TODO: Duplicated from gpu_upload_texture

	SDL_Surface *surface = default_texture();
	if (surface == nullptr)
	{
		return false;
	}

	const SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	};
	model->sampler = SDL_CreateGPUSampler(model->device, &sampler_info);
	if (model->sampler == nullptr)
	{
		SDL_DestroySurface(surface);
		return false;
	}

	const SDL_GPUTextureCreateInfo texture_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.width = surface->w,
		.height = surface->h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	};
	model->texture = SDL_CreateGPUTexture(model->device, &texture_info);
	if (model->texture == nullptr)
	{
		SDL_DestroySurface(surface);
		SDL_ReleaseGPUSampler(model->device, model->sampler);
		model->sampler = nullptr;
		return false;
	}

	// RGBA for each pixel
	const Uint32 surface_size = surface->w * surface->h * 4;

	const SDL_GPUTransferBufferCreateInfo buffer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = surface_size,
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(model->device, &buffer_info);
	if (transfer_buffer == nullptr)
	{
		SDL_DestroySurface(surface);
		SDL_ReleaseGPUSampler(model->device, model->sampler);
		SDL_ReleaseGPUTexture(model->device, model->texture);
		model->sampler = nullptr;
		model->texture = nullptr;
		return false;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(model->device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_DestroySurface(surface);
		SDL_ReleaseGPUTransferBuffer(model->device, transfer_buffer);
		SDL_ReleaseGPUSampler(model->device, model->sampler);
		SDL_ReleaseGPUTexture(model->device, model->texture);
		model->sampler = nullptr;
		model->texture = nullptr;
		return false;
	}

	SDL_memcpy(transfer_data, surface->pixels, surface_size);
	SDL_UnmapGPUTransferBuffer(model->device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(model->device);
	if (command_buffer == nullptr)
	{
		SDL_DestroySurface(surface);
		SDL_ReleaseGPUTransferBuffer(model->device, transfer_buffer);
		SDL_ReleaseGPUSampler(model->device, model->sampler);
		SDL_ReleaseGPUTexture(model->device, model->texture);
		model->sampler = nullptr;
		model->texture = nullptr;
		return false;
	}

	const SDL_GPUTextureTransferInfo source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUTextureRegion destination = {
		.texture = model->texture,
		.w = surface->w,
		.h = surface->h,
		.d = 1,
	};
	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
	SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
	SDL_EndGPUCopyPass(copy_pass);

	SDL_DestroySurface(surface);

	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUTransferBuffer(model->device, transfer_buffer);
		SDL_ReleaseGPUSampler(model->device, model->sampler);
		SDL_ReleaseGPUTexture(model->device, model->texture);
		model->sampler = nullptr;
		model->texture = nullptr;
		return false;
	}

	SDL_ReleaseGPUTransferBuffer(model->device, transfer_buffer);

	return true;
}

static bool upload_mesh(SDL_GPUDevice *device, mesh_primitive_t *primitive)
{
	// TODO: Duplicated from gpu_upload_mesh_info

	const size_t vertex_size = sizeof(vertex_t) * primitive->vertex_count;
	const size_t index_size = sizeof(mesh_index_t) * primitive->index_count;

	const SDL_GPUBufferCreateInfo vertex_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = vertex_size,
	};
	primitive->vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
	if (primitive->vertex_buffer == nullptr)
	{
		return false;
	}

	const SDL_GPUBufferCreateInfo index_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = index_size,
	};
	primitive->index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
	if (primitive->index_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, primitive->vertex_buffer);
		primitive->vertex_buffer = nullptr;
		return false;
	}

	const SDL_GPUTransferBufferCreateInfo transfer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = vertex_size + index_size,
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
	if (transfer_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, primitive->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, primitive->index_buffer);
		primitive->vertex_buffer = nullptr;
		primitive->index_buffer = nullptr;
		return false;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, primitive->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, primitive->index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		primitive->vertex_buffer = nullptr;
		primitive->index_buffer = nullptr;
		return false;
	}

	SDL_memcpy(transfer_data, primitive->vertices, vertex_size);
	SDL_memcpy(transfer_data + vertex_size, primitive->indices, index_size);

	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (command_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, primitive->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, primitive->index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		primitive->vertex_buffer = nullptr;
		primitive->index_buffer = nullptr;
		return false;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	const SDL_GPUTransferBufferLocation vertex_source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUBufferRegion vertex_destination = {
		.buffer = primitive->vertex_buffer,
		.offset = 0,
		.size = vertex_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

	const SDL_GPUTransferBufferLocation index_source = {
		.transfer_buffer = transfer_buffer,
		.offset = vertex_size,
	};
	const SDL_GPUBufferRegion index_destination = {
		.buffer = primitive->index_buffer,
		.offset = 0,
		.size = index_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, false);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUBuffer(device, primitive->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, primitive->index_buffer);
		primitive->vertex_buffer = nullptr;
		primitive->index_buffer = nullptr;
		return false;
	}

	return true;
}

static bool upload_model(const model_t *model)
{
	for (size_t i = 0; i < model->primitive_count; i++)
	{
		if (!upload_mesh(model->device, model->primitives + i))
		{
			return false;
		}
	}

	return true;
}

model_t *model_create(SDL_GPUDevice *device, SDL_IOStream *stream, const bool close_io)
{
	size_t file_size;
	void *file_data = SDL_LoadFile_IO(stream, &file_size, close_io);
	if (file_data == nullptr)
	{
		return nullptr;
	}

	model_t *model = SDL_malloc(sizeof(model_t));
	if (model == nullptr)
	{
		return nullptr;
	}

	model->device = device;

	model->materials = nullptr;
	model->material_count = 0;

	model->primitives = nullptr;
	model->primitive_count = 0;

	const cgltf_options options = {};

	const Uint64 begin = SDL_GetTicks();

	cgltf_result result = cgltf_parse(&options, file_data, file_size, &model->data);
	SDL_free(file_data);

	if (result != cgltf_result_success)
	{
		SDL_SetError("%s", cgltf_error_string(result));
		SDL_free(model);
		return nullptr;
	}

	const Uint64 parse_end = SDL_GetTicks();
	SDL_LogDebug(LOG_CATEGORY_MODEL, "Parsed model in %lu ms", parse_end - begin);

	result = cgltf_load_buffers(&options, model->data, nullptr);
	if (result != cgltf_result_success)
	{
		SDL_SetError("%s", cgltf_error_string(result));
		SDL_free(model);
		return nullptr;
	}

	const Uint64 buffer_end = SDL_GetTicks();
	SDL_LogDebug(LOG_CATEGORY_MODEL, "Loaded buffers in %lu ms", buffer_end - parse_end);

	log_debug_info(model->data);

	if (!load_materials(model)
		|| !load_model_data(model)
		|| !upload_model(model)
		|| !upload_texture(model))
	{
		model_destroy(model);
		return nullptr;
	}

	const Uint64 model_end = SDL_GetTicks();
	SDL_LogDebug(LOG_CATEGORY_MODEL, "Loaded model data in %lu ms", model_end - buffer_end);

	return model;
}

void model_destroy(model_t *model)
{
	if (model == nullptr)
	{
		return;
	}

	cgltf_free(model->data);

	SDL_free(model->materials);

	for (size_t i = 0; i < model->primitive_count; i++)
	{
		const mesh_primitive_t *primitive = model->primitives + i;

		SDL_ReleaseGPUBuffer(model->device, primitive->vertex_buffer);
		SDL_ReleaseGPUBuffer(model->device, primitive->index_buffer);
		SDL_free(primitive->vertices);
	}
	SDL_free(model->primitives);

	SDL_ReleaseGPUTexture(model->device, model->texture);
	SDL_ReleaseGPUSampler(model->device, model->sampler);

	SDL_free(model);
}

static void mesh_draw(const model_t *model, const mesh_primitive_t *primitive, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, const matrix4x4_t projection)
{
	// TODO: Duplicated from mesh

	const SDL_GPUBufferBinding vertex_binding = {
		.buffer = primitive->vertex_buffer,
		.offset = 0,
	};
	SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

	const SDL_GPUBufferBinding index_binding = {
		.buffer = primitive->index_buffer,
		.offset = 0,
	};
	SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	const SDL_GPUTextureSamplerBinding binding = {
			.texture = model->texture,
			.sampler = model->sampler,
		};
		SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);

	const vertex_uniform_data_t vertex_data = {
		.mvp = projection,
	};
	SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_data, sizeof(vertex_uniform_data_t));

	SDL_DrawGPUIndexedPrimitives(render_pass, primitive->index_count,
		1, 0, 0, 0);
}

void model_draw(const model_t *model, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, const matrix4x4_t projection)
{
	for (size_t i = 0; i < model->primitive_count; i++)
	{
		mesh_draw(model, model->primitives + i, render_pass, command_buffer, projection);
	}
}
