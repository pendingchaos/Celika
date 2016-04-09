#pragma once
#ifndef FONT_H
#define FONT_H

#include "draw.h"

#include <stdint.h>

typedef struct font_t font_t;

void font_init();
void font_deinit();
font_t* create_font(const char* filename);
void del_font(font_t* font);
void draw_text_font(font_t* font, const uint32_t* text, const float* pos,
                    draw_col_t col, size_t height);
float draw_text_font_width(font_t* font, const uint32_t* text, size_t height);
#endif
