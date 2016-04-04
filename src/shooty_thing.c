#include "game.h"
#include "draw.h"
#include "builtinfont.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500
#define PLAYER_FIRE_INTERVAL 0.1
#define PLAYER_PROJ_SPEED 400
#define MAX_PLAYER_PROJ 13 //(int)(WINDOW_HEIGHT/PLAYER_PROJ_SPEED/PLAYER_FIRE_INTERVAL)
#define MAX_ENEMIES 20
#define ENEMY_SPEED 200
#define ENEMY_FIRE_INTERVAL 0.5
#define ENEMY_PROJ_SPEED 400
#define ENEMY_PROJ_HIT_DAMAGE 0.025
#define MAX_ENEMY_PROJ 40 //(int)(WINDOW_HEIGHT/PLAYER_PROJ_SPEED/PLAYER_FIRE_INTERVAL) * MAX_ENEMIES
#define ENEMY_HIT_DAMAGE 0.05
#define PLAYER_PROJ_AMMO_USAGE 0.01
#define ENEMY_COUNT_UPDATE_RATE 0.25
#define ENEMY_FIRE_DIST_THRESHOLD 30
#define MAX_AMMO_PACK 32
#define AMMO_PACK_CHANCE 0.3
#define AMMO_PACK_SPEED 100
#define AMMO_PACK_INC 0.25
#define MAX_HP_PACK 32
#define HP_PACK_CHANCE 0.2
#define HP_PACK_SPEED 100
#define HP_PACK_INC 0.1
#define BACKGROUND_SCROLL_SPEED 128
#define BACKGROUND_CHANGE_SPEED 0.1

typedef enum state_t {
    STATE_PLAYING,
    STATE_LOST
} state_t;

static state_t state;
static draw_tex_t* background_tex[6];
static int background_width;
static int background_height;
static float background_scroll;
static draw_tex_t* player_tex;
static aabb_t player_aabb;
static float player_fire_timeout;
static float player_hp;
static float player_ammo;
static draw_tex_t* player_proj_tex;
static bool player_proj_used[MAX_PLAYER_PROJ];
static aabb_t player_proj_aabb[MAX_PLAYER_PROJ];
static draw_tex_t* enemy_proj_tex;
static bool enemy_proj_used[MAX_ENEMY_PROJ];
static aabb_t enemy_proj_aabb[MAX_ENEMY_PROJ];
static draw_tex_t* enemy_tex;
static bool enemy_used[MAX_ENEMIES];
static aabb_t enemy_aabb[MAX_ENEMIES];
static float enemy_vel_x[MAX_ENEMIES];
static float enemy_fire_timeout[MAX_ENEMIES];
static float enemy_count;
static draw_tex_t* ammo_pack_tex;
static aabb_t ammo_pack_aabb[MAX_AMMO_PACK];
static bool ammo_pack_used[MAX_AMMO_PACK];
static draw_tex_t* hp_pack_tex;
static aabb_t hp_pack_aabb[MAX_HP_PACK];
static bool hp_pack_used[MAX_HP_PACK];
static size_t player_hit;
static size_t player_miss;

static void create_player_proj() {
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        if (!player_proj_used[i]) {
            player_proj_aabb[i] = draw_get_tex_aabb(player_proj_tex);
            player_proj_aabb[i].left = player_aabb.left + player_aabb.width/2;
            player_proj_aabb[i].left -= player_proj_aabb[i].width/2;
            player_proj_aabb[i].bottom = player_aabb.bottom + player_aabb.height;
            player_proj_used[i] = true;
            return;
        }
    }
}

static void create_enemy_proj(size_t i) {
    for (size_t i = 0; i < MAX_ENEMY_PROJ; i++) {
        if (!enemy_proj_used[i]) {
            enemy_proj_aabb[i] = draw_get_tex_aabb(enemy_proj_tex);
            enemy_proj_aabb[i].left = enemy_aabb[i].left + player_aabb.width/2;
            enemy_proj_aabb[i].left -= enemy_proj_aabb[i].width/2;
            enemy_proj_aabb[i].bottom = enemy_aabb[i].bottom - enemy_proj_aabb[i].height;
            enemy_proj_used[i] = true;
            return;
        }
    }
}

