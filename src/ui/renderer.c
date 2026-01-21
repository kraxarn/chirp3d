#include "ui/renderer.h"
#include "atlas.inl"

#include "microui.h"

#include <stddef.h>

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

/*
 * Based of:
 * https://github.com/microui-community/microui/blob/master/demo/renderer.c
 */

static constexpr size_t buffer_size = 16'384;

static GLfloat tex_buf[buffer_size * 8];
static GLfloat vert_buf[buffer_size * 8];
static GLubyte color_buf[buffer_size * 16];
static GLuint index_buf[buffer_size * 6];

static int width = 800;
static int height = 600;
static int buf_idx;

static SDL_Window *window;

static constexpr char button_map[256] = {
	[ SDL_BUTTON_LEFT & 0xff ]   = MU_MOUSE_LEFT,
	[ SDL_BUTTON_RIGHT & 0xff ]  = MU_MOUSE_RIGHT,
	[ SDL_BUTTON_MIDDLE & 0xff ] = MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
	[ SDL_SCANCODE_LSHIFT ]    = MU_KEY_SHIFT,
	[ SDL_SCANCODE_RSHIFT ]    = MU_KEY_SHIFT,
	[ SDL_SCANCODE_LCTRL ]     = MU_KEY_CTRL,
	[ SDL_SCANCODE_RCTRL ]     = MU_KEY_CTRL,
	[ SDL_SCANCODE_LALT ]      = MU_KEY_ALT,
	[ SDL_SCANCODE_RALT ]      = MU_KEY_ALT,
	[ SDL_SCANCODE_RETURN ]    = MU_KEY_RETURN,
	[ SDL_SCANCODE_BACKSPACE ] = MU_KEY_BACKSPACE,
};

void r_init()
{
	SDL_Init(SDL_INIT_VIDEO);

	// init SDL window
	window = SDL_CreateWindow(nullptr,
		width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	SDL_GL_CreateContext(window);

	SDL_StartTextInput(window);

	// init gl
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	// init texture
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
		GL_ALPHA, GL_UNSIGNED_BYTE, atlas_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	SDL_assert(glGetError() == 0);
}

static void flush(void)
{
	if (buf_idx == 0) { return; }

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glTexCoordPointer(2, GL_FLOAT, 0, tex_buf);
	glVertexPointer(2, GL_FLOAT, 0, vert_buf);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, color_buf);
	glDrawElements(GL_TRIANGLES, buf_idx * 6, GL_UNSIGNED_INT, index_buf);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	buf_idx = 0;
}

static void push_quad(const mu_Rect dst, const mu_Rect src, const mu_Color color)
{
	if (buf_idx == buffer_size)
	{
		flush();
	}

	const int texvert_idx = buf_idx * 8;
	const int color_idx = buf_idx * 16;
	const int element_idx = buf_idx * 4;
	const int index_idx = buf_idx * 6;
	buf_idx++;

	// update texture buffer
	const float x = (float) src.x / (float) ATLAS_WIDTH;
	const float y = (float) src.y / (float) ATLAS_HEIGHT;
	const float w = (float) src.w / (float) ATLAS_WIDTH;
	const float h = (float) src.h / (float) ATLAS_HEIGHT;
	tex_buf[texvert_idx + 0] = x;
	tex_buf[texvert_idx + 1] = y;
	tex_buf[texvert_idx + 2] = x + w;
	tex_buf[texvert_idx + 3] = y;
	tex_buf[texvert_idx + 4] = x;
	tex_buf[texvert_idx + 5] = y + h;
	tex_buf[texvert_idx + 6] = x + w;
	tex_buf[texvert_idx + 7] = y + h;

	// update vertex buffer
	vert_buf[texvert_idx + 0] = dst.x;
	vert_buf[texvert_idx + 1] = dst.y;
	vert_buf[texvert_idx + 2] = dst.x + dst.w;
	vert_buf[texvert_idx + 3] = dst.y;
	vert_buf[texvert_idx + 4] = dst.x;
	vert_buf[texvert_idx + 5] = dst.y + dst.h;
	vert_buf[texvert_idx + 6] = dst.x + dst.w;
	vert_buf[texvert_idx + 7] = dst.y + dst.h;

	/* update color buffer */
	SDL_memcpy(color_buf + color_idx + 0, &color, 4);
	SDL_memcpy(color_buf + color_idx + 4, &color, 4);
	SDL_memcpy(color_buf + color_idx + 8, &color, 4);
	SDL_memcpy(color_buf + color_idx + 12, &color, 4);

	/* update index buffer */
	index_buf[index_idx + 0] = element_idx + 0;
	index_buf[index_idx + 1] = element_idx + 1;
	index_buf[index_idx + 2] = element_idx + 2;
	index_buf[index_idx + 3] = element_idx + 2;
	index_buf[index_idx + 4] = element_idx + 3;
	index_buf[index_idx + 5] = element_idx + 1;
}

