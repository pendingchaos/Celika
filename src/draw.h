#pragma once
#ifndef DRAW_H
#define DRAW_H

#include <stddef.h>

void draw_init();
void draw_deinit();
int draw_create_tex(const char* filename, int* w, int* h);
void draw_del_tex(int tex);
void draw_set_tex(int tex);
void draw_begin(size_t width, size_t height);
void draw_end();
void draw_clear(float* col);
void draw_set_orientation(float degrees, float ox, float oy);
void draw_add_tri(float* pos, float* col, float* uv);
void draw_add_quad(float* pos, float* col, float* uv); //vertices: {bl, br, tr, tl}
void draw_add_rect(float* bl, float* size, float* col);
void draw_prims();
void draw_text(const char* text, float* pos, float* col, float height);
#endif