static void create_enemy() {
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        if (!enemy_used[i]) {
            float cx = rand() % WINDOW_WIDTH;
            enemy_aabb[i] = draw_get_tex_aabb(enemy_tex);
            enemy_aabb[i].left = cx - enemy_aabb[i].width/2;
            enemy_aabb[i].bottom = WINDOW_HEIGHT;
            enemy_vel_x[i] = cx<WINDOW_WIDTH/2 ? 1 : -1;
            enemy_vel_x[i] *= rand()%25 + 25;
            enemy_used[i] = true;
            enemy_fire_timeout[i] = 0;
            return;
        }
    }
}

static void create_ammo_pack() {
    for (size_t i = 0; i < MAX_AMMO_PACK; i++) {
        if (!ammo_pack_used[i]) {
            ammo_pack_aabb[i] = draw_get_tex_aabb(ammo_pack_tex);
            ammo_pack_aabb[i].left = rand()%WINDOW_WIDTH - ammo_pack_aabb[i].width/2;
            ammo_pack_aabb[i].bottom = WINDOW_HEIGHT;
            ammo_pack_used[i] = true;
            return;
        }
    }
}

static void create_hp_pack() {
    for (size_t i = 0; i < MAX_HP_PACK; i++) {
        if (!hp_pack_used[i]) {
            hp_pack_aabb[i] = draw_get_tex_aabb(hp_pack_tex);
            hp_pack_aabb[i].left = rand()%WINDOW_WIDTH - hp_pack_aabb[i].width/2;
            hp_pack_aabb[i].bottom = WINDOW_HEIGHT;
            hp_pack_used[i] = true;
            return;
        }
    }
}

static void update(float frametime) {
    int mx, my;
    bool fire = SDL_GetMouseState(&mx, &my) & SDL_BUTTON_LMASK;
    
    player_aabb.left = mx - player_aabb.width/2;
    player_aabb.bottom = WINDOW_HEIGHT - my;
    player_fire_timeout -= frametime;
    
    if (fire && player_fire_timeout<=0 && player_ammo>0) {
        create_player_proj();
        player_fire_timeout = PLAYER_FIRE_INTERVAL;
        player_ammo -= PLAYER_PROJ_AMMO_USAGE;
    }
    
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        if (!player_proj_used[i]) continue;
        player_proj_aabb[i].bottom += frametime * PLAYER_PROJ_SPEED;
        if (player_proj_aabb[i].bottom > WINDOW_HEIGHT) {
            player_proj_used[i] = false;
            player_miss++;
        }
    }
    
    for (size_t i = 0; i < MAX_ENEMY_PROJ; i++) {
        if (!enemy_proj_used[i]) continue;
        enemy_proj_aabb[i].bottom -= frametime * ENEMY_PROJ_SPEED;
        if (aabb_top(enemy_proj_aabb[i]) < 0)
            enemy_proj_used[i] = false;
        
        if (intersect_aabb(enemy_proj_aabb[i], player_aabb)) {
            enemy_proj_used[i] = false;
            player_hp -= ENEMY_PROJ_HIT_DAMAGE;
        }
    }
    
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        if (!enemy_used[i]) continue;
        
        enemy_aabb[i].bottom -= frametime * ENEMY_SPEED;
        enemy_aabb[i].left += enemy_vel_x[i] * frametime;
        if (aabb_top(enemy_aabb[i]) < 0) {
            enemy_used[i] = false;
            create_enemy();
            continue;
        }
        
        if (intersect_aabb(enemy_aabb[i], player_aabb)) {
            enemy_used[i] = false;
            create_enemy();
            player_hp -= ENEMY_HIT_DAMAGE;
        }
        
        enemy_fire_timeout[i] -= frametime;
        float ex = aabb_cx(enemy_aabb[i]);
        float px = aabb_cx(player_aabb);
        if (fabs(ex-px)<ENEMY_FIRE_DIST_THRESHOLD &&
            enemy_fire_timeout[i]<=0) {
            create_enemy_proj(i);
            enemy_fire_timeout[i] = ENEMY_FIRE_INTERVAL;
        }
        
        for (size_t j = 0; j < MAX_PLAYER_PROJ; j++) {
            if (intersect_aabb(enemy_aabb[i], player_proj_aabb[j]) &&
                player_proj_used[j]) {
                player_proj_used[j] = false;
                enemy_used[i] = false;
                create_enemy();
                player_hit++;
            }
        }
    }
    
    for (size_t i = 0; i < MAX_AMMO_PACK; i++) {
        if (!ammo_pack_used[i]) continue;
        
        ammo_pack_aabb[i].bottom -= frametime * AMMO_PACK_SPEED;
        if (aabb_top(ammo_pack_aabb[i]) < 0) {
            ammo_pack_used[i] = false;
            continue;
        }
        
        if (intersect_aabb(ammo_pack_aabb[i], player_aabb)) {
            ammo_pack_used[i] = false;
            player_ammo += AMMO_PACK_INC;
        }
    }
    
    for (size_t i = 0; i < MAX_HP_PACK; i++) {
        if (!hp_pack_used[i]) continue;
        
        hp_pack_aabb[i].bottom -= frametime * HP_PACK_SPEED;
        if (aabb_top(hp_pack_aabb[i]) < 0) {
            hp_pack_used[i] = false;
            continue;
        }
        
        if (intersect_aabb(hp_pack_aabb[i], player_aabb)) {
            hp_pack_used[i] = false;
            player_hp += HP_PACK_INC;
        }
    }
    
    enemy_count += frametime * ENEMY_COUNT_UPDATE_RATE;
    
    size_t actual_enemy_count = 0;
    for (size_t i = 0; i < MAX_ENEMIES; i++)
        actual_enemy_count += enemy_used[i] ? 1 : 0;
    
    for (size_t i = 0; i < ((int)enemy_count)-actual_enemy_count; i++)
        create_enemy();
    
    if (rand()/(double)RAND_MAX > (1.0-AMMO_PACK_CHANCE*frametime))
        create_ammo_pack();
    
    if (rand()/(double)RAND_MAX > (1.0-HP_PACK_CHANCE*frametime))
        create_hp_pack();
    
    if (player_hp <= 0) state = STATE_LOST;
    
    player_ammo = fmin(fmax(player_ammo, 0), 1);
    player_hp = fmin(fmax(player_hp, 0), 1);
}

