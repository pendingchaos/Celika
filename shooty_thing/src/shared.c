#include "shared.h"
#include "shooty_thing.h"

#include <SDL2/SDL.h>

static Uint32 mouse_state = 0;
static Uint32 last_mouse_state = 0;
static size_t w, h;

void gui_begin_frame(size_t w_, size_t h_) {
    mouse_state = SDL_GetMouseState(NULL, NULL);
    w = w_;
    h = h_;
}

void gui_end_frame() {
    last_mouse_state = SDL_GetMouseState(NULL, NULL);
}

bool gui_button(float posy, const char* label) {
    aabb_t aabb = draw_get_tex_aabb(button_tex);
    aabb.left = (w-aabb.width)/2;
    aabb.bottom = (h-aabb.height)/2 + (aabb.height+BUTTON_PADDING)*posy;
    
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    my = h - my - 1;
    
    bool pressed = false;
    bool intersect = mx>aabb.left && mx<aabb_right(aabb) &&
                     my>aabb.bottom && my<aabb_top(aabb);
    float brightness = intersect ? 0.8 : 1;
    if (last_mouse_state&SDL_BUTTON_LMASK &&
        !(mouse_state&SDL_BUTTON_LMASK) &&
        intersect) {
        pressed = true;
    }
    
    draw_set_tex(button_tex);
    draw_add_aabb(aabb, draw_rgb(brightness, brightness, brightness));
    draw_set_tex(NULL);
    
    float text_height = aabb.height * BUTTON_TEXT_SIZE;
    float text_width = draw_text_width(label, text_height);
    float text_pos[] = {aabb_cx(aabb)-text_width/2, aabb_cy(aabb)-text_height/2};
    draw_text(label, text_pos, draw_rgb(0, 0, 0), text_height);
    
    return pressed;
}
