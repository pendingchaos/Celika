#include "shooty_thing.h"
#include "shared.h"
#include "celika/celika.h"

#include <SDL2/SDL.h>

void pause_frame(size_t w, size_t h, float frametime) {
    static const uint32_t* title = U"Shooty Thing";
    float text_pos[] = {(w-draw_text_font_width(font, title, h*0.08))/2, h*0.75};
    draw_text_font(font, title, text_pos, draw_rgb(1, 1, 1), h*0.08);
    
    if (gui_button(0.5, "Main Menu")) menu = MENU_MAIN;
    if (gui_button(-0.5, "Unpause")) menu = MENU_NONE;
}
