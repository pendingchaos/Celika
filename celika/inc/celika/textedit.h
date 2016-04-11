#pragma once
#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "aabb.h"
#include "font.h"

#include <SDL2/SDL.h>

typedef struct textedit_t textedit_t;

typedef struct textedit_draw_callbacks_t {
    void* userdata;
    void (*draw_cur)(float left, size_t line, void* userdata);
    void (*draw_insert)(float left, float width, size_t line, void* userdata);
    void (*draw_sel)(float left, float width, size_t line, void* userdata);
    void (*draw_text)(uint32_t* codepoints, size_t line, void* userdata);
} textedit_draw_callbacks_t;

textedit_t* create_textedit(font_t* font, size_t height, bool single_line);
void del_textedit(textedit_t* edit);
void textedit_event(textedit_t* edit, float left, float bottom, SDL_Event event);
void textedit_draw(textedit_t* edit, textedit_draw_callbacks_t callbacks);
const uint32_t* textedit_get(textedit_t* edit);
size_t textedit_get_cursor(textedit_t* edit);
bool textedit_is_insert(textedit_t* edit);
bool textedit_get_selection(textedit_t* edit, size_t* start, size_t* end);
aabb_t textedit_calc_aabb(textedit_t* edit, float left, float bottom);
#endif
