#include "game.h"
#include "draw.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_timer.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#define TILE_SIZE 20

typedef struct snake_bit_t {
    int x, y;
    struct snake_bit_t* next;
    struct snake_bit_t* prev;
} snake_bit_t;

typedef struct yumyum_t {
    int x, y;
    struct yumyum_t* next;
    struct yumyum_t* prev;
} yumyum_t;

static snake_bit_t* head = NULL;
static yumyum_t* yumyums = NULL;
static Uint64 last_move = 0;
static Uint64 move_interval;
static size_t points = 0;
static int dir[2] = {0, 0};

static void draw_snake() {
    snake_bit_t* cur = head;
    while (cur) {
        float pos[] = {TILE_SIZE*(int)cur->x, TILE_SIZE*(int)cur->y};
        float size[] = {TILE_SIZE, TILE_SIZE};
        float col[] = {1.0, 0.5, 0.5};
        draw_add_rect(pos, size, col);
        cur = cur->next;
    }
}

static void draw_yumyums() {
    yumyum_t* cur = yumyums;
    while (cur) {
        float pos[] = {TILE_SIZE*cur->x, TILE_SIZE*cur->y};
        float size[] = {TILE_SIZE, TILE_SIZE};
        float col[] = {0.5, 1.0, 0.5};
        draw_add_rect(pos, size, col);
        cur = cur->next;
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
    
    yumyum_t* yumyum = yumyums;
    while (yumyum) {
        int hx = head->x;
        int hy = head->y;
        if (yumyum->x==hx && yumyum->y==hy) {
            yumyum->x = rand() % (500/TILE_SIZE); //TODO Ensure there is not yumyum at this location
            yumyum->y = rand() % (500/TILE_SIZE);
            points++;
        }
        yumyum = yumyum->next;
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

static void create_yumyum() {
    yumyum_t* yumyum = malloc(sizeof(yumyum_t));
    yumyum->x = rand() % (500/TILE_SIZE); //TODO Ensure there is not yumyum at this location
    yumyum->y = rand() % (500/TILE_SIZE);
    yumyum->prev = NULL;
    yumyum->next = yumyums;
    yumyums = yumyum;
}

void game_init() {
    srand(time(NULL));
    
    head = malloc(sizeof(snake_bit_t));
    head->x = head->y = 0;
    head->next = head->prev = NULL;
    
    yumyums = NULL;
    for (size_t i = 0; i < 10; i++)
        create_yumyum();
    
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
