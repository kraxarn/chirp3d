#include "ecs.h"
#include "logcategory.h"
#include "resources.h"
#include "shader.h"

#include "flecs.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

// >.<
#define STBTT_ifloor(x)   ((int)SDL_floor(x))
#define STBTT_iceil(x)    ((int)SDL_ceil(x))
#define STBTT_sqrt(x)     SDL_sqrt(x)
#define STBTT_pow(x,y)    SDL_pow(x,y)
#define STBTT_fmod(x,y)   SDL_fmod(x,y)
#define STBTT_cos(x)      SDL_cos(x)
#define STBTT_acos(x)     SDL_acos(x)
#define STBTT_fabs(x)     SDL_fabs(x)
#define STBTT_malloc(x,u) ((void)(u),SDL_malloc(x))
#define STBTT_free(x,u)   ((void)(u),SDL_free(x))
#define STBTT_assert(x)   SDL_assert(x)
#define STBTT_strlen(x)   SDL_strlen(x)
#define STBTT_memcpy      SDL_memcpy
#define STBTT_memset      SDL_memset

#define NK_IMPLEMENTATION
#include "nkui.h"

#include <stddef.h>

// TODO: Maybe move these to context

static SDL_GPUGraphicsPipeline *pipeline = nullptr;
static SDL_GPUSampler *sampler = nullptr;
static SDL_GPUTexture *font_texture = nullptr;

static SDL_GPUBuffer *vertex_buffer = nullptr;
static SDL_GPUBuffer *index_buffer = nullptr;
static Uint32 vertex_buffer_size = 0;
static Uint32 index_buffer_size = 0;

static nk_buffer_t buffer = {0};
static nk_draw_null_texture_t null_texture = {0};

static void *vertex_data = nullptr;
static void *element_data = nullptr;
static bool insert_toggle = false;

static constexpr size_t max_vertex_buffer = (size_t) (512 * 1024);
static constexpr size_t max_element_buffer = (size_t) (128 * 1024);

typedef struct
{
	vector2f_t position;
	vector2f_t uv;
	nk_colorf_t color;
} vertex_t;

#define texture_format() SDL_GetGPUSwapchainTextureFormat(device, window)

static bool create_pipeline(SDL_Window *window, SDL_GPUDevice *device)
{
	SDL_IOStream *vertex_source;
	SDL_IOStream *fragment_source;

	switch (shader_format(device))
	{
		case SDL_GPU_SHADERFORMAT_MSL:
			vertex_source = res_shader_nkui_vert_msl();
			fragment_source = res_shader_nkui_frag_msl();
			break;

		case SDL_GPU_SHADERFORMAT_SPIRV:
			vertex_source = res_shader_nkui_vert_spv();
			fragment_source = res_shader_nkui_frag_spv();
			break;

		case SDL_GPU_SHADERFORMAT_DXIL:
			vertex_source = res_shader_nkui_vert_dxil();
			fragment_source = res_shader_nkui_frag_dxil();
			break;

		default:
			return SDL_SetError("No shader");
	}

	SDL_GPUShader *vertex_shader = load_shader(device, vertex_source,
		SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);

	if (vertex_shader == nullptr)
	{
		return false;
	}

	SDL_GPUShader *fragment_shader = load_shader(device, fragment_source,
		SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);

	if (fragment_shader == nullptr)
	{
		SDL_ReleaseGPUShader(device, vertex_shader);
		return false;
	}

	pipeline = SDL_CreateGPUGraphicsPipeline(device, &(SDL_GPUGraphicsPipelineCreateInfo){
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
		.vertex_input_state.num_vertex_attributes = 3,
		.vertex_input_state.vertex_attributes = (SDL_GPUVertexAttribute[]){
			(SDL_GPUVertexAttribute){
				.location = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.offset = offsetof(vertex_t, position),
			},
			(SDL_GPUVertexAttribute){
				.location = 1,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.offset = offsetof(vertex_t, uv),
			},
			(SDL_GPUVertexAttribute){
				.location = 2,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				.offset = offsetof(vertex_t, color),
			},
		},
		.vertex_input_state.num_vertex_buffers = 1,
		.vertex_input_state.vertex_buffer_descriptions = &(SDL_GPUVertexBufferDescription){
			.slot = 0,
			.pitch = sizeof(vertex_t),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		},
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_CLOCKWISE,
		},
		.target_info.num_color_targets = 1,
		.target_info.color_target_descriptions = (SDL_GPUColorTargetDescription[]){
			(SDL_GPUColorTargetDescription){
				.format = texture_format(),
				.blend_state.enable_blend = true,
				.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD,
				.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			},
		},
	});

	SDL_ReleaseGPUShader(device, vertex_shader);
	SDL_ReleaseGPUShader(device, fragment_shader);

	return pipeline != nullptr;
}

