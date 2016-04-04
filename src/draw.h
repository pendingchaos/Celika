#pragma once
#ifndef DRAW_H
#define DRAW_H

#include "aabb.h"

#include <stddef.h>

typedef struct draw_tex_t draw_tex_t;

typedef struct draw_col_t {
    union {
        struct {float r, g, b, a;};
        float rgba[4];
    };
} draw_col_t;

void draw_init();
void draw_deinit();
draw_tex_t* draw_create_tex(const char* filename, int* w, int* h);
draw_tex_t* draw_create_scaled_tex(const char* filename, int reqw, int reqh, int* w, int* h);
draw_tex_t* draw_create_scaled_tex_aabb(const char* filename, int reqw, int reqh, aabb_t* aabb);
void draw_del_tex(draw_tex_t* tex);
aabb_t draw_get_tex_aabb(draw_tex_t* tex);
void draw_set_tex(draw_tex_t* tex);
void draw_begin(size_t width, size_t height);
void draw_end();
void draw_clear(draw_col_t col);
void draw_set_orientation(float degrees, float ox, float oy);
void draw_set_scale(float x, float y, float ox, float oy);
void draw_add_tri(float* pos, draw_col_t* col, float* uv);
void draw_add_quad(float* pos, draw_col_t* col, float* uv); //vertices: {bl, br, tr, tl}
void draw_add_rect(float* bl, float* size, draw_col_t col);
void draw_add_aabb(aabb_t aabb, draw_col_t col);
void draw_prims();
void draw_text(const char* text, float* pos, draw_col_t col, float height);
draw_col_t draw_rgb(float r, float g, float b);
draw_col_t draw_rgba(float r, float g, float b, float a);
#endif
