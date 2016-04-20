#pragma once
#ifndef DRAW_H
#define DRAW_H

#include "aabb.h"

#include <stdint.h>
#include <stddef.h>

typedef enum blend_t {
    BLEND_NONE,
    BLEND_ALPHA,
    BLEND_ADD,
    BLEND_SUB,
    BLEND_MULT
} blend_t;

typedef enum tex_flags_t {
    TEX_MIN_NEAREST = 1 << 0,
    TEX_MAG_NEAREST = 1 << 1
} tex_flags_t;

typedef struct draw_tex_t draw_tex_t;
typedef struct draw_effect_t draw_effect_t;

typedef struct draw_col_t {
    union {
        struct {float r, g, b, a;};
        float rgba[4];
    };
} draw_col_t;

void draw_init();
void draw_deinit();
draw_tex_t* draw_create_tex(const char* filename, int* w, int* h, int flags);
draw_tex_t* draw_create_tex_data(uint8_t* data, size_t w, size_t h, int flags);
draw_tex_t* draw_create_tex_scaled(const char* filename, int reqw, int reqh, int* w, int* h, int flags);
draw_tex_t* draw_create_tex_scaled_aabb(const char* filename, int reqw, int reqh, aabb_t* aabb, int flags);
void draw_del_tex(draw_tex_t* tex);
aabb_t draw_get_tex_aabb(draw_tex_t* tex);
draw_effect_t* draw_create_effect(const char* shdr_fname);
void draw_del_effect(draw_effect_t* effect);
void draw_set_tex(draw_tex_t* tex);
draw_tex_t* draw_get_tex();
void draw_set_blend(blend_t blend);
blend_t draw_get_blend();
void draw_set_orientation(float degrees, float ox, float oy);
void draw_set_scale(float x, float y, float ox, float oy);
void draw_add_tri(const float* pos, const draw_col_t* col, const float* uv);
void draw_add_quad(const float* pos, const draw_col_t* col, const float* uv); //vertices: {bl, br, tr, tl}
void draw_add_rect(const float* bl, const float* size, draw_col_t col);
void draw_add_aabb(aabb_t aabb, draw_col_t col);
void draw_prims(size_t viewport_width, size_t viewport_height);
draw_tex_t* draw_prims_fb(size_t viewport_width, size_t viewport_height);
void draw_effect_param_float(draw_effect_t* effect, const char* name, float val);
void draw_effect_param_tex(draw_effect_t* effect, const char* name, draw_tex_t* val);
void draw_do_effect(draw_effect_t* effect, size_t viewport_width, size_t viewport_height);
draw_tex_t* draw_do_effect_fb(draw_effect_t* effect, size_t viewport_width, size_t viewport_height);
void draw_text(const char* text, const float* pos, draw_col_t col, float height);
float draw_text_width(const char* text, float height);
draw_col_t draw_create_rgb(float r, float g, float b);
draw_col_t draw_create_rgba(float r, float g, float b, float a);
#endif
