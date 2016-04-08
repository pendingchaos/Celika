#include "shooty_thing.h"
#include "shared.h"
#include "celika/celika.h"

#include <SDL2/SDL.h>

void mainmenu_state_init() {}
void mainmenu_state_deinit() {}

void mainmenu_state_frame(size_t w, size_t h, float frametime) {
    draw_background(frametime);
    
    static const char* title = "Shooty Thing";
    float text_pos[] = {(w-draw_text_width(title, h*0.15))/2, h*0.75};
    draw_text(title, text_pos, draw_rgb(1, 1, 1), h*0.15);
    
    if (gui_button(0.5, "Play")) state = STATE_PLAYING;
    
    if (gui_button(-0.5, "Quit")) {
        SDL_Event event;
        event.type = SDL_QUIT;
        event.quit.timestamp = SDL_GetTicks();
        SDL_PushEvent(&event);
    }
}
