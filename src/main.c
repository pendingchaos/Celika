#include "draw.h"
#include "game.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdbool.h>

#define FAIL(...) do {\
    fprintf(stderr, __VA_ARGS__);\
    quit(EXIT_FAILURE);\
} while (0)

static SDL_Window* win = NULL;
static SDL_GLContext ctx = NULL;

static void quit() {
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

int main() {
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
    
    win = SDL_CreateWindow(PROJNAME,
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           500,
                           500,
                           SDL_WINDOW_OPENGL);
    if (!win) FAIL(SDL_GetError());
    
    ctx = SDL_GL_CreateContext(win);
    if (!ctx) FAIL(SDL_GetError());
    
    SDL_assert_release(SDL_GL_ExtensionSupported("GL_EXT_framebuffer_sRGB"));
    SDL_assert_release(SDL_GL_ExtensionSupported("GL_EXT_texture_sRGB"));
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    
    bool running = true;
    float frametime = 1 / 60.0;
    
    draw_init();
    
    int winw, winh;
    game_init(&winw, &winh);
    SDL_SetWindowSize(win, winw, winh);
    
    while (running) {
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
        game_frame(w, h, frametime);
        SDL_GL_SwapWindow(win);
        
        Uint64 end = SDL_GetPerformanceCounter();
        frametime = (end-start) / (double)SDL_GetPerformanceFrequency();
        
        char title[256];
        static const char* fmt = "%s - %.0f mspf";
        snprintf(title, sizeof(title), fmt, PROJNAME, frametime*1000);
        
        SDL_SetWindowTitle(win, title);
    }
    
    game_deinit();
    draw_deinit();
    
    quit(EXIT_SUCCESS);
    return 0;
}
