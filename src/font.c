#include "font.h"
#include "gpu.h"
#include "logcategory.h"
#include "matrix.h"
#include "meshinfo.h"
#include "uniformdata.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
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

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// ASCII printable characters
static constexpr auto ascii_begin = 32;
static constexpr auto ascii_size = 95;

// TODO: These should be dynamic
static constexpr auto atlas_width = 1024;
static constexpr auto atlas_height = 1024;

static constexpr size_t num_vertices = 4;
static constexpr size_t num_indices = 6;

typedef struct glyph_info_t
{
	vector2i_t offset;
	int advance_x;
	vector2i_t size;
} glyph_info_t;

typedef struct font_t
{
	Uint16 size;
	SDL_Color color;
	glyph_info_t glyphs[ascii_size];
	vector2f_aligned_t uv_coordinates[num_vertices * ascii_size];

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
	const auto font_size = (float) font->size;

	const vertex_t vertices[] = {
		(vertex_t){
			.position = (vector3f_t){.x = -font_size, .y = font_size, .z = 0},
			.tex_coord = (vector2f_t){.x = 0, .y = 1}
		},
		(vertex_t){
			.position = (vector3f_t){.x = font_size, .y = font_size, .z = 0},
			.tex_coord = (vector2f_t){.x = 1, .y = 1}
		},
		(vertex_t){
			.position = (vector3f_t){.x = font_size, .y = -font_size, .z = 0},
			.tex_coord = (vector2f_t){1, 0}
		},
		(vertex_t){
			.position = (vector3f_t){.x = -font_size, .y = -font_size, .z = 0},
			.tex_coord = (vector2f_t){0, 0}
		},
	};

	const mesh_index_t indices[] = {
		0, 1, 2,
		0, 2, 3,
	};

	const mesh_info_t info = {
		.num_vertices = SDL_arraysize(vertices),
		.vertices = vertices,
		.num_indices = SDL_arraysize(indices),
		.indices = indices,
	};

	return gpu_upload_mesh_info(font->device, info, &font->vertex_buffer, &font->index_buffer);
}

static bool upload_atlas(font_t *font, const SDL_Surface *atlas)
{
	const SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};

	return gpu_upload_texture(font->device, atlas, &sampler_info,
		&font->sampler, &font->texture);
}

static void build_atlas_data(const float font_size, vector2f_aligned_t *coordinates)
{
	for (auto i = 0; i < ascii_size; i++)
	{
		// TODO: Should probably always be power of 2
		const int atlas_tile_size = atlas_width / (int) font_size;

		const vector2f_t top_left = {
			.x = ((float) (i % atlas_tile_size) * font_size) / (float) atlas_width,
			.y = ((float) (i / atlas_tile_size) * font_size) / (float) atlas_height,
		};

		const vector2f_t bottom_right = {
			.x = top_left.x + ((float) font_size / (float) atlas_width),
			.y = top_left.y + ((float) font_size / (float) atlas_height),
		};

		const size_t idx = i * num_vertices;
		coordinates[idx + 0] = (vector2f_aligned_t){
			.x = top_left.x,
			.y = bottom_right.y,
		};
		coordinates[idx + 1] = (vector2f_aligned_t){
			.x = bottom_right.x,
			.y = bottom_right.y,
		};
		coordinates[idx + 2] = (vector2f_aligned_t){
			.x = bottom_right.x,
			.y = top_left.y,
		};
		coordinates[idx + 3] = (vector2f_aligned_t){
			.x = top_left.x,
			.y = top_left.y,
		};
	}
}

