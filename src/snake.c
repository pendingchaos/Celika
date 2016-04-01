#include "game.h"
#include "draw.h"

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
    struct snake_bit_t* next;
    struct snake_bit_t* prev;
} snake_bit_t;

typedef struct yumyum_t {
    int x, y;
} yumyum_t;

static state_t state = STATE_PAUSED;
static snake_bit_t* head = NULL;
static yumyum_t yumyums[NUM_YUMYUMS];
static Uint64 last_move;
static Uint64 move_interval;
static size_t points;
static int dir[2];
static SDL_Scancode dir_key;

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

static bool used(int x, int y) {
    for (size_t i = 0; i < NUM_YUMYUMS; i++)
        if (x==yumyums[i].x && y==yumyums[i].y)
            return true;
    
    snake_bit_t* bit = head;
    while (bit) {
        if (x==bit->x && y==bit->y)
            return true;
        bit = bit->next;
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
        snake_bit_t* new_head = malloc(sizeof(snake_bit_t));
        new_head->x = head->x + dir[0];
        new_head->y = head->y + dir[1];
        
        for (size_t i = 0; i < NUM_YUMYUMS; i++) {
            int hx = new_head->x;
            int hy = new_head->y;
            yumyum_t* yumyum = yumyums + i;
            if (yumyum->x==hx && yumyum->y==hy) {
                place_yumyum(yumyum);
                points++;
            }
        }
        
        if (used(new_head->x, new_head->y)) state = STATE_LOST;
        if (new_head->x < 0 || new_head->x>WIDTH-1) state = STATE_LOST;
        if (new_head->y < 0 || new_head->y>HEIGHT-1) state = STATE_LOST;
        
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

void snake_game_init() {
    srand(time(NULL));
    
    points = 0;
    memset(dir, 0, sizeof(dir));
    
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
    dir_key = 0;
    
    state = STATE_PAUSED;
}

void snake_game_deinit() {
    snake_bit_t* cur = head;
    while (cur) {
        snake_bit_t* bit = cur;
        cur = cur->next;
        free(bit);
    }
}

void snake_game_frame(size_t w, size_t h, float frametime) {
    draw_begin(w, h);
    
    float col[] = {0.5, 0.5, 1.0};
    draw_clear(col);
    
    update_snake(frametime);
    
    draw_snake();
    draw_yumyums();
    
    draw_prims();
    
    if (state == STATE_LOST) game_init();
}
