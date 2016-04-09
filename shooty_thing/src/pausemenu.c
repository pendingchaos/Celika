#include "shooty_thing.h"
#include "shared.h"
#include "celika/celika.h"

#include <SDL2/SDL.h>

void pause_frame(size_t w, size_t h, float frametime) {
    static const char* title = "Shooty Thing";
    float text_pos[] = {(w-draw_text_width(title, h*0.15))/2, h*0.75};
    draw_text(title, text_pos, draw_rgb(1, 1, 1), h*0.15);
    
    if (gui_button(0.5, "Main Menu")) menu = MENU_MAIN;
    if (gui_button(-0.5, "Unpause")) menu = MENU_NONE;
}
