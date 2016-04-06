#include "audio.h"
#include "draw.h"
#include "game.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdbool.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define FAIL(...) do {\
    fprintf(stderr, __VA_ARGS__);\
    quit(EXIT_FAILURE);\
} while (0)

static SDL_Window* win = NULL;
static SDL_GLContext ctx = NULL;
static bool running = true;
static float frametime = 1 / 60.0;
static Uint64 last_frame_start = 0;

static void quit(int ret) {
    celika_game_deinit();
    draw_deinit();
    audio_deinit();
    
    if (ctx) SDL_GL_DeleteContext(ctx);
    if (win) SDL_DestroyWindow(win);
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
    SDL_GetWindowSize(win, &w, &h);
    
    draw_begin(w, h);
    celika_game_frame(w, h, frametime);
    draw_end();
    
    SDL_GL_SwapWindow(win);
    
    frametime = (start-last_frame_start) / (double)SDL_GetPerformanceFrequency();
    last_frame_start = start;
    
    char title[256];
    static const char* fmt = "%s - %.0f mspf - %.0f fps";
    snprintf(title, sizeof(title), fmt, "Celika", frametime*1000, 1/frametime);
    
    SDL_SetWindowTitle(win, title);
    
    #ifdef __EMSCRIPTEN__
    if (!running) emscripten_cancel_main_loop();
    #endif
}

int main() {
    int subsystems = SDL_INIT_VIDEO|SDL_INIT_AUDIO/*|SDL_INIT_TIMER*/;
    if (SDL_Init(subsystems) < 0) FAIL(SDL_GetError());
    
    win = SDL_CreateWindow("Celika",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           500,
                           500,
                           SDL_WINDOW_OPENGL);
    if (!win) FAIL(SDL_GetError());
    
    ctx = SDL_GL_CreateContext(win);
    if (!ctx) FAIL(SDL_GetError());
    
    #ifndef __EMSCRIPTEN__
    SDL_assert_release(SDL_GL_ExtensionSupported("GL_EXT_framebuffer_sRGB"));
    SDL_assert_release(SDL_GL_ExtensionSupported("GL_EXT_texture_sRGB"));
    SDL_assert_release(SDL_GL_ExtensionSupported("GL_ARB_framebuffer_object"));
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    #endif
    
    audio_init(44100, 4096);
    draw_init();
    
    int winw, winh;
    celika_game_init(&winw, &winh);
    SDL_SetWindowSize(win, winw, winh);
    
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