static void setup_state() {
    player_fire_timeout = 0;
    player_hp = 1;
    player_ammo = 1;
    player_hit = 0;
    player_miss = 0;
    enemy_count = 1;
    
    memset(player_proj_used, 0, sizeof(player_proj_used));
    memset(enemy_used, 0, sizeof(enemy_used));
    memset(ammo_pack_used, 0, sizeof(ammo_pack_used));
    memset(hp_pack_used, 0, sizeof(hp_pack_used));
    
    state = STATE_PLAYING;
}

void shooty_thing_game_init(int* w, int* h) {
    *w = WINDOW_WIDTH;
    *h = WINDOW_HEIGHT;
    
    srand(time(NULL));
    
    player_tex = draw_create_scaled_tex_aabb("SpaceShooterRedux/PNG/playerShip3_green.png",
                                             0, 30, &player_aabb);
    
    enemy_tex = draw_create_scaled_tex("SpaceShooterRedux/PNG/ufoRed.png",
                                       0, 30, NULL, NULL);
    
    player_proj_tex = draw_create_scaled_tex("SpaceShooterRedux/PNG/Lasers/laserGreen10.png",
                                             0, 25, NULL, NULL);
    
    enemy_proj_tex = draw_create_scaled_tex("SpaceShooterRedux/PNG/Lasers/laserRed16.png",
                                            0, 25, NULL, NULL);
    
    hp_pack_tex = draw_create_scaled_tex("SpaceShooterRedux/PNG/Power-ups/pill_yellow.png",
                                         0, 25, NULL, NULL);
    
    ammo_pack_tex = draw_create_scaled_tex("SpaceShooterRedux/PNG/Power-ups/bolt_gold.png",
                                           0, 25, NULL, NULL);
    
    background_scroll = 0;
    background_tex[0] = draw_create_tex("SpaceShooterRedux/Backgrounds/black.png",
                                        &background_width, &background_height);
    background_tex[1] = draw_create_tex("SpaceShooterRedux/Backgrounds/blue.png",
                                        &background_width, &background_height);
    background_tex[2] = draw_create_tex("SpaceShooterRedux/Backgrounds/darkPurple.png",
                                        &background_width, &background_height);
    background_tex[3] = draw_create_tex("SpaceShooterRedux/Backgrounds/purple.png",
                                        &background_width, &background_height);
    background_tex[4] = background_tex[2];
    background_tex[5] = background_tex[1];
    
    setup_state();
}

void shooty_thing_game_deinit() {
    for (size_t i = 0; i < 4; i++)
        draw_del_tex(background_tex[i]);
    draw_del_tex(ammo_pack_tex);
    draw_del_tex(hp_pack_tex);
    draw_del_tex(enemy_proj_tex);
    draw_del_tex(player_proj_tex);
    draw_del_tex(enemy_tex);
    draw_del_tex(player_tex);
}