static bool create_sampler(SDL_GPUDevice *device)
{
	sampler = SDL_CreateGPUSampler(device, &(SDL_GPUSamplerCreateInfo){
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	});

	return sampler != nullptr;
}

static bool create_data_buffers()
{
	vertex_data = SDL_malloc(max_vertex_buffer);
	element_data = SDL_malloc(max_element_buffer);

	return (bool) (vertex_data != nullptr && element_data != nullptr);
}

static void *nkui_alloc([[maybe_unused]] nk_handle userdata,
	[[maybe_unused]] void *old, const nk_size size)
{
	return SDL_malloc(size);
}

static void nkui_free([[maybe_unused]] nk_handle userdata, void *old)
{
	SDL_free(old);
}

static void clipboard_copy([[maybe_unused]] const nk_handle handle,
	const char *text, const int len)
{
	if (len <= 0)
	{
		SDL_LogWarn(LOG_CATEGORY_UI, "No text to copy");
		return;
	}

	char *temp = SDL_malloc((len + 1) * sizeof(char));
	SDL_memcpy(temp, text, (size_t) len);
	temp[len] = '\0';

	if (!SDL_SetClipboardText(temp))
	{
		SDL_LogError(LOG_CATEGORY_UI, "Failed to copy: %s",
			SDL_GetError());
	}

	SDL_free(temp);
}

static void clipboard_paste([[maybe_unused]] nk_handle handle,
	struct nk_text_edit *edit)
{
	const char *text = SDL_GetClipboardText();
	if (text == nullptr || SDL_strlen(text) <= 0)
	{
		SDL_LogWarn(LOG_CATEGORY_UI, "No text to paste");
		return;
	}

	if (!nk_textedit_paste(edit, text, nk_strlen(text)))
	{
		SDL_LogError(LOG_CATEGORY_UI, "Failed to paste: %s",
			SDL_GetError());
	}
}

static nk_allocator_t default_allocator()
{
	return (nk_allocator_t){
		.userdata = nk_handle_ptr(nullptr),
		.alloc = nkui_alloc,
		.free = nkui_free,
	};
}

static nk_font_atlas_t font_stash_begin()
{
	nk_font_atlas_t font_atlas = {
		.temporary = default_allocator(),
		.permanent = default_allocator(),
	};
	nk_font_atlas_begin(&font_atlas);

	return font_atlas;
}

static bool font_stash_end(SDL_GPUDevice *device, nk_context_t *context, nk_font_atlas_t *font_atlas)
{
	int width = 0;
	int height = 0;
	const void *image = nk_font_atlas_bake(font_atlas, &width, &height, NK_FONT_ATLAS_RGBA32);

	font_texture = SDL_CreateGPUTexture(device, &(SDL_GPUTextureCreateInfo){
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	});

	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device,
		&(SDL_GPUTransferBufferCreateInfo){
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = width * height * 4,
		}
	);
	if (transfer_buffer == nullptr)
	{
		SDL_ReleaseGPUTexture(device, font_texture);
		return false;
	}

	Uint8 *map = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	SDL_memcpy(map, image, (size_t) width * (size_t) height * 4);
	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd);

	SDL_UploadToGPUTexture(copy_pass,
		&(SDL_GPUTextureTransferInfo){
			.transfer_buffer = transfer_buffer,
			.offset = 0,
			.pixels_per_row = width,
			.rows_per_layer = height,
		}, &(SDL_GPUTextureRegion){
			.texture = font_texture,
			.w = width,
			.h = height,
			.d = 1,
		}, false
	);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(cmd);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	nk_font_atlas_end(font_atlas, nk_handle_ptr(font_texture), &null_texture);
	if (font_atlas->default_font)
	{
		nk_style_set_font(context, &font_atlas->default_font->handle);
	}

	return true;
}

