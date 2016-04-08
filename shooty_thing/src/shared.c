#include "shared.h"
#include "shooty_thing.h"

#include <SDL2/SDL.h>

static Uint32 mouse_state = 0;
static Uint32 last_mouse_state = 0;
static size_t w, h;
static float background_scroll = 0;

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

void draw_background(float frametime) {
    float time = background_scroll / (double)BACKGROUND_SCROLL_SPEED;
    time *= BACKGROUND_CHANGE_SPEED;
    draw_tex_t* background_textures[] = {background_tex[(int)floor(time) % 6],
                                         background_tex[(int)ceil(time) % 6]};
    float background_alpha[] = {1, time-floor(time)};
    for (size_t i = 0; i < 2; i++) {
        draw_tex_t* tex = background_textures[i];
        draw_set_tex(tex);
        aabb_t bb = draw_get_tex_aabb(tex);
        for (size_t x = 0; x < ceil(w/bb.width); x++) {
            for (size_t y = 0; y < ceil(h/bb.height)+1; y++) {
                float pos[] = {x*bb.width, y*bb.height};
                pos[1] -= ((int)background_scroll) % (int)bb.height;
                float size[] = {bb.width, bb.height};
                draw_add_rect(pos, size, draw_rgba(1, 1, 1, background_alpha[i]));
            }
        }
        draw_set_tex(NULL);
    }
    
    background_scroll += frametime * BACKGROUND_SCROLL_SPEED;
}
