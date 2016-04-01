#include "game.h"
#include "draw.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_timer.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#define TILE_SIZE 20
#define NUM_YUMYUMS 10

typedef struct snake_bit_t {
    int x, y;
    struct snake_bit_t* next;
    struct snake_bit_t* prev;
} snake_bit_t;

typedef struct yumyum_t {
    int x, y;
} yumyum_t;

static snake_bit_t* head = NULL;
static yumyum_t yumyums[NUM_YUMYUMS];
static Uint64 last_move = 0;
static Uint64 move_interval;
static size_t points = 0;
static int dir[2] = {0, 0};

static void draw_snake() {
    snake_bit_t* cur = head->next;
    while (cur) {
        float pos[] = {TILE_SIZE*(int)cur->x, TILE_SIZE*(int)cur->y};
        float size[] = {TILE_SIZE, TILE_SIZE};
        float col[] = {0.5, 1.0, 0.5};
        draw_add_rect(pos, size, col);
        cur = cur->next;
    }
    
    float pos[] = {TILE_SIZE*(int)head->x, TILE_SIZE*(int)head->y};
    float size[] = {TILE_SIZE, TILE_SIZE};
    float col[] = {0.2, 1.0, 0.2};
    draw_add_rect(pos, size, col);
}

static void draw_yumyums() {
    for (size_t i = 0; i < NUM_YUMYUMS; i++) {
        yumyum_t* yumyum = yumyums + i;
        float pos[] = {TILE_SIZE*yumyum->x, TILE_SIZE*yumyum->y};
        float size[] = {TILE_SIZE, TILE_SIZE};
        float col[] = {1.0, 0.5, 0.5};
        draw_add_rect(pos, size, col);
    }
}

static void place_yumyum(yumyum_t* yumyum) {
    bool used = true;
    while (used) {
        yumyum->x = rand() % (500/TILE_SIZE);
        yumyum->y = rand() % (500/TILE_SIZE);
        
        used = false;
        for (size_t i = 0; i < NUM_YUMYUMS; i++)
            if (yumyums+i != yumyum)
                if (yumyum->x==yumyums[i].x && yumyum->y==yumyums[i].y)
                    used = true;
        
        snake_bit_t* bit = head;
        while (bit) {
            if (yumyum->x==bit->x && yumyum->y==bit->y)
                used = true;
            bit = bit->next;
        }
    }
}

static void update_snake(float frametime) {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_LEFT]) {
        dir[0] = -1;
        dir[1] = 0;
    } else if (keys[SDL_SCANCODE_RIGHT]) {
        dir[0] = 1;
        dir[1] = 0;
    } else if (keys[SDL_SCANCODE_DOWN]) {
        dir[0] = 0;
        dir[1] = -1;
    } else if (keys[SDL_SCANCODE_UP]) {
        dir[0] = 0;
        dir[1] = 1;
    }
    
    for (size_t i = 0; i < NUM_YUMYUMS; i++) {
        int hx = head->x;
        int hy = head->y;
        yumyum_t* yumyum = yumyums + i;
        if (yumyum->x==hx && yumyum->y==hy) {
            place_yumyum(yumyum);
            points++;
        }
    }
    
    if (SDL_GetPerformanceCounter()-last_move > move_interval) {
        snake_bit_t* new_head = malloc(sizeof(snake_bit_t));
        new_head->x = head->x + dir[0];
        new_head->y = head->y + dir[1];
        new_head->next = head;
        new_head->prev = NULL;
        head->prev = new_head;
        head = new_head;
        
        size_t len = 0;
        snake_bit_t* cur = head;
        while (cur->next) {
            cur = cur->next;
            len++;
        }
        
        if (len > points) {
            snake_bit_t* cur = head;
            while (cur->next) cur = cur->next;
            cur->prev->next = NULL;
            free(cur);
        }
        
        last_move = SDL_GetPerformanceCounter();
    }
    
}

void game_init() {
    srand(time(NULL));
    
    head = malloc(sizeof(snake_bit_t));
    head->x = head->y = 0;
    head->next = head->prev = NULL;
    
    for (size_t i = 0; i < NUM_YUMYUMS; i++) {
        yumyums[i].x = -1;
        yumyums[i].y = -1;
    }
    
    for (size_t i = 0; i < NUM_YUMYUMS; i++)
        place_yumyum(yumyums+i);
    
    last_move = SDL_GetPerformanceCounter();
    move_interval = SDL_GetPerformanceFrequency() / 20;
}

void game_deinit() {
    snake_bit_t* cur = head;
    while (cur) {
        snake_bit_t* bit = cur;
        cur = cur->next;
        free(bit);
    }
}

void game_frame(size_t w, size_t h, float frametime) {
    update_snake(frametime);
    
    draw_begin(w, h);
    
    float col[] = {0.5, 0.5, 1.0};
    draw_clear(col);
    
    draw_snake();
    draw_yumyums();
    
    draw_prims();
}