static bool font_bake(font_t *font, const Uint8 *data)
{
	const Uint64 start = SDL_GetTicks();

	stbtt_fontinfo font_info;
	if (!stbtt_InitFont(&font_info, data, 0))
	{
		return false;
	}

	const float scale = stbtt_ScaleForPixelHeight(&font_info, font->size);

	int ascent;
	int descent;
	int line_gap;
	stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

	SDL_Surface *atlas = SDL_CreateSurface(atlas_width, atlas_height, SDL_PIXELFORMAT_INDEX8);
	build_palette(atlas, font->color);

	for (auto i = 0; i < ascii_size; i++)
	{
		const int codepoint = ascii_begin + i;
		glyph_info_t *glyph = font->glyphs + i;

		const int index = stbtt_FindGlyphIndex(&font_info, codepoint);
		if (index <= 0)
		{
			SDL_DestroySurface(atlas);
			return SDL_SetError("Invalid codepoint: %d", codepoint);
		}

		stbtt_GetGlyphBitmapBox(&font_info, index, scale, scale,
			&glyph->offset.x, &glyph->offset.y, nullptr, nullptr
		);

		stbtt_GetCodepointHMetrics(&font_info, codepoint, &glyph->advance_x, nullptr);
		glyph->advance_x = (int) ((float) glyph->advance_x * scale);
		glyph->offset.y += (int) ((float) ascent * scale);

		if (codepoint == ' ')
		{
			glyph->size = (vector2i_t){
				.x = glyph->advance_x,
				.y = font->size,
			};
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

			glyph->size = (vector2i_t){
				.x = box_x1 - box_x0,
				.y = box_y1 - box_y0
			};
		}

		if (glyph->size.x > font->size || glyph->size.y > font->size)
		{
			SDL_DestroySurface(atlas);
			return SDL_SetError("Glyph %d too large: %dx%d", codepoint, glyph->size.x, glyph->size.y);
		}

		SDL_Surface *surface = SDL_CreateSurface(glyph->size.x, glyph->size.y, SDL_PIXELFORMAT_INDEX8);
		if (surface == nullptr || !build_palette(surface, font->color))
		{
			SDL_DestroySurface(surface);
			SDL_DestroySurface(atlas);
			return false;
		}

		stbtt_MakeCodepointBitmap(&font_info,
			surface->pixels, glyph->size.x, glyph->size.y,
			glyph->size.x, scale, scale, codepoint
		);

		const int atlas_tile_size = atlas_width / (int) font->size;

		vector2i_t src;
		const vector2i_t dst = {
			.x = (i % atlas_tile_size) * font->size,
			.y = ((i / atlas_tile_size) * font->size),
		};

		for (src.y = 0; src.y < surface->h; src.y++)
		{
			for (src.x = 0; src.x < surface->w; src.x++)
			{
				const size_t dst_idx = ((dst.y + src.y) * atlas->w) + (dst.x + src.x);
				const size_t src_idx = (src.y * surface->w) + src.x;
				((Uint8 *) atlas->pixels)[dst_idx] = ((Uint8 *) surface->pixels)[src_idx];
			}
		}

		SDL_DestroySurface(surface);
	}

	SDL_Surface *gpu_atlas = SDL_ConvertSurface(atlas, SDL_PIXELFORMAT_ABGR8888);
	SDL_DestroySurface(atlas);

	if (gpu_atlas == nullptr)
	{
		return false;
	}

	if (!upload_mesh_data(font) || !upload_atlas(font, gpu_atlas))
	{
		SDL_DestroySurface(gpu_atlas);
		return false;
	}

	build_atlas_data(font->size, font->uv_coordinates);

	const Uint64 end = SDL_GetTicks();
	SDL_LogDebug(LOG_CATEGORY_FONT, "Baked font in %lu ms", end - start);

	return true;
}

static float display_scale(SDL_Window *window)
{
	const float scale = SDL_GetWindowDisplayScale(window);
	if (scale > 0.F)
	{
		return scale;
	}

	SDL_LogWarn(LOG_CATEGORY_FONT, "Failed to get display scale: %s", SDL_GetError());
	return 1.F;
}

font_t *font_create(SDL_Window *window, SDL_GPUDevice *device,
	SDL_IOStream *source, const Uint16 font_size, const SDL_Color color)
{
	font_t *font = SDL_malloc(sizeof(font_t));
	if (font == nullptr)
	{
		return nullptr;
	}

	// TODO: We don't necessarily always have the same scale
	font->size = (Uint16) ((float) font_size * display_scale(window));

	// TODO: Failsafe until we have dynamic atlas sizing
	constexpr Uint16 max_font_size = 100;
	if (font->size > max_font_size)
	{
		SDL_SetError("Font size too large (%u > %u)", font->size, max_font_size);
		SDL_free(font);
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

void font_draw_text(const font_t *font, SDL_GPURenderPass *render_pass, SDL_GPUCommandBuffer *command_buffer,
	const vector2f_t swapchain_size, const vector2f_t position, const char *text)
{
	auto offset_x = 0.F;
	auto offset_y = 0.F;

	const matrix4x4_t view = matrix4x4_create_orthographic(
		0.F, swapchain_size.x, swapchain_size.y, 0.F,
		0.F, -1.F
	);

	const size_t text_length = SDL_strlen(text);
	for (size_t i = 0; i < text_length; i++)
	{
		const auto codepoint = (int) text[i];
		const int index = codepoint - ascii_begin;

		const glyph_info_t *glyph = font->glyphs + index;

		if (codepoint == '\n')
		{
			constexpr auto line_spacing = 2.F;

			offset_x = 0.F;
			offset_y += (float) font->size + line_spacing;
			continue;
		}

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

		const matrix4x4_t m_scale = matrix4x4_create_scale((vector3f_t){
			.x = 0.5F,
			.y = 0.5F,
			.z = 1.F,
		});
		const matrix4x4_t m_pos = matrix4x4_create_translation((vector3f_t){
			.x = position.x + ((float) font->size / 2.F) + offset_x + (float) glyph->offset.x,
			.y = position.y + ((float) font->size / 2.F) + offset_y + (float) glyph->offset.y,
			.z = 0.F,
		});

		vertex_uniform_data_t vertex_data = {
			.mvp = matrix4x4_multiply(matrix4x4_multiply(m_scale, m_pos), view),
		};
		SDL_memcpy(
			vertex_data.tex_uv,
			font->uv_coordinates + (size_t) ((codepoint - ascii_begin) * (int) num_vertices),
			sizeof(vector2f_aligned_t) * 4
		);

		SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_data, sizeof(vertex_uniform_data_t));

		SDL_DrawGPUIndexedPrimitives(render_pass, num_indices, 1, 0, 0, 0);

		const int width = glyph->advance_x == 0 ? glyph->size.x : glyph->advance_x;
		offset_x += (float) width;
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