void shooty_thing_game_frame(size_t w, size_t h, float frametime) {
    if (state == STATE_PLAYING)
        update(frametime);
    else if (state == STATE_LOST)
        setup_state();
    
    draw_begin(w, h);
    
    float time = background_scroll / (double)BACKGROUND_SCROLL_SPEED;
    time *= BACKGROUND_CHANGE_SPEED;
    draw_tex_t* background_textures[] = {background_tex[(int)floor(time) % 6],
                                         background_tex[(int)ceil(time) % 6]};
    float background_alpha[] = {1, time-floor(time)};
    for (size_t i = 0; i < 2; i++) {
        draw_set_tex(background_textures[i]);
        for (size_t x = 0; x < ceil(w/(double)background_width); x++) {
            for (size_t y = 0; y < ceil(h/(double)background_width)+1; y++) {
                float pos[] = {x*background_width, y*background_height};
                pos[1] -= ((int)background_scroll) % background_height;
                float size[] = {background_width, background_height};
                draw_add_rect(pos, size, draw_rgba(1, 1, 1, background_alpha[i]));
            }
        }
        draw_set_tex(NULL);
    }
    
    {
        draw_set_tex(player_tex);
        float pos[] = {player_aabb.left, player_aabb.bottom};
        float size[] = {player_aabb.width, player_aabb.height};
        draw_add_rect(pos, size, draw_rgb(1, 1, 1));
        draw_set_tex(NULL);
    }
    
    draw_set_tex(player_proj_tex);
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        if (player_proj_used[i])
            draw_add_aabb(player_proj_aabb[i], draw_rgb(1, 1, 1));
    }
    draw_set_tex(NULL);
    
    draw_set_tex(enemy_proj_tex);
    for (size_t i = 0; i < MAX_ENEMY_PROJ; i++) {
        if (enemy_proj_used[i])
            draw_add_aabb(enemy_proj_aabb[i], draw_rgb(1, 1, 1));
    }
    draw_set_tex(NULL);
    
    draw_set_tex(ammo_pack_tex);
    for (size_t i = 0; i < MAX_AMMO_PACK; i++) {
        if (ammo_pack_used[i])
            draw_add_aabb(ammo_pack_aabb[i], draw_rgb(1, 1, 1));
    }
    draw_set_tex(NULL);
    
    draw_set_tex(hp_pack_tex);
    for (size_t i = 0; i < MAX_HP_PACK; i++) {
        if (hp_pack_used[i])
            draw_add_aabb(hp_pack_aabb[i], draw_rgb(1, 1, 1));
    }
    draw_set_tex(NULL);
    
    draw_set_tex(enemy_tex);
    for (size_t i = 0; i < MAX_ENEMIES; i++) {        
        if (enemy_used[i])
            draw_add_aabb(enemy_aabb[i], draw_rgb(1, 1, 1));
    }
    draw_set_tex(NULL);
    
    {
        float pos[] = {0, 0};
        float size[] = {20, 100};
        draw_col_t col = draw_rgba(0.5, 0.5, 0.25, 0.5);
        draw_add_rect(pos, size, col);
        for (size_t i = 0; i < 3; i++) col.rgba[i] *= 2;
        size[1] *= player_hp;
        draw_add_rect(pos, size, col);
    }
    
    {
        float pos[] = {20, 0};
        float size[] = {20, 100};
        draw_col_t col = draw_rgba(0.5, 0.25, 0.5, 0.5);
        draw_add_rect(pos, size, col);
        for (size_t i = 0; i < 3; i++) col.rgba[i] *= 2;
        size[1] *= player_ammo;
        draw_add_rect(pos, size, col);
    }
    
    {
        char text[256];
        snprintf(text, sizeof(text), "Hit: %zu", player_hit);
        float pos[] = {WINDOW_WIDTH-strlen(text)*BUILTIN_FONT_WIDTH, 0};
        draw_text(text, pos, draw_rgb(0.5, 1, 1), BUILTIN_FONT_HEIGHT);
    }
    
    if (player_hit + player_miss) {
        char text[256];
        snprintf(text, sizeof(text), "Accuracy: %.0f%c",
                 player_hit/(double)(player_hit+player_miss)*100, '%');
        float pos[] = {WINDOW_WIDTH-strlen(text)*BUILTIN_FONT_WIDTH, BUILTIN_FONT_HEIGHT};
        draw_text(text, pos, draw_rgb(0.5, 1, 1), BUILTIN_FONT_HEIGHT);
    }
    
    draw_end();
    
    background_scroll += frametime * BACKGROUND_SCROLL_SPEED;
}
