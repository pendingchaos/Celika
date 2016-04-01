#include "game.h"
#include "draw.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define PLAYER_WIDTH 15
#define PLAYER_HEIGHT 30
#define PLAYER_PROJ_WIDTH 2.5
#define PLAYER_PROJ_HEIGHT 25
#define ENEMY_WIDTH 15
#define ENEMY_HEIGHT 30
#define PLAYER_FIRE_INTERVAL 0.1
#define PLAYER_PROJ_SPEED 400
#define ENEMY_SPEED 200
#define MAX_PLAYER_PROJ 13 //((int)(500.0/PLAYER_PROJ_SPEED/PLAYER_FIRE_INTERVAL))
#define MAX_ENEMIES 25

static float player_posx, player_posy;
static float player_fire_timeout;
static bool player_proj_used[MAX_PLAYER_PROJ];
static float player_proj_x[MAX_PLAYER_PROJ];
static float player_proj_y[MAX_PLAYER_PROJ];
static bool enemy_used[MAX_ENEMIES];
static float enemy_x[MAX_ENEMIES];
static float enemy_y[MAX_ENEMIES];

static void create_player_proj() {
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        if (!player_proj_used[i]) {
            player_proj_x[i] = player_posx;
            player_proj_y[i] = player_posy + PLAYER_HEIGHT;
            player_proj_used[i] = true;
            return;
        }
    }
}

static void create_enemy() {
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        if (!enemy_used[i]) {
            enemy_x[i] = rand() % 500;
            enemy_y[i] = 500;
            enemy_used[i] = true;
            return;
        }
    }
}

static void update(float frametime) {
    int mx, my;
    bool fire = SDL_GetMouseState(&mx, &my) & SDL_BUTTON_LMASK;
    
    player_posx = mx;
    player_posy = 500 - my;
    player_fire_timeout -= frametime;
    
    if (fire && player_fire_timeout<=0) {
        create_player_proj();
        player_fire_timeout = PLAYER_FIRE_INTERVAL;
    }
    
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        player_proj_y[i] += frametime * PLAYER_PROJ_SPEED;
        if (player_proj_y[i] > 500) player_proj_used[i] = false;
    }
    
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        enemy_y[i] -= frametime * ENEMY_SPEED;
        if (enemy_y[i]+ENEMY_HEIGHT < 0) {
            enemy_used[i] = false;
            create_enemy();
        }
        
        float ex = enemy_x[i];
        float ey = enemy_y[i] + ENEMY_HEIGHT/2;
        float px = player_posx;
        float py = player_posy + PLAYER_HEIGHT/2;
        if (fabs(ex-px) < (PLAYER_WIDTH+ENEMY_WIDTH)/2 &&
            fabs(ey-py) < (PLAYER_HEIGHT+ENEMY_HEIGHT)/2) {
            enemy_used[i] = false;
            create_enemy();
            //TODO: Get hurt
        }
        
        for (size_t j = 0; j < MAX_PLAYER_PROJ; j++) {
            float px = player_proj_x[j];
            float py = player_proj_y[j] + PLAYER_PROJ_HEIGHT/2;
            if (fabs(ex-px) < (PLAYER_PROJ_WIDTH+ENEMY_WIDTH)/2 &&
                fabs(ey-py) < (PLAYER_PROJ_HEIGHT+ENEMY_HEIGHT)/2 &&
                player_proj_used[j]) {
                enemy_used[i] = false;
                create_enemy();
            }
        }
    }
}

void shooty_thing_game_init() {
    srand(time(NULL));
    player_posx = 250;
    player_posy = 250;
    player_fire_timeout = 0;
    
    memset(player_proj_used, 0, sizeof(player_proj_used));
    memset(enemy_used, 0, sizeof(enemy_used));
    
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        create_enemy();
        enemy_y[i] = rand() % 500;
    }
}

void shooty_thing_game_deinit() {
}

void shooty_thing_game_frame(size_t w, size_t h, float frametime) {
    update(frametime);
    
    draw_begin(w, h);
    
    float background[] = {0.1, 0.1, 0.2};
    draw_clear(background);
    
    float pos[] = {player_posx-PLAYER_WIDTH/2,
                   player_posy};
    float size[] = {PLAYER_WIDTH, PLAYER_HEIGHT};
    float col[] = {0.5, 1.0, 0.5};
    draw_add_rect(pos, size, col);
    
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        if (player_proj_used[i]) {
            float x = player_proj_x[i];
            float y = player_proj_y[i];
            float pos[] = {x-PLAYER_PROJ_WIDTH/2, y};
            float size[] = {PLAYER_PROJ_WIDTH, PLAYER_PROJ_HEIGHT};
            float col[] = {1.0, 0.5, 0.5};
            draw_add_rect(pos, size, col);
        }
    }
    
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        if (enemy_used[i]) {
            float x = enemy_x[i];
            float y = enemy_y[i];
            float pos[] = {x-ENEMY_WIDTH/2, y};
            float size[] = {ENEMY_WIDTH, ENEMY_HEIGHT};
            float col[] = {0.5, 0.5, 0.5};
            draw_add_rect(pos, size, col);
        }
    }
    
    draw_prims();
}
