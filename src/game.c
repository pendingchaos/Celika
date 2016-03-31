#include "game.h"
#include "draw.h"

#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <time.h>

static float col[] = {0.5, 1.0, 0.5};

static void gen_color(float* col) {
    for (size_t i = 0; i < 3; i++)
        col[i] = rand() / (double)RAND_MAX;
    float div = sqrt(col[0]*col[0] + col[1]*col[1] + col[2]*col[2]);
    for (size_t i = 0; i < 3; i++)
        col[i] = col[i]/div * 0.5 + 0.5;
}

void game_init() {
    
}

void game_deinit() {
    
}

void game_frame(size_t w, size_t h, float frametime) {
    srand(time(NULL));
    
    gen_color(col);
    draw_clear(col);
    
    gen_color(col);
    float pos[] = {100, 100};
    float size[] = {300, 300};
    draw_add_rect(pos, size, col);
    
    draw_prims();
}
