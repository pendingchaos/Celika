#pragma once
#ifndef AABB_H
#define AABB_H

#include <stdbool.h>

typedef struct aabb_t {
    float bottom;
    float left;
    float width;
    float height;
} aabb_t;

aabb_t aabb_create_lbwh(float l, float b, float w, float h);
aabb_t aabb_create_lbrt(float l, float b, float r, float t);
aabb_t aabb_create_ce(float cx, float cy, float ex, float ey);
aabb_t aabb_translate(aabb_t aabb, float tx, float ty);
aabb_t aabb_scale(aabb_t aabb, float sx, float sy);
bool aabb_intersect(aabb_t a, aabb_t b);
float aabb_cx(aabb_t aabb);
float aabb_cy(aabb_t aabb);
float aabb_top(aabb_t aabb);
float aabb_right(aabb_t aabb);
#endif
