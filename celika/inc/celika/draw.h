#pragma once
#ifndef DRAW_H
#define DRAW_H

#include "aabb.h"

#include <stdint.h>
#include <stddef.h>

typedef struct draw_tex_t draw_tex_t;
typedef struct draw_effect_t draw_effect_t;
typedef struct draw_fb_t draw_fb_t;

typedef struct draw_col_t {
    union {
        struct {float r, g, b, a;};
        float rgba[4];
    };
} draw_col_t;

void draw_init();
void draw_deinit();
draw_tex_t* draw_create_tex(const char* filename, int* w, int* h);
draw_tex_t* draw_create_tex_data(uint8_t* data, size_t w, size_t h, bool filtering);
draw_tex_t* draw_create_scaled_tex(const char* filename, int reqw, int reqh, int* w, int* h);
draw_tex_t* draw_create_scaled_tex_aabb(const char* filename, int reqw, int reqh, aabb_t* aabb);
void draw_del_tex(draw_tex_t* tex);
aabb_t draw_get_tex_aabb(draw_tex_t* tex);
draw_effect_t* draw_create_effect(const char* shdr_fname);
void draw_del_effect(draw_effect_t* effect);
void draw_set_tex(draw_tex_t* tex);
draw_tex_t* draw_get_tex();
void draw_begin(size_t width, size_t height);
void draw_end();
void draw_set_orientation(float degrees, float ox, float oy);
void draw_set_scale(float x, float y, float ox, float oy);
void draw_add_tri(const float* pos, const draw_col_t* col, const float* uv);
void draw_add_quad(const float* pos, const draw_col_t* col, const float* uv); //vertices: {bl, br, tr, tl}
void draw_add_rect(const float* bl, const float* size, draw_col_t col);
void draw_add_aabb(aabb_t aabb, draw_col_t col);
void draw_prims();
draw_fb_t* draw_prims_fb();
void draw_effect_param_float(draw_effect_t* effect, const char* name, float val);
void draw_effect_param_fb(draw_effect_t* effect, const char* name, draw_fb_t* val);
void draw_do_effect(draw_effect_t* effect);
draw_fb_t* draw_do_effect_fb(draw_effect_t* effect);
void draw_free_fb(draw_fb_t* res);
void draw_text(const char* text, const float* pos, draw_col_t col, float height);
float draw_text_width(const char* text, float height);
draw_col_t draw_rgb(float r, float g, float b);
draw_col_t draw_rgba(float r, float g, float b, float a);
#endif
