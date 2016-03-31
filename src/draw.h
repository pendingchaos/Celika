#pragma once
#ifndef DRAW_H
#define DRAW_H

#include <stddef.h>

void draw_begin(size_t width, size_t height);
void draw_clear(float* col);
void draw_add_tri(float* pos, float* col);
void draw_add_quad(float* pos, float* col); //vertices: {bl, br, tr, tl}
void draw_add_rect(float* bl, float* size, float* col);
void draw_prims();

#endif
