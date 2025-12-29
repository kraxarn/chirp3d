#include "font.h"
#include "logcategory.h"
#include "meshinfo.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>

// >.<
#define STBTT_ifloor(x) ((int) SDL_floor(x))
#define STBTT_iceil(x) ((int) SDL_ceil(x))
#define STBTT_sqrt(x) SDL_sqrt(x)
#define STBTT_pow(x,y) SDL_pow(x,y)
#define STBTT_fmod(x,y) SDL_fmod(x,y)
#define STBTT_cos(x) SDL_cos(x)
#define STBTT_acos(x) SDL_acos(x)
#define STBTT_fabs(x) SDL_fabs(x)
#define STBTT_malloc(x,u) ((void)(u),SDL_malloc(x))
#define STBTT_free(x,u) ((void)(u),SDL_free(x))
#define STBTT_assert(x) SDL_assert(x)
#define STBTT_strlen(x) SDL_strlen(x)
#define STBTT_memcpy SDL_memcpy
#define STBTT_memset SDL_memset

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// ASCII printable characters
static constexpr auto ascii_begin = 32;
static constexpr auto ascii_size = 95;

static constexpr auto font_size = 32;
static constexpr auto glyph_padding = 4;

typedef struct glyph_info_t
{
	int offset_x;
	int offset_y;
	int advance_x;
	SDL_Rect rect;
} glyph_info_t;

typedef struct font_t
{
	SDL_Color color;
	glyph_info_t glyphs[ascii_size];

	SDL_GPUDevice *device;
	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUBuffer *index_buffer;
	SDL_GPUTexture *texture;
	SDL_GPUSampler *sampler;
} font_t;

static bool build_palette(SDL_Surface *surface, const SDL_Color color)
{
	constexpr auto color_count = 256; // INDEX8

	SDL_Palette *palette = SDL_CreateSurfacePalette(surface);
	if (palette == nullptr)
	{
		return false;
	}

	SDL_Color colors[color_count];
	for (auto i = 0; i < color_count; i++)
	{
		colors[i].r = color.r;
		colors[i].g = color.g;
		colors[i].b = color.b;
		colors[i].a = i;
	}

	return SDL_SetPaletteColors(palette, colors, 0, color_count);
}

