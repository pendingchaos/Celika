#include "celika/celika.h"

font_t* font;
textedit_t* edit;
float bottom;

static void draw_cur_callback(float left, size_t line, void* userdata) {
    aabb_t aabb = create_aabb_lbwh(left, 500-line*32-32, 1, 32);
    draw_add_aabb(aabb, draw_rgb(0, 0, 0));
}

static void draw_insert_callback(float left, float width, size_t line, void* userdata) {
    aabb_t aabb = create_aabb_lbwh(left, 500-line*32-32, width, 5);
    draw_add_aabb(aabb, draw_rgb(0, 0, 0));
}

static void draw_sel_callback(float left, float width, size_t line, void* userdata) {
    aabb_t aabb = create_aabb_lbwh(left, 500-line*32-32, width, 32);
    draw_add_aabb(aabb, draw_rgb(0.1, 0.1, 0.1));
}

static void draw_text_callback(uint32_t* codepoints, size_t line, void* userdata) {
    float pos[] = {0, 500-line*32-32};
    draw_text_font(font, codepoints, pos, draw_rgb(0, 0, 0), 32);
}

void celika_game_init(int* w, int* h) {
    *w = *h = 500;
    
    font = create_font("Junction/webfonts/junction-regular.ttf");
    edit = create_textedit(font, 32, false);
}

void celika_game_deinit() {
    del_textedit(edit);
    del_font(font);
}

void celika_game_event(SDL_Event event) {
    textedit_event(edit, 0, bottom, event);
    bottom = 500 - textedit_calc_aabb(edit, 0, 0).height;
}

void celika_game_frame(size_t w, size_t h, float frametime) {
    draw_add_aabb(create_aabb_lbwh(0, 0, w, h), draw_rgb(1, 1, 1));
    draw_add_aabb(textedit_calc_aabb(edit, 0, bottom), draw_rgb(0.7, 0.7, 0.7));
    textedit_draw_callbacks_t callbacks;
    callbacks.userdata = NULL;
    callbacks.draw_cur = &draw_cur_callback;
    callbacks.draw_insert = &draw_insert_callback;
    callbacks.draw_sel = &draw_sel_callback;
    callbacks.draw_text = &draw_text_callback;
    textedit_draw(edit, callbacks);
}
