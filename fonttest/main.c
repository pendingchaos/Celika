#include <celika/celika.h>

font_t* font;

void celika_game_init(int* w, int* h) {
    *w = *h = 500;
    font = create_font("OstrichSans/OstrichSans-Medium.otf");
}

void celika_game_frame(size_t w, size_t h, float frametime) {
    aabb_t aabb = create_aabb_lbwh(0, 0, w, h);
    draw_add_aabb(aabb, draw_rgb(1, 1, 1));
    
    draw_col_t col = draw_rgb(0, 0, 0);
    
    uint32_t* text = utf8_to_utf32("Hello world!");
    float pos[] = {0, 0};
    draw_text_font(font, text, pos, col, 96);
    free(text);
    
    //float draw_text_font_width(font_t* font, const uint32_t* text, size_t height);
}

void celika_game_deinit() {
    del_font(font);
}