static bool upload_mesh_data(font_t *font)
{
	constexpr size_t num_vertices = 4;
	constexpr size_t num_indices = 6;

	constexpr size_t vertex_size = sizeof(vertex_t) * num_vertices;
	constexpr size_t index_size = sizeof(mesh_index_t) * num_indices;

	const SDL_GPUBufferCreateInfo vertex_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = vertex_size,
	};
	font->vertex_buffer = SDL_CreateGPUBuffer(font->device, &vertex_buffer_info);
	if (font->vertex_buffer == nullptr)
	{
		return false;
	}

	const SDL_GPUBufferCreateInfo index_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = index_size,
	};
	font->index_buffer = SDL_CreateGPUBuffer(font->device, &index_buffer_info);
	if (font->index_buffer == nullptr)
	{
		return false;
	}

	const SDL_GPUTransferBufferCreateInfo transfer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = vertex_size + index_size,
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(font->device, &transfer_info);
	if (transfer_buffer == nullptr)
	{
		return false;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(font->device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		return false;
	}

	const vertex_t vertices[] = {
		(vertex_t){-1, 1, 0, 0, 0},
		(vertex_t){1, 1, 0, 4, 0},
		(vertex_t){1, -1, 0, 4, 4},
		(vertex_t){-1, -1, 0, 0, 4},
	};

	const mesh_index_t indices[] = {
		0, 1, 2,
		0, 2, 3,
	};

	SDL_memcpy(transfer_data, vertices, sizeof(vertices));
	SDL_memcpy(transfer_data + sizeof(vertices), indices, sizeof(indices));

	SDL_UnmapGPUTransferBuffer(font->device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(font->device);
	if (command_buffer == nullptr)
	{
		return false;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	const SDL_GPUTransferBufferLocation vertex_source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUBufferRegion vertex_destination = {
		.buffer = font->vertex_buffer,
		.offset = 0,
		.size = vertex_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

	const SDL_GPUTransferBufferLocation index_source = {
		.transfer_buffer = transfer_buffer,
		.offset = vertex_size,
	};
	const SDL_GPUBufferRegion index_destination = {
		.buffer = font->index_buffer,
		.offset = 0,
		.size = index_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, false);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_ReleaseGPUTransferBuffer(font->device, transfer_buffer);

	return SDL_SubmitGPUCommandBuffer(command_buffer);
}

static bool upload_atlas(font_t *font, SDL_Surface *atlas)
{
	const SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};

	font->sampler = SDL_CreateGPUSampler(font->device, &sampler_info);
	if (font->sampler == nullptr)
	{
		return false;
	}

	const SDL_GPUTextureCreateInfo texture_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.width = atlas->w,
		.height = atlas->h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	};

	font->texture = SDL_CreateGPUTexture(font->device, &texture_info);
	if (font->texture == nullptr)
	{
		return false;
	}

	const SDL_GPUTransferBufferCreateInfo buffer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = atlas->w * atlas->h * 4,
	};

	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(font->device, &buffer_info);
	if (transfer_buffer == nullptr)
	{
		return false;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(font->device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_ReleaseGPUTransferBuffer(font->device, transfer_buffer);
		return false;
	}

	SDL_memcpy(transfer_data, atlas->pixels, (size_t) (atlas->w * atlas->h * 4));
	SDL_UnmapGPUTransferBuffer(font->device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(font->device);
	if (command_buffer == nullptr)
	{
		return false;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	const SDL_GPUTextureTransferInfo source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUTextureRegion destination = {
		.texture = font->texture,
		.w = atlas->w,
		.h = atlas->h,
		.d = 1,
	};
	SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);

	SDL_EndGPUCopyPass(copy_pass);
	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUTransferBuffer(font->device, transfer_buffer);
		return false;
	}

	SDL_ReleaseGPUTransferBuffer(font->device, transfer_buffer);

	return true;
}

static bool font_bake(font_t *font, const Uint8 *data)
{
	const Uint64 start = SDL_GetTicks();

	glyph_info_t *glyphs = font->glyphs;

	stbtt_fontinfo font_info;
	if (!stbtt_InitFont(&font_info, data, 0))
	{
		return false;
	}

	const float scale = stbtt_ScaleForPixelHeight(&font_info, (float) font_size);

	int ascent;
	int descent;
	int line_gap;
	stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

	SDL_Surface *surfaces[ascii_size];

	for (auto i = 0; i < ascii_size; i++)
	{
		const int codepoint = ascii_begin + i;
		glyph_info_t *glyph = font->glyphs + i;

		const int index = stbtt_FindGlyphIndex(&font_info, codepoint);
		if (index <= 0)
		{
			return SDL_SetError("Invalid codepoint: %d", codepoint);
		}

		stbtt_GetGlyphBitmapBox(&font_info, index, scale, scale,
			&glyph->offset_x, &glyph->offset_y, nullptr, nullptr
		);

		stbtt_GetCodepointHMetrics(&font_info, codepoint, &glyph->advance_x, nullptr);
		glyph->advance_x = (int) ((float) glyph->advance_x * scale);
		glyph->offset_y += (int) ((float) ascent * scale);

		int glyph_width;
		int glyph_height;

		if (codepoint == ' ')
		{
			glyph_width = glyph->advance_x;
			glyph_height = font_size;
		}
		else
		{
			int box_x0;
			int box_y0;
			int box_x1;
			int box_y1;
			stbtt_GetCodepointBitmapBox(&font_info, codepoint, scale, scale,
				&box_x0, &box_y0, &box_x1, &box_y1
			);

			glyph_width = box_x1 - box_x0;
			glyph_height = box_y1 - box_y0;
		}

		surfaces[i] = SDL_CreateSurface(glyph_width, glyph_height, SDL_PIXELFORMAT_INDEX8);
		if (surfaces[i] == nullptr || !build_palette(surfaces[i], font->color))
		{
			return false;
		}

		stbtt_MakeCodepointBitmap(&font_info, surfaces[i]->pixels, glyph_width, glyph_height,
			glyph_width, scale, scale, codepoint
		);
	}

	auto total_width = 0;
	auto max_glyph_width = 0;

	for (auto i = 0; i < ascii_size; i++)
	{
		max_glyph_width = SDL_max(max_glyph_width, surfaces[i]->w);
		total_width += surfaces[i]->w + (2 * glyph_padding);
	}

	constexpr int padded_font_size = font_size + (2 * glyph_padding);
	const float total_area = (float) total_width * (float) padded_font_size * 1.2F;
	const float image_min_size = SDL_sqrtf(total_area);
	const auto image_size = (int) SDL_powf(2, SDL_ceilf(SDL_logf(image_min_size) / SDL_logf(2)));

	const int atlas_width = image_size;
	const int atlas_height = (int) total_area < ((image_size * image_size) / 2) ? image_size / 2 : image_size;
	SDL_Surface *atlas = SDL_CreateSurface(atlas_width, atlas_height, SDL_PIXELFORMAT_INDEX8);

	if (atlas == nullptr || !build_palette(atlas, font->color))
	{
		return false;
	}

	stbrp_context context;
	stbrp_node nodes[ascii_size];

	stbrp_init_target(&context, atlas->w, atlas->h, nodes, ascii_size);
	stbrp_rect rects[ascii_size];

	for (auto i = 0; i < ascii_size; i++)
	{
		rects[i].id = i;
		rects[i].w = surfaces[i]->w + (2 * glyph_padding);
		rects[i].h = surfaces[i]->h + (2 * glyph_padding);
	}

	if (!stbrp_pack_rects(&context, rects, ascii_size))
	{
		return SDL_SetError("Font packing failed");
	}

	for (auto i = 0; i < ascii_size; i++)
	{
		if (!rects[i].was_packed)
		{
			SDL_LogWarn(LOG_CATEGORY_FONT, "Invalid character for packaging: '%c'", ascii_begin + i);
			continue;
		}

		glyph_info_t *glyph = glyphs + i;
		SDL_Rect *rect = &glyph->rect;

		rect->x = rects[i].x + glyph_padding;
		rect->y = rects[i].y + glyph_padding;
		rect->w = surfaces[i]->w;
		rect->h = surfaces[i]->h;

		for (auto y = 0; y < surfaces[i]->h; y++)
		{
			for (auto x = 0; x < surfaces[i]->w; x++)
			{
				((unsigned char *) atlas->pixels)[((rects[i].y + glyph_padding + y) * atlas->w) + (rects[i].x +
					glyph_padding + x)] = ((unsigned char *) surfaces[i]->pixels)[(y * surfaces[i]->w) + x];
			}
		}

		SDL_DestroySurface(surfaces[i]);
	}

	SDL_Surface *gpu_atlas = SDL_ConvertSurface(atlas, SDL_PIXELFORMAT_ABGR8888);
	SDL_DestroySurface(atlas);

	if (gpu_atlas == nullptr || !upload_mesh_data(font) || !upload_atlas(font, gpu_atlas))
	{
		SDL_DestroySurface(gpu_atlas);
		return false;
	}

	SDL_DestroySurface(gpu_atlas);

	const Uint64 end = SDL_GetTicks();
	SDL_LogDebug(LOG_CATEGORY_FONT, "Baked font in %lld ms", end - start);

	return true;
}

font_t *font_create(SDL_GPUDevice *device, SDL_IOStream *source, const SDL_Color color)
{
	font_t *font = SDL_malloc(sizeof(font_t));
	if (font == nullptr)
	{
		return nullptr;
	}

	font->device = device;
	font->color = color;

	const Uint8 *font_data = SDL_LoadFile_IO(source, nullptr, true);
	if (font_data == nullptr)
	{
		SDL_free(font);
		return nullptr;
	}

	if (!font_bake(font, font_data))
	{
		SDL_free(font);
		return nullptr;
	}

	return font;
}

void font_draw_text(const font_t *font, SDL_GPURenderPass *render_pass,
	const vector2f_t position, const float text_size, const char *text)
{
	auto offset_x = 0.F;
	auto offset_y = 0.F;
	const float scale = text_size / (float) font_size;

	const size_t text_length = SDL_strlen(text);
	for (size_t i = 0; i < text_length; i++)
	{
		const auto codepoint = (int) text[i];
		const int index = codepoint - ascii_begin;

		const glyph_info_t *glyph = font->glyphs + index;
		const SDL_Rect *rect = &glyph->rect;

		if (codepoint == '\n')
		{
			constexpr auto line_spacing = 2.F;

			offset_x = 0.F;
			offset_y += font_size + line_spacing;
			continue;
		}

		SDL_FRect src = {
			.x = (float) rect->x - (float) glyph_padding,
			.y = (float) rect->y - (float) glyph_padding,
			.w = ((float) rect->w + ((float) glyph_padding * 2.F)),
			.h = ((float) rect->h + ((float) glyph_padding * 2.F)),
		};
		SDL_FRect dst = {
			.x = position.x + offset_x + ((float) glyph->offset_x * scale),
			.y = position.y + offset_y + ((float) glyph->offset_y * scale),
			.w = ((float) rect->w + ((float) glyph_padding * 2.F)) * scale,
			.h = ((float) rect->h + ((float) glyph_padding * 2.F)) * scale,
		};

		const SDL_GPUBufferBinding vertex_binding = {
			.buffer = font->vertex_buffer,
			.offset = 0,
		};
		SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

		const SDL_GPUBufferBinding index_binding = {
			.buffer = font->index_buffer,
			.offset = 0,
		};
		SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

		const SDL_GPUTextureSamplerBinding binding = {
			.texture = font->texture,
			.sampler = font->sampler,
		};
		SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);

		SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);

		const int width = glyph->advance_x == 0 ? rect->w : glyph->advance_x;
		offset_x += (float) width * scale;
	}
}

void font_destroy(font_t *font)
{
	if (font == nullptr)
	{
		return;
	}

	SDL_ReleaseGPUTexture(font->device, font->texture);
	SDL_ReleaseGPUSampler(font->device, font->sampler);
	SDL_ReleaseGPUBuffer(font->device, font->vertex_buffer);
	SDL_ReleaseGPUBuffer(font->device, font->index_buffer);

	SDL_free(font);
}
