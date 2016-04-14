#include "celika.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdbool.h>
#include <stdarg.h>

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
static float frametimes[64];
static size_t next_frametime;

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

float celika_get_display_frametime() {
    float max_frametime = frametimes[0];
    for (size_t i = 1; i < 64; i++)
        max_frametime = fmax(max_frametime, frametimes[i]);
    return max_frametime;
}

void celika_set_title(uint32_t* fmt, ...) {
    va_list list, list2;
    va_start(list, fmt);
    va_copy(list2, list);
    
    int count = utf32_vformat(NULL, 0, fmt, list2);
    if (count < 0) {
        va_end(list2);
        va_end(list);
        return;
    }
    
    va_end(list2);
    
    uint32_t* title = malloc(count*4+4);
    utf32_vformat(title, count+1, fmt, list);
    
    uint8_t* utf8 = utf32_to_utf8(title);
    SDL_SetWindowTitle(celika_window, (char*)utf8);
    free(utf8);
    
    free(title);
    
    va_end(list);
}

static void frame() {
    Uint64 start = SDL_GetPerformanceCounter();
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        celika_game_event(event);
        
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
    
    frametimes[next_frametime] = frametime;
    next_frametime = (next_frametime+1) % 64;
    
    int v;
    if (utf32_scan(U"aaa123bbb", U"aaa%dbbb", &v) < 0)
        printf("Error from utf32_scan\n");
    else
        printf("%d\n", v);
    running = false;
    #ifdef __EMSCRIPTEN__
    if (!running) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        SDL_GL_SwapWindow(celika_window);
        
        emscripten_cancel_main_loop();
    }
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
