#pragma once

#include "microui.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>

mu_Context *r_init();

void r_draw_rect(mu_Rect rect, mu_Color color);

void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color);

void r_draw_icon(int id, mu_Rect rect, mu_Color color);

[[nodiscard]]
int r_get_event_key_modifier(SDL_KeyboardEvent event);

[[nodiscard]]
int r_get_button_modifier(SDL_MouseButtonEvent event);

int r_get_text_width(const char *text, int len);

int r_get_text_height();

void r_set_clip_rect(mu_Rect rect);

void r_clear(mu_Color color);

void r_draw();

void r_present();

SDL_AppResult r_handle_event(const SDL_Event *event);
