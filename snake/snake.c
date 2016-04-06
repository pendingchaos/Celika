#include "celika/celika.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_timer.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#define TILE_SIZE 20
#define WIDTH (500/TILE_SIZE)
#define HEIGHT (500/TILE_SIZE)
#define NUM_YUMYUMS 10

typedef enum state_t {
    STATE_PAUSED,
    STATE_PLAYING,
    STATE_LOST
} state_t;

typedef struct snake_bit_t {
    int x, y;
} snake_bit_t;

typedef struct yumyum_t {
    int x, y;
} yumyum_t;

static state_t state = STATE_PAUSED;
static list_t* bits; //list_t of snake_bit_t
static yumyum_t yumyums[NUM_YUMYUMS];
static Uint64 last_move;
static Uint64 move_interval;
static size_t points;
static int dir[2];
static SDL_Scancode dir_key;

static void draw_snake() {
    for (size_t i = 1; i < list_len(bits); i++) {
        snake_bit_t* bit = list_nth(bits, i);
        float pos[] = {TILE_SIZE*(int)bit->x, TILE_SIZE*(int)bit->y};
        float size[] = {TILE_SIZE, TILE_SIZE};
        draw_add_rect(pos, size, draw_rgb(0.5, 1, 0.5));
    }
    
    snake_bit_t* head = list_nth(bits, 0);
    float pos[] = {TILE_SIZE*(int)head->x, TILE_SIZE*(int)head->y};
    float size[] = {TILE_SIZE, TILE_SIZE};
    draw_add_rect(pos, size, draw_rgb(0.2, 1, 0.2));
}

static void draw_yumyums() {
    for (size_t i = 0; i < NUM_YUMYUMS; i++) {
        yumyum_t* yumyum = yumyums + i;
        float pos[] = {TILE_SIZE*yumyum->x, TILE_SIZE*yumyum->y};
        float size[] = {TILE_SIZE, TILE_SIZE};
        draw_add_rect(pos, size, draw_rgb(1, 0.5, 0.5));
    }
}

static bool used(int x, int y) {
    for (size_t i = 0; i < NUM_YUMYUMS; i++)
        if (x==yumyums[i].x && y==yumyums[i].y)
            return true;
    
    for (size_t i = 0; i < list_len(bits); i++) {
        snake_bit_t* bit = list_nth(bits, i);
        if (x==bit->x && y==bit->y)
            return true;
    }
    
    return false;
}

static void place_yumyum(yumyum_t* yumyum) {
    yumyum->x = -1;
    yumyum->y = -1;
    int x, y;
    do {
        x = rand() % WIDTH;
        y = rand() % HEIGHT;
    } while (used(x, y));
    yumyum->x = x;
    yumyum->y = y;
}

static void update_snake(float frametime) {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if ((keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) &&
        dir_key!=SDL_SCANCODE_RIGHT) {
        dir[0] = -1;
        dir[1] = 0;
        dir_key = SDL_SCANCODE_LEFT;
        state = state==STATE_PAUSED ? STATE_PLAYING : state;
    }
    
    if ((keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) &&
        dir_key!=SDL_SCANCODE_LEFT) {
        dir[0] = 1;
        dir[1] = 0;
        dir_key = SDL_SCANCODE_RIGHT;
        state = state==STATE_PAUSED ? STATE_PLAYING : state;
    }
    
    if ((keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) &&
        dir_key!=SDL_SCANCODE_UP) {
        dir[0] = 0;
        dir[1] = -1;
        dir_key = SDL_SCANCODE_DOWN;
        state = state==STATE_PAUSED ? STATE_PLAYING : state;
    }
    
    if ((keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) &&
        dir_key!=SDL_SCANCODE_DOWN) {
        dir[0] = 0;
        dir[1] = 1;
        dir_key = SDL_SCANCODE_UP;
        state = state==STATE_PAUSED ? STATE_PLAYING : state;
    }
    
    if (SDL_GetPerformanceCounter()-last_move > move_interval &&
        state==STATE_PLAYING) {
        snake_bit_t* head = list_nth(bits, 0);
        
        snake_bit_t new_head;
        new_head.x = head->x + dir[0];
        new_head.y = head->y + dir[1];
        
        for (size_t i = 0; i < NUM_YUMYUMS; i++) {
            int hx = new_head.x;
            int hy = new_head.y;
            yumyum_t* yumyum = yumyums + i;
            if (yumyum->x==hx && yumyum->y==hy) {
                place_yumyum(yumyum);
                points++;
            }
        }
        
        if (used(new_head.x, new_head.y)) state = STATE_LOST;
        if (new_head.x < 0 || new_head.x>WIDTH-1) state = STATE_LOST;
        if (new_head.y < 0 || new_head.y>HEIGHT-1) state = STATE_LOST;
        
        list_insert(bits, 0, &new_head);
        
        if (list_len(bits)-1 > points)
            list_remove(list_nth(bits, list_len(bits)-1));
        
        last_move = SDL_GetPerformanceCounter();
    }
}

static void setup_state() {
    points = 0;
    memset(dir, 0, sizeof(dir));
    
    snake_bit_t head;
    head.x = head.y = 0;
    
    bits = list_new(sizeof(snake_bit_t), NULL);
    list_append(bits, &head);
    
    for (size_t i = 0; i < NUM_YUMYUMS; i++) {
        yumyums[i].x = -1;
        yumyums[i].y = -1;
    }
    
    for (size_t i = 0; i < NUM_YUMYUMS; i++)
        place_yumyum(yumyums+i);
    
    last_move = SDL_GetPerformanceCounter();
    move_interval = SDL_GetPerformanceFrequency() / 20;
    dir_key = 0;
    
    state = STATE_PAUSED;
}

static void free_state() {
    list_free(bits);
}

void celika_game_init(int* w, int* h) {
    *w = *h = 500;
    
    srand(time(NULL));
    
    setup_state();
}

void celika_game_deinit() {
    free_state();
}

void celika_game_frame(size_t w, size_t h, float frametime) {
    draw_add_aabb(create_aabb_lbwh(0, 0, w, h), draw_rgb(0.5, 0.5, 1));
    
    update_snake(frametime);
    
    draw_snake();
    draw_yumyums();
    
    draw_prims();
    
    if (state == STATE_LOST) {
        free_state();
        setup_state();
    }
}
