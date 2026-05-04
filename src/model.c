#include "model.h"
#include "logcategory.h"
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

	Uint16 *indices;
	size_t index_count;
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

static bool load_indices(mesh_primitive_t *primitive, const cgltf_accessor *indices)
{
	if (indices == nullptr || indices->buffer_view == nullptr)
	{
		return SDL_SetError("Primitive has no indices");
	}

	if (indices->component_type != cgltf_component_type_r_16u)
	{
		return SDL_SetError("Only unsigned short (u16) indices are supported");
	}

	if (indices->offset != 0) // TODO
	{
		SDL_LogWarn(LOG_CATEGORY_MODEL, "Index offset not supported, expect corrupted model");
	}

	const cgltf_buffer_view *buffer_view = indices->buffer_view;

	if (buffer_view->stride != 0) // TODO
	{
		SDL_LogWarn(LOG_CATEGORY_MODEL, "Index stride not supported, expect corrupted model");
	}

	primitive->index_count = indices->count;
	primitive->indices = (mesh_index_t*) buffer_view->buffer->data
		+ (buffer_view->offset / sizeof(Uint16));

	return true;
}

static bool vertices_valid(mesh_primitive_t *primitive, const cgltf_accessor *data)
{
	if (data->offset != 0) // TODO
	{
		return SDL_SetError("Buffer offset is currently not supported");
	}

	const cgltf_buffer_view *buffer_view = data->buffer_view;

	if (buffer_view->stride != 0) // TODO
	{
		return SDL_SetError("Buffer stride is currently not supported");
	}

	if (primitive->vertex_count > 0
		&& primitive->vertices != nullptr
		&& primitive->vertex_count != data->count)
	{
		return SDL_SetError("Unexpected vertex count, found %zu but expected %zu",
			data->count, primitive->vertex_count);
	}

	if (primitive->vertex_count == 0
		&& primitive->vertices == nullptr)
	{
		primitive->vertex_count = data->count;
		primitive->vertices = SDL_calloc(primitive->vertex_count, sizeof(vertex_t));
	}

	return true;
}

#define load_buffer_data(a,p,f,t,gt,gc)								\
	if (a->type != cgltf_type_##gt									\
		|| a->component_type != cgltf_component_type_##gc)			\
	{																\
		return SDL_SetError("Only %s %s values are supported",		\
			cgltf_type_string(cgltf_type_##gt),						\
			cgltf_component_type_string(cgltf_component_type_##gc)	\
		);															\
	}																\
	if ((p)->vertices == nullptr)									\
	{																\
		return SDL_SetError("Vertex memory error");					\
	}																\
	for (size_t vi = 0; vi < (p)->vertex_count; vi++)				\
	{																\
		(p)->vertices[vi].f = ((t*) ((a)->buffer_view->buffer->data	\
			+ ((a)->buffer_view->offset / sizeof(float))))[vi];		\
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

		if (!load_indices(model->primitives + i, primitive->indices))
		{
			return false;
		}

		for (cgltf_size j = 0; j < primitive->attributes_count; j++)
		{
			const cgltf_attribute *attribute = primitive->attributes + j;
			mesh_primitive_t *model_primitive = model->primitives + i;

			if (attribute->type == cgltf_attribute_type_position)
			{
				load_buffer_data(attribute->data, model_primitive, position, vector3f_t, vec3, r_32f);
			}
			else if (attribute->type == cgltf_attribute_type_normal)
			{
				load_buffer_data(attribute->data, model_primitive, normal, vector3f_t, vec3, r_32f);
			}
			else if (attribute->type == cgltf_attribute_type_texcoord)
			{
				load_buffer_data(attribute->data, model_primitive, tex_coord, vector2f_t, vec2, r_32f);
			}
			else
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

	if (!load_materials(model) || !load_model_data(model))
	{
		model_destroy(model);
		return nullptr;
	}

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
		SDL_free(primitive->vertices);
	}
	SDL_free(model->primitives);

	SDL_free(model);
}
