#include "shooty_thing.h"
#include "shared.h"
#include "celika/celika.h"

#include <SDL2/SDL.h>

void mainmenu_frame(size_t w, size_t h, float frametime) {
    static const uint32_t* title = U"Shooty Thing";
    float text_pos[] = {(w-font_drawn_width(font, title, h*0.08))/2, h*0.75};
    font_draw(font, title, text_pos, draw_create_rgb(1, 1, 1), h*0.08);
    
    if (gui_button(0.5, "Play")) menu = MENU_NONE;
    
    if (gui_button(-0.5, "Quit")) {
        SDL_Event event;
        event.type = SDL_QUIT;
        event.quit.timestamp = SDL_GetTicks();
        SDL_PushEvent(&event);
    }
}
