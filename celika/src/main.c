#include "audio.h"
#include "draw.h"
#include "game.h"
#include "font.h"

#include <SDL2/SDL.h>
#include <stdbool.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define FAIL(...) do {\
    fprintf(stderr, __VA_ARGS__);\
    quit(EXIT_FAILURE);\
} while (0)

SDL_Window* celika_window = NULL;
static SDL_GLContext ctx = NULL;
static bool running = true;
static float frametime = 1 / 60.0;
static Uint64 last_frame_start = 0;

static void quit(int ret) {
    celika_game_deinit();
    font_deinit();
    draw_deinit();
    audio_deinit();
    
    if (ctx) SDL_GL_DeleteContext(ctx);
    if (celika_window) SDL_DestroyWindow(celika_window);
    SDL_Quit();
    
    exit(ret);
}

static void frame() {
    Uint64 start = SDL_GetPerformanceCounter();
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            running = false;
            break;
        }
    }
    
    int w, h;
    SDL_GetWindowSize(celika_window, &w, &h);
    
    draw_begin(w, h);
    celika_game_frame(w, h, frametime);
    draw_end();
    
    SDL_GL_SwapWindow(celika_window);
    
    frametime = (start-last_frame_start) / (double)SDL_GetPerformanceFrequency();
    last_frame_start = start;
    
    char title[256];
    static const char* fmt = "%s - %.0f mspf - %.0f fps";
    snprintf(title, sizeof(title), fmt, "Celika", frametime*1000, 1/frametime);
    
    SDL_SetWindowTitle(celika_window, title);
    
    #ifdef __EMSCRIPTEN__
    if (!running) emscripten_cancel_main_loop();
    #endif
}

int main() {
    int subsystems = SDL_INIT_VIDEO|SDL_INIT_AUDIO/*|SDL_INIT_TIMER*/;
    if (SDL_Init(subsystems) < 0) FAIL("%s", SDL_GetError());
    
    celika_window = SDL_CreateWindow("Celika",
                                     SDL_WINDOWPOS_UNDEFINED,
                                     SDL_WINDOWPOS_UNDEFINED,
                                     500,
                                     500,
                                     SDL_WINDOW_OPENGL);
    if (!celika_window) FAIL("%s", SDL_GetError());
    
    ctx = SDL_GL_CreateContext(celika_window);
    if (!ctx) FAIL("%s", SDL_GetError());
    
    audio_init(44100, 4096);
    draw_init();
    font_init();
    
    int winw, winh;
    celika_game_init(&winw, &winh);
    SDL_SetWindowSize(celika_window, winw, winh);
    
    last_frame_start = SDL_GetPerformanceCounter();
    #ifndef __EMSCRIPTEN__
    while (running)
        frame();
    #else
    emscripten_set_main_loop(&frame, 0, 1);
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
    #endif
    
    quit(EXIT_SUCCESS);
    return 0;
}
