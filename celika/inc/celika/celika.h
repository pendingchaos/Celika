#include "textedit.h"
#include "aabb.h"
#include "audio.h"
#include "draw.h"
#include "list.h"
#include "font.h"
#include "str.h"

#include <stddef.h>
#include <SDL2/SDL.h>

extern SDL_Window* celika_window;

void celika_game_init(int* w, int* h);
void celika_game_event(SDL_Event event);
void celika_game_frame(size_t w, size_t h, float frametime);
void celika_game_deinit();
float celika_get_display_frametime();
void celika_set_title(uint32_t* fmt, ...);