static bool load_font(SDL_Window *window, SDL_GPUDevice *device, nk_context_t *context)
{
	const float scale = SDL_max(SDL_GetWindowDisplayScale(window), 1.F);

	nk_font_atlas_t font_atlas = font_stash_begin();

	struct nk_font_config font_config = nk_font_config(0.F);
	constexpr Uint8 max_oversample = 8; // To avoid blurry text
	font_config.oversample_v = max_oversample;
	font_config.oversample_h = max_oversample;

	SDL_IOStream *font_stream = res_font_cousine_ttf();
	size_t font_size = 0;
	void *font_data = SDL_LoadFile_IO(font_stream, &font_size, true);

	if (font_data == nullptr)
	{
		return false;
	}

	struct nk_font *font = nk_font_atlas_add_from_memory(&font_atlas,
		font_data, font_size, 15.F * scale, &font_config);

	if (font == nullptr)
	{
		return SDL_SetError("Invalid font atlas");
	}

	font->handle.height /= scale;
	nk_style_set_font(context, &font->handle);
	return font_stash_end(device, context, &font_atlas);
}

static const char *convert_result_string(const nk_convert_result_t result)
{
	switch (result)
	{
		case NK_CONVERT_SUCCESS:
			return "Success";

		case NK_CONVERT_INVALID_PARAM:
			return "Invalid parameter";

		case NK_CONVERT_COMMAND_BUFFER_FULL:
			return "Command buffer full";

		case NK_CONVERT_VERTEX_BUFFER_FULL:
			return "Vertex buffer full";

		case NK_CONVERT_ELEMENT_BUFFER_FULL:
			return "Element buffer full";

		default:
			return nullptr;
	}
}