void r_draw_rect(const mu_Rect rect, const mu_Color color)
{
	push_quad(rect, atlas[ATLAS_WHITE], color);
}

void r_draw_text(const char *text, const mu_Vec2 pos, const mu_Color color)
{
	mu_Rect dst = {
		.x = pos.x,
		.y = pos.y,
		.w = 0,
		.h = 0,
	};

	for (const char *p = text; *p; p++)
	{
		if ((*p & 0xc0) == 0x80)
		{
			continue;
		}

		const int chr = mu_min((unsigned char) *p, 127);
		const mu_Rect src = atlas[ATLAS_FONT + chr];
		dst.w = src.w;
		dst.h = src.h;
		push_quad(dst, src, color);
		dst.x += dst.w;
	}
}

void r_draw_icon(const int id, const mu_Rect rect, const mu_Color color)
{
	mu_Rect src = atlas[id];
	int x = rect.x + ((rect.w - src.w) / 2);
	int y = rect.y + ((rect.h - src.h) / 2);
	push_quad(mu_rect(x, y, src.w, src.h), src, color);
}

int r_get_event_key_modifier(const SDL_KeyboardEvent event)
{
	return key_map[event.scancode];
}

int r_get_button_modifier(const SDL_MouseButtonEvent event)
{
	return button_map[event.button];
}

int r_get_text_width(const char *text, int len)
{
	int res = 0;
	for (const char *p = text; *p && len--; p++)
	{
		if ((*p & 0xc0) == 0x80)
		{
			continue;
		}

		const int chr = mu_min((unsigned char) *p, 127);
		res += atlas[ATLAS_FONT + chr].w;
	}
	return res;
}

int r_get_text_height()
{
	return 18;
}

void r_set_clip_rect(const mu_Rect rect)
{
	flush();
	glScissor(rect.x, height - (rect.y + rect.h), rect.w, rect.h);
}

void r_clear(const mu_Color color)
{
	flush();
	glClearColor(
		(GLclampf) color.r / 255.F,
		(GLclampf) color.g / 255.F,
		(GLclampf) color.b / 255.F,
		(GLclampf) color.a / 255.F
	);
	glClear(GL_COLOR_BUFFER_BIT);
}

void r_present()
{
	flush();
	SDL_GL_SwapWindow(window);
}

void r_handle_events(mu_Context *ctx, bool *running)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				*running = false;
				break;

			case SDL_EVENT_MOUSE_MOTION:
				mu_input_mousemove(ctx, event.motion.x, event.motion.y);
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				mu_input_scroll(ctx, 0, event.wheel.y * -30);
				break;

			case SDL_EVENT_TEXT_INPUT:
				mu_input_text(ctx, event.text.text);
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
				const int b = r_get_button_modifier(event.button);

				if (b && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
				{
					mu_input_mousedown(ctx, event.button.x, event.button.y, b);
				}

				if (b && event.type == SDL_EVENT_MOUSE_BUTTON_UP)
				{
					mu_input_mouseup(ctx, event.button.x, event.button.y, b);
				}

				break;

			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				const int c = r_get_event_key_modifier(event.key);

				if (c && event.type == SDL_EVENT_KEY_DOWN)
				{
					mu_input_keydown(ctx, c);
				}

				if (c && event.type == SDL_EVENT_KEY_UP)
				{
					mu_input_keyup(ctx, c);
				}

				break;

			case SDL_EVENT_WINDOW_RESIZED:
				width = event.window.data1;
				height = event.window.data2;
				break;
		}
	}
}
