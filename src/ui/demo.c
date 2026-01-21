#include "ui/renderer.h"

#include "microui.h"

#include <stddef.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

/*
 * Based of:
 * https://github.com/microui-community/microui/blob/master/demo/main.c
 */

static constexpr size_t logbuf_len = 64'000;
static char logbuf[logbuf_len];
static int logbuf_updated = 0;
static float bg[3] = {90, 95, 100};

static void write_log(const char *text)
{
	if (logbuf[0])
	{
		SDL_strlcat(logbuf, "\n", logbuf_len);
	}

	SDL_strlcat(logbuf, text, logbuf_len);
	logbuf_updated = 1;
}

static void test_window(mu_Context *ctx)
{
	// do window
	if (mu_begin_window(ctx, "Demo Window", mu_rect(40, 40, 300, 450)))
	{
		mu_Container *win = mu_get_current_container(ctx);
		win->rect.w = mu_max(win->rect.w, 240);
		win->rect.h = mu_max(win->rect.h, 300);

		// window info
		if (mu_header(ctx, "Window Info"))
		{
			win = mu_get_current_container(ctx);
			char buf[64];
			mu_layout_row(ctx, 2, (int[]){54, -1}, 0);
			mu_label(ctx, "Position:");
			SDL_snprintf(buf, 64, "%d, %d", win->rect.x, win->rect.y);
			mu_label(ctx, buf);
			mu_label(ctx, "Size:");
			SDL_snprintf(buf, 64, "%d, %d", win->rect.w, win->rect.h);
			mu_label(ctx, buf);
		}

		// labels + buttons
		if (mu_header_ex(ctx, "Test Buttons", MU_OPT_EXPANDED))
		{
			mu_layout_row(ctx, 3, (int[]){86, -110, -1}, 0);
			mu_label(ctx, "Test buttons 1:");

			if (mu_button(ctx, "Button 1"))
			{
				write_log("Pressed button 1");
			}
			if (mu_button(ctx, "Button 2"))
			{
				write_log("Pressed button 2");
			}

			mu_label(ctx, "Test buttons 2:");

			if (mu_button(ctx, "Button 3"))
			{
				write_log("Pressed button 3");
			}

			if (mu_button(ctx, "Popup"))
			{
				mu_open_popup(ctx, "Test Popup");
			}

			if (mu_begin_popup(ctx, "Test Popup"))
			{
				mu_button(ctx, "Hello");
				mu_button(ctx, "World");
				mu_end_popup(ctx);
			}
		}

		// tree
		if (mu_header_ex(ctx, "Tree and Text", MU_OPT_EXPANDED))
		{
			mu_layout_row(ctx, 2, (int[]){140, -1}, 0);
			mu_layout_begin_column(ctx);
			if (mu_begin_treenode(ctx, "Test 1"))
			{
				if (mu_begin_treenode(ctx, "Test 1a"))
				{
					mu_label(ctx, "Hello");
					mu_label(ctx, "world");
					mu_end_treenode(ctx);
				}

				if (mu_begin_treenode(ctx, "Test 1b"))
				{
					if (mu_button(ctx, "Button 1"))
					{
						write_log("Pressed button 1");
					}

					if (mu_button(ctx, "Button 2"))
					{
						write_log("Pressed button 2");
					}

					mu_end_treenode(ctx);
				}

				mu_end_treenode(ctx);
			}

			if (mu_begin_treenode(ctx, "Test 2"))
			{
				mu_layout_row(ctx, 2, (int[]){54, 54}, 0);

				if (mu_button(ctx, "Button 3"))
				{
					write_log("Pressed button 3");
				}

				if (mu_button(ctx, "Button 4"))
				{
					write_log("Pressed button 4");
				}

				if (mu_button(ctx, "Button 5"))
				{
					write_log("Pressed button 5");
				}

				if (mu_button(ctx, "Button 6"))
				{
					write_log("Pressed button 6");
				}

				mu_end_treenode(ctx);
			}

			if (mu_begin_treenode(ctx, "Test 3"))
			{
				static int checks[3] = {1, 0, 1};
				mu_checkbox(ctx, "Checkbox 1", &checks[0]);
				mu_checkbox(ctx, "Checkbox 2", &checks[1]);
				mu_checkbox(ctx, "Checkbox 3", &checks[2]);
				mu_end_treenode(ctx);
			}

			mu_layout_end_column(ctx);

			mu_layout_begin_column(ctx);

			mu_layout_row(ctx, 1, (int[]){-1}, 0);
			mu_text(ctx, "Lorem ipsum dolor sit amet, consectetur adipiscing "
				"elit. Maecenas lacinia, sem eu lacinia molestie, mi risus faucibus "
				"ipsum, eu varius magna felis a nulla.");

			mu_layout_end_column(ctx);
		}

		// background color sliders
		if (mu_header_ex(ctx, "Background Color", MU_OPT_EXPANDED))
		{
			mu_layout_row(ctx, 2, (int[]){-78, -1}, 74);

			// sliders
			static int idx = 0;
			mu_layout_begin_column(ctx);
			mu_layout_row(ctx, 2, (int[]){46, -1}, 0);
			mu_label(ctx, "Red:");
			mu_slider(ctx, &bg[0], &idx, 0, 255);
			mu_label(ctx, "Green:");
			mu_slider(ctx, &bg[1], &idx, 0, 255);
			mu_label(ctx, "Blue:");
			mu_slider(ctx, &bg[2], &idx, 0, 255);
			mu_layout_end_column(ctx);

			// color preview
			mu_Rect r = mu_layout_next(ctx);
			mu_draw_rect(ctx, r, mu_color(bg[0], bg[1], bg[2], 255));
			char buf[32];
			SDL_snprintf(buf, 32, "#%02X%02X%02X", (int) bg[0], (int) bg[1], (int) bg[2]);
			mu_draw_control_text(ctx, buf, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
		}

		mu_end_window(ctx);
	}
}

static void log_window(mu_Context *ctx)
{
	if (mu_begin_window(ctx, "Log Window", mu_rect(350, 40, 300, 200)))
	{
		// output text panel
		mu_layout_row(ctx, 1, (int[]){-1}, -25);
		mu_begin_panel(ctx, "Log Output");
		mu_Container *panel = mu_get_current_container(ctx);
		mu_layout_row(ctx, 1, (int[]){-1}, -1);
		mu_text(ctx, logbuf);
		mu_end_panel(ctx);
		if (logbuf_updated)
		{
			panel->scroll.y = panel->content_size.y;
			logbuf_updated = 0;
		}

		// input textbox + submit button
		static char buf[128];
		static int idx = 0;
		int submitted = 0;
		mu_layout_row(ctx, 2, (int[]){-70, -1}, 0);

		if (mu_textbox(ctx, buf, sizeof(buf), &idx) & MU_RES_SUBMIT)
		{
			mu_set_focus(ctx, ctx->last_id);
			submitted = 1;
		}

		if (mu_button(ctx, "Submit"))
		{
			submitted = 1;
		}

		if (submitted)
		{
			write_log(buf);
			buf[0] = '\0';
		}

		mu_end_window(ctx);
	}
}

static int uint8_slider(mu_Context *ctx, unsigned char *value,
	const int low, const int high)
{
	static float tmp;
	static int idx = 0;
	mu_push_id(ctx, (void *) &value, sizeof(value));
	tmp = *value;
	const int res = mu_slider_ex(ctx, &tmp, &idx, (mu_Real) low, (mu_Real) high,
		0, "%.0f", MU_OPT_ALIGNCENTER);
	*value = (unsigned char) tmp;
	mu_pop_id(ctx);
	return res;
}

static void style_window(mu_Context *ctx)
{
	static struct
	{
		const char *label;
		int idx;
	} colors[] = {
		{.label = "text:", .idx = MU_COLOR_TEXT},
		{.label = "border:", .idx = MU_COLOR_BORDER},
		{.label = "windowbg:", .idx = MU_COLOR_WINDOWBG},
		{.label = "titlebg:", .idx = MU_COLOR_TITLEBG},
		{.label = "titletext:", .idx = MU_COLOR_TITLETEXT},
		{.label = "panelbg:", .idx = MU_COLOR_PANELBG},
		{.label = "button:", .idx = MU_COLOR_BUTTON},
		{.label = "buttonhover:", .idx = MU_COLOR_BUTTONHOVER},
		{.label = "buttonfocus:", .idx = MU_COLOR_BUTTONFOCUS},
		{.label = "base:", .idx = MU_COLOR_BASE},
		{.label = "basehover:", .idx = MU_COLOR_BASEHOVER},
		{.label = "basefocus:", .idx = MU_COLOR_BASEFOCUS},
		{.label = "scrollbase:", .idx = MU_COLOR_SCROLLBASE},
		{.label = "scrollthumb:", .idx = MU_COLOR_SCROLLTHUMB},
		{.label = nullptr}
	};

	if (mu_begin_window(ctx, "Style Editor", mu_rect(350, 250, 300, 240)))
	{
		const auto sw = (int) ((float) mu_get_current_container(ctx)->body.w * 0.14F);
		mu_layout_row(ctx, 6, (int[]){80, sw, sw, sw, sw, -1}, 0);

		for (int i = 0; colors[i].label; i++)
		{
			mu_label(ctx, colors[i].label);
			uint8_slider(ctx, &ctx->style->colors[i].r, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].g, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].b, 0, 255);
			uint8_slider(ctx, &ctx->style->colors[i].a, 0, 255);
			mu_draw_rect(ctx, mu_layout_next(ctx), ctx->style->colors[i]);
		}

		mu_end_window(ctx);
	}
}

static void process_frame(mu_Context *ctx)
{
	mu_begin(ctx);
	style_window(ctx);
	log_window(ctx);
	test_window(ctx);
	mu_end(ctx);
}

SDL_AppResult SDL_AppInit(void **appstate,
	[[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
	*appstate = r_init();
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	process_frame((mu_Context *) appstate);

	r_clear(mu_color(bg[0], bg[1], bg[2], 255));
	mu_Command *cmd = nullptr;

	while (mu_next_command((mu_Context *) appstate, &cmd))
	{
		switch (cmd->type)
		{
			case MU_COMMAND_TEXT:
				r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color);
				break;

			case MU_COMMAND_RECT:
				r_draw_rect(cmd->rect.rect, cmd->rect.color);
				break;

			case MU_COMMAND_ICON:
				r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
				break;

			case MU_COMMAND_CLIP:
				r_set_clip_rect(cmd->clip.rect);
				break;
		}
	}

	r_present();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event)
{
	return r_handle_event(event);
}

void SDL_AppQuit([[maybe_unused]] void *appstate,
	[[maybe_unused]] SDL_AppResult result)
{
}