bool nkui_render_upload(nk_context_t *context, SDL_GPUDevice *device,
	SDL_GPUCommandBuffer *command_buffer)
{
	struct nk_buffer nk_vertex_buffer;
	struct nk_buffer nk_element_buffer;
	nk_buffer_init_fixed(&nk_vertex_buffer, vertex_data, max_vertex_buffer);
	nk_buffer_init_fixed(&nk_element_buffer, element_data, max_element_buffer);

	const nk_convert_result_t result = nk_convert(context, &buffer,
		&nk_vertex_buffer, &nk_element_buffer,
		&(nk_convert_config_t){
			.vertex_layout = (nk_draw_vertex_layout_element_t[]){
				(nk_draw_vertex_layout_element_t){
					.attribute = NK_VERTEX_POSITION,
					.format = NK_FORMAT_FLOAT,
					.offset = offsetof(vertex_t, position),
				},
				(nk_draw_vertex_layout_element_t){
					.attribute = NK_VERTEX_TEXCOORD,
					.format = NK_FORMAT_FLOAT,
					.offset = offsetof(vertex_t, uv),
				},
				(nk_draw_vertex_layout_element_t){
					.attribute = NK_VERTEX_COLOR,
					.format = NK_FORMAT_R32G32B32A32_FLOAT,
					.offset = offsetof(vertex_t, color),
				},
				NK_VERTEX_LAYOUT_END,
			},
			.vertex_size = sizeof(vertex_t),
			.vertex_alignment = alignof(vertex_t),
			.tex_null = null_texture,
			.circle_segment_count = 22,
			.curve_segment_count = 22,
			.arc_segment_count = 22,
			.global_alpha = 1.0f,
			.shape_AA = NK_ANTI_ALIASING_OFF,
			.line_AA = NK_ANTI_ALIASING_OFF,
		}
	);

	if (result != NK_CONVERT_SUCCESS)
	{
		return SDL_SetError("%s", convert_result_string(result));
	}

	const Uint32 vertex_size = (Uint32) nk_vertex_buffer.needed;
	const Uint32 element_size = (Uint32) nk_element_buffer.needed;

	if (vertex_size == 0 || element_size == 0)
	{
		return true;
	}

	if (vertex_buffer_size < vertex_size)
	{
		SDL_ReleaseGPUBuffer(device, vertex_buffer);

		vertex_buffer_size = vertex_size * 2;
		vertex_buffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = vertex_buffer_size,
		});
	}

	if (index_buffer_size < element_size)
	{
		SDL_ReleaseGPUBuffer(device, index_buffer);

		index_buffer_size = element_size * 2;
		index_buffer = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = index_buffer_size,
		});
	}

	if (vertex_buffer == nullptr || index_buffer == nullptr)
	{
		return false;
	}

	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device,
		&(SDL_GPUTransferBufferCreateInfo){
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = vertex_size + element_size,
		}
	);
	if (transfer_buffer == nullptr)
	{
		return false;
	}

	Uint8 *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		return false;
	}

	SDL_memcpy(transfer_data, vertex_data, vertex_size);
	SDL_memcpy(transfer_data + vertex_size, element_data, element_size);

	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	SDL_UploadToGPUBuffer(copy_pass,
		&(SDL_GPUTransferBufferLocation){
			.transfer_buffer = transfer_buffer,
			.offset = 0,
		},
		&(SDL_GPUBufferRegion){
			.buffer = vertex_buffer,
			.offset = 0,
			.size = vertex_size,
		},
		false
	);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	return true;
}

bool nkui_render_draw(nk_context_t *context, SDL_Window *window,
	SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass)
{
	if (vertex_buffer == nullptr
		|| index_buffer == nullptr)
	{
		return true; // Nothing to render
	}

	SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

	SDL_BindGPUVertexBuffers(render_pass, 0, (SDL_GPUBufferBinding[]){
		(SDL_GPUBufferBinding){
			.buffer = vertex_buffer,
			.offset = 0,
		},
	}, 1);

	int width;
	int height;
	if (!SDL_GetWindowSize(window, &width, &height))
	{
		return false;
	}

	SDL_SetGPUViewport(render_pass, &(SDL_GPUViewport){
		.x = 0,
		.y = 0,
		.w = (float) width,
		.h = (float) height,
		.min_depth = 0,
		.max_depth = 1,
	});

	SDL_PushGPUVertexUniformData(command_buffer, 0,
		&(matrix4x4_t){
			.m = {
				2.F / (float) width, 0.F, 0.F, 0.F,
				0.F, -2.F / (float) height, 0.F, 0.F,
				0.F, 0.F, -1.F, 0.F,
				-1.F, 1.F, 0.F, 1.F,
			},
		}, sizeof(matrix4x4_t)
	);

	const nk_draw_command_t *command;
	Uint32 offset = 0;

	nk_draw_foreach(command, context, &buffer) // TODO: while?
	{
		if (command->elem_count == 0)
		{
			continue;
		}

		SDL_SetGPUScissor(render_pass, &(SDL_Rect){
			.x = SDL_max((int) command->clip_rect.x - 1, 0),
			.y = SDL_max((int) command->clip_rect.y - 1, 0),
			.w = (int) command->clip_rect.w + 2,
			.h = (int) command->clip_rect.h + 2,
		});

		SDL_GPUTexture *texture = command->texture.ptr;
		if (texture == nullptr)
		{
			texture = font_texture;
		}

		SDL_BindGPUFragmentSamplers(render_pass, 0, (SDL_GPUTextureSamplerBinding[]){
			(SDL_GPUTextureSamplerBinding){
				.texture = texture,
				.sampler = sampler,
			},
		}, 1);

		SDL_DrawGPUIndexedPrimitives(render_pass, command->elem_count,
			1, offset, 0, 0);

		offset += command->elem_count;
	}

	nk_clear(context);
	return true;
}

