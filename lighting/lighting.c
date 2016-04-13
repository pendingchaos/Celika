#include "celika/celika.h"

static float light_x, light_y;
static float light_radius = 250;
static draw_tex_t* light_tex;

void draw_shadow_line(float x0, float y0, float x1, float y1,
                      float length, draw_col_t col) {
    float dirx0 = x0 - light_x;
    float diry0 = y0 - light_y;
    float div = sqrt(dirx0*dirx0 + diry0*diry0);
    dirx0 = dirx0/div * length;
    diry0 = diry0/div * length;
    
    float dirx1 = x1 - light_x;
    float diry1 = y1 - light_y;
    div = sqrt(dirx1*dirx1 + diry1*diry1);
    dirx1 = dirx1/div * length;
    diry1 = diry1/div * length;
    
    float pos[] = {x0, y0, x1, y1,
                   x1+dirx1, y1+diry1, x0+dirx0, y0+diry0};
    draw_col_t cols[] = {col, col, col, col};
    float uv[] = {0, 0, 0, 0, 0, 0, 0, 0};
    draw_add_quad(pos, cols, uv);
}

void draw_shadow(aabb_t aabb, float length, draw_col_t col) {
    float left = aabb.left;
    float right = aabb_right(aabb);
    float bottom = aabb.bottom;
    float top = aabb_top(aabb);
    
    draw_shadow_line(left, bottom, right, bottom, length, col);
    draw_shadow_line(left, top, right, top, length, col);
    draw_shadow_line(left, bottom, aabb.left, top, length, col);
    draw_shadow_line(right, bottom, right, top, length, col);
}

void celika_game_init(int* w, int* h) {
    *w = *h = 500;
    light_tex = draw_create_tex("lighttex.png", NULL, NULL);
}

void celika_game_deinit() {
    draw_del_tex(light_tex);
}

void celika_game_event(SDL_Event event) {
}

void celika_game_frame(size_t w, size_t h, float frametime) {
    draw_add_aabb(aabb_create_lbwh(0, 0, w, h), draw_create_rgb(1, 1, 1));
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    light_x = mx;
    light_y = 500 - my;
    
    #define BOX_SHADOW(x, y) draw_shadow(aabb_create_ce(x, y, 20, 20),\
                                         100000, draw_create_rgb(0.0, 0.0, 0.0));
    BOX_SHADOW(265, 175)
    BOX_SHADOW(72, 110)
    BOX_SHADOW(132, 276)
    BOX_SHADOW(149, 420)
    BOX_SHADOW(381, 73)
    #undef BOX_SHADOW
    
    #define BOX(x, y)  draw_add_aabb(aabb_create_ce(x, y, 20, 20),\
                                     draw_create_rgb(1, 0.5, 0.5));
    BOX(265, 175)
    BOX(72, 110)
    BOX(132, 276)
    BOX(149, 420)
    BOX(381, 73)
    #undef BOX
    
    aabb_t light_aabb = aabb_create_ce(light_x, light_y, light_radius, light_radius);
    draw_set_blend(BLEND_MULT);
    draw_set_tex(light_tex);
    draw_add_aabb(light_aabb, draw_create_rgb(1, 1, 1));
    draw_set_tex(NULL);
    
    aabb_t left_aabb = aabb_create_lbwh(light_aabb.left-500, light_aabb.bottom-500,
                                        500, 1000+light_radius);
    draw_add_aabb(left_aabb, draw_create_rgb(0, 0, 0));
    
    aabb_t right_aabb = aabb_create_lbwh(aabb_right(light_aabb), light_aabb.bottom-500,
                                         500, 1000+light_radius);
    draw_add_aabb(right_aabb, draw_create_rgb(0, 0, 0));
    
    aabb_t bottom_aabb = aabb_create_lbwh(light_aabb.left, light_aabb.bottom-500,
                                          light_radius*2, 500);
    draw_add_aabb(bottom_aabb, draw_create_rgb(0, 0, 0));
    
    aabb_t top_aabb = aabb_create_lbwh(light_aabb.left, aabb_top(light_aabb),
                                       light_radius*2, 500);
    draw_add_aabb(top_aabb, draw_create_rgb(0, 0, 0));
    
    draw_set_blend(BLEND_ALPHA);
    
    celika_set_title(U"Lighting - %d fps", (int)(1/celika_get_display_frametime()));
}
