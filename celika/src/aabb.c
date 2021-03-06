#include "aabb.h"

#include <math.h>

aabb_t aabb_create_lbwh(float l, float b, float w, float h) {
    aabb_t res;
    res.left = l;
    res.bottom = b;
    res.width = w;
    res.height = h;
    return res;
}

aabb_t aabb_create_lbrt(float l, float b, float r, float t) {
    aabb_t res;
    res.left = l;
    res.bottom = b;
    res.width = r - l;
    res.height = t - b;
    return res;
}

aabb_t aabb_create_ce(float cx, float cy, float ex, float ey) {
    aabb_t res;
    res.left = cx - ex;
    res.bottom = cy - ey;
    res.width = ex * 2;
    res.height = ey * 2;
    return res;
}

aabb_t aabb_translate(aabb_t aabb, float tx, float ty) {
    aabb_t res = aabb;
    res.left += tx;
    res.bottom += ty;
    return res;
}

aabb_t aabb_scale(aabb_t aabb, float sx, float sy) {
    float left = aabb.left;
    float right = left + aabb.width;
    float bottom = aabb.bottom;
    float top = bottom + aabb.height;
    float cx = (left+right) / 2;
    float cy = (bottom+top) / 2;
    left = (left-cx)*sx + cx;
    right = (right-cx)*sx + cx;
    bottom = (bottom-cy)*sy + cy;
    top = (top-cy)*sy + cy;
    return aabb_create_lbrt(left, bottom, right, top);
}

bool aabb_intersect(aabb_t a, aabb_t b) {
    float exa = a.width / 2;
    float eya = a.height / 2;
    float exb = b.width / 2;
    float eyb = b.height / 2;
    float cxa = a.left + exa;
    float cya = a.bottom + eya;
    float cxb = b.left + exb;
    float cyb = b.bottom + eyb;
    return fabs(cxa-cxb) < (exa+exb) &&
           fabs(cya-cyb) < (eya+eyb);
}

float aabb_cx(aabb_t aabb) {
    return aabb.left + aabb.width/2;
}

float aabb_cy(aabb_t aabb) {
    return aabb.bottom + aabb.height/2;
}

float aabb_top(aabb_t aabb) {
    return aabb.bottom + aabb.height;
}

float aabb_right(aabb_t aabb) {
    return aabb.left + aabb.width;
}

void aabb_set_cx(aabb_t* aabb, float cx) {
    aabb->left = cx - aabb->width/2;
}

void aabb_set_cy(aabb_t* aabb, float cy) {
    aabb->bottom = cy - aabb->height/2;
}

void aabb_set_right(aabb_t* aabb, float right) {
    aabb->left = right - aabb->width;
}

void aabb_set_top(aabb_t* aabb, float top) {
    aabb->bottom = top - aabb->height;
}