bool nkui_init(SDL_Window *window, SDL_GPUDevice *device, nk_context_t *context)
{
	const nk_allocator_t allocator = default_allocator();

	if (!nk_init(context, &allocator, nullptr))
	{
		return false;
	}

	nk_buffer_init(&buffer, &allocator, NK_BUFFER_DEFAULT_INITIAL_SIZE);

	if (!create_pipeline(window, device)
		|| !create_sampler(device)
		|| !create_data_buffers())
	{
		return false;
	}

	context->clip.copy = clipboard_copy;
	context->clip.paste = clipboard_paste;
	context->clip.userdata = nk_handle_ptr(nullptr);

	if (!load_font(window, device, context))
	{
		return false;
	}

	return true;
}

void nkui_deinit(nk_context_t *context, SDL_GPUDevice *device)
{
	nk_free(context);
	nk_buffer_free(&buffer);

	SDL_free(vertex_data);
	SDL_free(element_data);

	SDL_ReleaseGPUBuffer(device, vertex_buffer);
	SDL_ReleaseGPUBuffer(device, index_buffer);
	SDL_ReleaseGPUTexture(device, font_texture);
	SDL_ReleaseGPUSampler(device, sampler);
	SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
}

void nkui_handle_event(nk_context_t *context, const SDL_Event *event)
{
	const SDL_EventType event_type = event->type;

	if (event_type == SDL_EVENT_KEY_DOWN
		|| event_type == SDL_EVENT_KEY_UP)
	{
		const bool key_down = event->type == SDL_EVENT_KEY_DOWN;
		const bool ctrl_down = (event->key.mod & SDL_KMOD_CTRL) > 0;
		const SDL_Keycode key = event->key.key;

		// TODO: Maybe make this a map?

		if (key == SDLK_LSHIFT || key == SDLK_RSHIFT)
		{
			nk_input_key(context, NK_KEY_SHIFT, key_down);
		}
		else if (key == SDLK_DELETE)
		{
			nk_input_key(context, NK_KEY_DEL, key_down);
		}
		else if (key == SDLK_RETURN)
		{
			nk_input_key(context, NK_KEY_ENTER, key_down);
		}
		else if (key == SDLK_TAB)
		{
			nk_input_key(context, NK_KEY_TAB, key_down);
		}
		else if (key == SDLK_BACKSPACE)
		{
			nk_input_key(context, NK_KEY_BACKSPACE, key_down);
		}
		else if (key == SDLK_HOME)
		{
			nk_input_key(context, NK_KEY_TEXT_START, key_down);
			nk_input_key(context, NK_KEY_SCROLL_START, key_down);
		}
		else if (key == SDLK_END)
		{
			nk_input_key(context, NK_KEY_TEXT_END, key_down);
			nk_input_key(context, NK_KEY_SCROLL_END, key_down);
		}
		else if (key == SDLK_PAGEDOWN)
		{
			nk_input_key(context, NK_KEY_SCROLL_DOWN, key_down);
		}
		else if (key == SDLK_PAGEUP)
		{
			nk_input_key(context, NK_KEY_SCROLL_UP, key_down);
		}
		else if (key == SDLK_A)
		{
			nk_input_key(context, NK_KEY_TEXT_SELECT_ALL,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_Z)
		{
			nk_input_key(context, NK_KEY_TEXT_UNDO,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_R)
		{
			nk_input_key(context, NK_KEY_TEXT_REDO,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_C)
		{
			nk_input_key(context, NK_KEY_COPY,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_V)
		{
			nk_input_key(context, NK_KEY_PASTE,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_X)
		{
			nk_input_key(context, NK_KEY_CUT,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_B)
		{
			nk_input_key(context, NK_KEY_TEXT_LINE_START,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_E)
		{
			nk_input_key(context, NK_KEY_TEXT_LINE_END,
				(bool) (key_down && ctrl_down));
		}
		else if (key == SDLK_UP)
		{
			nk_input_key(context, NK_KEY_UP, key_down);
		}
		else if (key == SDLK_DOWN)
		{
			nk_input_key(context, NK_KEY_DOWN, key_down);
		}
		else if (key == SDLK_ESCAPE)
		{
			nk_input_key(context, NK_KEY_TEXT_RESET_MODE, key_down);
		}
		else if (key == SDLK_INSERT)
		{
			if (key_down)
			{
				insert_toggle = (bool) !insert_toggle;
			}

			if (insert_toggle)
			{
				nk_input_key(context, NK_KEY_TEXT_INSERT_MODE, key_down);
			}
			else
			{
				nk_input_key(context, NK_KEY_TEXT_REPLACE_MODE, key_down);
			}
		}
		else if (key == SDLK_LEFT)
		{
			if (ctrl_down)
			{
				nk_input_key(context, NK_KEY_TEXT_WORD_LEFT, key_down);
			}
			else
			{
				nk_input_key(context, NK_KEY_LEFT, key_down);
			}
		}
		else if (SDLK_RIGHT)
		{
			if (ctrl_down)
			{
				nk_input_key(context, NK_KEY_TEXT_WORD_RIGHT, key_down);
			}
			else
			{
				nk_input_key(context, NK_KEY_RIGHT, key_down);
			}
		}

		return;
	}

	if (event_type == SDL_EVENT_MOUSE_BUTTON_DOWN
		|| event_type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		const int cursor_x = (int) event->button.x;
		const int cursor_y = (int) event->button.y;
		const bool down = event->button.down;
		const SDL_MouseButtonFlags button = event->button.button;

		if (button == SDL_BUTTON_LEFT)
		{
			if (event->button.clicks > 1)
			{
				nk_input_button(context, NK_BUTTON_DOUBLE, cursor_x, cursor_y, down);
			}

			nk_input_button(context, NK_BUTTON_LEFT, cursor_x, cursor_y, down);
		}
		else if (button == SDL_BUTTON_MIDDLE)
		{
			nk_input_button(context, NK_BUTTON_MIDDLE, cursor_x, cursor_y, down);
		}
		else if (button == SDL_BUTTON_RIGHT)
		{
			nk_input_button(context, NK_BUTTON_RIGHT, cursor_x, cursor_y, down);
		}
		else if (button == SDL_BUTTON_X1)
		{
			nk_input_button(context, NK_BUTTON_X1, cursor_x, cursor_y, down);
		}
		else if (button == SDL_BUTTON_X2)
		{
			nk_input_button(context, NK_BUTTON_X2, cursor_x, cursor_y, down);
		}

		return;
	}

	if (event_type == SDL_EVENT_MOUSE_MOTION)
	{
		context->input.mouse.pos.x = event->motion.x;
		context->input.mouse.pos.y = event->motion.y;
		context->input.mouse.delta.x = context->input.mouse.pos.x - context->input.mouse.prev.x;
		context->input.mouse.delta.y = context->input.mouse.pos.y - context->input.mouse.prev.y;
		return;
	}

	if (event_type == SDL_EVENT_TEXT_INPUT)
	{
		const char *text = event->text.text;
		SDL_assert(text != nullptr);

		const size_t len = SDL_strlen(text);
		SDL_assert(len <= NK_UTF_SIZE);

		nk_glyph glyph;
		SDL_memcpy(glyph, event->text.text, len);

		nk_input_glyph(context, glyph);
		return;
	}

	if (event_type == SDL_EVENT_MOUSE_WHEEL)
	{
		nk_input_scroll(context, nk_vec2(event->wheel.x, event->wheel.y));
	}
}
