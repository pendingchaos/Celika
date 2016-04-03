#include "game.h"
#include "draw.h"
#include "builtinfont.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define PLAYER_WIDTH player_width
#define PLAYER_HEIGHT 30
#define PLAYER_PROJ_WIDTH player_proj_width
#define PLAYER_PROJ_HEIGHT 25
#define ENEMY_PROJ_WIDTH enemy_proj_width
#define ENEMY_PROJ_HEIGHT 25
#define ENEMY_WIDTH enemy_width
#define ENEMY_HEIGHT 30
#define AMMO_PACK_WIDTH ammo_pack_width
#define AMMO_PACK_HEIGHT 25
#define HP_PACK_WIDTH hp_pack_width
#define HP_PACK_HEIGHT 25
#define PLAYER_FIRE_INTERVAL 0.1
#define PLAYER_PROJ_SPEED 400
#define MAX_PLAYER_PROJ 13 //(int)(500.0/PLAYER_PROJ_SPEED/PLAYER_FIRE_INTERVAL)
#define MAX_ENEMIES 20
#define ENEMY_SPEED 200
#define ENEMY_FIRE_INTERVAL 0.5
#define ENEMY_PROJ_SPEED 400
#define ENEMY_PROJ_HIT_DAMAGE 0.025
#define MAX_ENEMY_PROJ 40 //(int)(500.0/PLAYER_PROJ_SPEED/PLAYER_FIRE_INTERVAL) * MAX_ENEMIES
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
static int background_tex[6];
static int background_width;
static int background_height;
static float background_scroll;
static int player_width;
static int player_tex;
static float player_posx, player_posy;
static float player_fire_timeout;
static float player_hp;
static float player_ammo;
static int player_proj_width;
static int player_proj_tex;
static bool player_proj_used[MAX_PLAYER_PROJ];
static float player_proj_x[MAX_PLAYER_PROJ];
static float player_proj_y[MAX_PLAYER_PROJ];
static int enemy_proj_width;
static int enemy_proj_tex;
static bool enemy_proj_used[MAX_ENEMY_PROJ];
static float enemy_proj_x[MAX_ENEMY_PROJ];
static float enemy_proj_y[MAX_ENEMY_PROJ];
static int enemy_width;
static int enemy_tex;
static bool enemy_used[MAX_ENEMIES];
static float enemy_x[MAX_ENEMIES];
static float enemy_y[MAX_ENEMIES];
static float enemy_vel_x[MAX_ENEMIES];
static float enemy_fire_timeout[MAX_ENEMIES];
static float enemy_count;
static int ammo_pack_width;
static int ammo_pack_tex;
static float ammo_pack_x[MAX_AMMO_PACK];
static float ammo_pack_y[MAX_AMMO_PACK];
static bool ammo_pack_used[MAX_AMMO_PACK];
static int hp_pack_width;
static int hp_pack_tex;
static float hp_pack_x[MAX_HP_PACK];
static float hp_pack_y[MAX_HP_PACK];
static bool hp_pack_used[MAX_HP_PACK];
static size_t player_hit;
static size_t player_miss;

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

static void create_enemy_proj(size_t i) {
    for (size_t i = 0; i < MAX_ENEMY_PROJ; i++) {
        if (!enemy_proj_used[i]) {
            enemy_proj_x[i] = enemy_x[i];
            enemy_proj_y[i] = enemy_y[i];
            enemy_proj_used[i] = true;
            return;
        }
    }
}

static void create_enemy() {
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        if (!enemy_used[i]) {
            enemy_x[i] = rand() % 500;
            enemy_y[i] = 500;
            enemy_vel_x[i] = enemy_x[i] < 250 ? 1 : -1;
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
            ammo_pack_x[i] = rand() % 500;
            ammo_pack_y[i] = 500;
            ammo_pack_used[i] = true;
            return;
        }
    }
}

static void create_hp_pack() {
    for (size_t i = 0; i < MAX_HP_PACK; i++) {
        if (!hp_pack_used[i]) {
            hp_pack_x[i] = rand() % 500;
            hp_pack_y[i] = 500;
            hp_pack_used[i] = true;
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
    
    if (fire && player_fire_timeout<=0 && player_ammo>0) {
        create_player_proj();
        player_fire_timeout = PLAYER_FIRE_INTERVAL;
        player_ammo -= PLAYER_PROJ_AMMO_USAGE;
    }
    
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        if (!player_proj_used[i]) continue;
        player_proj_y[i] += frametime * PLAYER_PROJ_SPEED;
        if (player_proj_y[i] > 500) {
            player_proj_used[i] = false;
            player_miss++;
        }
    }
    
    for (size_t i = 0; i < MAX_ENEMY_PROJ; i++) {
        if (!enemy_proj_used[i]) continue;
        enemy_proj_y[i] -= frametime * ENEMY_PROJ_SPEED;
        if (enemy_proj_y[i] < 0)
            enemy_proj_used[i] = false;
        
        float px = enemy_proj_x[i];
        float py = enemy_proj_y[i] + ENEMY_PROJ_HEIGHT/2;
        float playery = player_posy + PLAYER_HEIGHT/2;
        if (fabs(px-player_posx) < (PLAYER_WIDTH+ENEMY_PROJ_WIDTH)/2 &&
            fabs(py-playery) < (PLAYER_HEIGHT+ENEMY_PROJ_HEIGHT)/2) {
            enemy_proj_used[i] = false;
            player_hp -= ENEMY_PROJ_HIT_DAMAGE;
        }
    }
    
    for (size_t i = 0; i < MAX_ENEMIES; i++) {
        if (!enemy_used[i]) continue;
        
        enemy_y[i] -= frametime * ENEMY_SPEED;
        enemy_x[i] += enemy_vel_x[i] * frametime;
        if (enemy_y[i]+ENEMY_HEIGHT < 0) {
            enemy_used[i] = false;
            create_enemy();
            continue;
        }
        
        float ex = enemy_x[i];
        float ey = enemy_y[i] + ENEMY_HEIGHT/2;
        float px = player_posx;
        float py = player_posy + PLAYER_HEIGHT/2;
        if (fabs(ex-px) < (PLAYER_WIDTH+ENEMY_WIDTH)/2 &&
            fabs(ey-py) < (PLAYER_HEIGHT+ENEMY_HEIGHT)/2) {
            enemy_used[i] = false;
            create_enemy();
            player_hp -= ENEMY_HIT_DAMAGE;
        }
        
        enemy_fire_timeout[i] -= frametime;
        if (abs(ex-player_posx)<ENEMY_FIRE_DIST_THRESHOLD &&
            enemy_fire_timeout[i]<=0) {
            create_enemy_proj(i);
            enemy_fire_timeout[i] = ENEMY_FIRE_INTERVAL;
        }
        
        for (size_t j = 0; j < MAX_PLAYER_PROJ; j++) {
            float px = player_proj_x[j];
            float py = player_proj_y[j] + PLAYER_PROJ_HEIGHT/2;
            if (fabs(ex-px) < (PLAYER_PROJ_WIDTH+ENEMY_WIDTH)/2 &&
                fabs(ey-py) < (PLAYER_PROJ_HEIGHT+ENEMY_HEIGHT)/2 &&
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
        
        ammo_pack_y[i] -= frametime * AMMO_PACK_SPEED;
        if (ammo_pack_y[i]+AMMO_PACK_HEIGHT < 0) {
            ammo_pack_used[i] = false;
            continue;
        }
        
        float ax = ammo_pack_x[i];
        float ay = ammo_pack_y[i] + AMMO_PACK_HEIGHT/2;
        float px = player_posx;
        float py = player_posy + PLAYER_HEIGHT/2;
        if (fabs(ax-px) < (PLAYER_WIDTH+AMMO_PACK_WIDTH)/2 &&
            fabs(ay-py) < (PLAYER_HEIGHT+AMMO_PACK_HEIGHT)/2) {
            ammo_pack_used[i] = false;
            player_ammo += AMMO_PACK_INC;
        }
    }
    
    for (size_t i = 0; i < MAX_HP_PACK; i++) {
        if (!hp_pack_used[i]) continue;
        
        hp_pack_y[i] -= frametime * HP_PACK_SPEED;
        if (hp_pack_y[i]+HP_PACK_HEIGHT < 0) {
            hp_pack_used[i] = false;
            continue;
        }
        
        float hx = hp_pack_x[i];
        float hy = hp_pack_y[i] + HP_PACK_HEIGHT/2;
        float px = player_posx;
        float py = player_posy + PLAYER_HEIGHT/2;
        if (fabs(hx-px) < (PLAYER_WIDTH+HP_PACK_WIDTH)/2 &&
            fabs(hy-py) < (PLAYER_HEIGHT+HP_PACK_HEIGHT)/2) {
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

void shooty_thing_game_init() {
    srand(time(NULL));
    player_posx = 250;
    player_posy = 250;
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
    
    int h;
    player_tex = draw_create_tex("SpaceShooterRedux/PNG/playerShip3_green.png",
                                 &player_width, &h);
    player_width *= PLAYER_HEIGHT/(double)h;
    
    enemy_tex = draw_create_tex("SpaceShooterRedux/PNG/ufoRed.png",
                                &enemy_width, &h);
    enemy_width *= ENEMY_HEIGHT/(double)h;
    
    player_proj_tex = draw_create_tex("SpaceShooterRedux/PNG/Lasers/laserGreen10.png",
                                      &player_proj_width, &h);
    player_proj_width *= PLAYER_PROJ_HEIGHT/(double)h;
    
    enemy_proj_tex = draw_create_tex("SpaceShooterRedux/PNG/Lasers/laserRed16.png",
                                      &enemy_proj_width, &h);
    enemy_proj_width *= ENEMY_PROJ_HEIGHT/(double)h;
    
    hp_pack_tex = draw_create_tex("SpaceShooterRedux/PNG/Power-ups/pill_yellow.png",
                                  &hp_pack_width, &h);
    hp_pack_width *= HP_PACK_HEIGHT/(double)h;
    
    ammo_pack_tex = draw_create_tex("SpaceShooterRedux/PNG/Power-ups/bolt_gold.png",
                                    &ammo_pack_width, &h);
    ammo_pack_width *= AMMO_PACK_HEIGHT/(double)h;
    
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
}

void shooty_thing_game_deinit() {
    for (size_t i = 0; i < 4; i++)
        draw_del_tex(background_tex[i]);
    draw_del_tex(ammo_pack_tex);
    draw_del_tex(hp_pack_tex);
    draw_del_tex(player_proj_tex);
    draw_del_tex(enemy_tex);
    draw_del_tex(player_tex);
}

void shooty_thing_game_frame(size_t w, size_t h, float frametime) {
    if (state == STATE_PLAYING)
        update(frametime);
    else if (state == STATE_LOST) {
        state = STATE_PLAYING;
        shooty_thing_game_deinit();
        shooty_thing_game_init();
    }
    
    draw_begin(w, h);
    
    float time = background_scroll / (double)BACKGROUND_SCROLL_SPEED;
    time *= BACKGROUND_CHANGE_SPEED;
    size_t background_textures[] = {background_tex[(int)floor(time) % 6],
                                    background_tex[(int)ceil(time) % 6]};
    float background_alpha[] = {1, time-floor(time)};
    for (size_t i = 0; i < 2; i++) {
        draw_set_tex(background_textures[i]);
        for (size_t x = 0; x < ceil(w/(double)background_width); x++) {
            for (size_t y = 0; y < ceil(h/(double)background_width)+1; y++) {
                float pos[] = {x*background_width, y*background_height};
                pos[1] -= ((int)background_scroll) % background_height;
                float size[] = {background_width, background_height};
                float col[] = {1, 1, 1, background_alpha[i]};
                draw_add_rect(pos, size, col);
            }
        }
        draw_set_tex(0);
    }
    
    {
        draw_set_tex(player_tex);
        float pos[] = {player_posx-PLAYER_WIDTH/2,
                       player_posy};
        float size[] = {PLAYER_WIDTH, PLAYER_HEIGHT};
        float col[] = {1, 1, 1, 1};
        draw_add_rect(pos, size, col);
        draw_set_tex(0);
    }
    
    draw_set_tex(player_proj_tex);
    for (size_t i = 0; i < MAX_PLAYER_PROJ; i++) {
        if (player_proj_used[i]) {
            float x = player_proj_x[i];
            float y = player_proj_y[i];
            float pos[] = {x-PLAYER_PROJ_WIDTH/2, y};
            float size[] = {PLAYER_PROJ_WIDTH, PLAYER_PROJ_HEIGHT};
            float col[] = {1, 1, 1, 1};
            draw_add_rect(pos, size, col);
        }
    }
    draw_set_tex(0);
    
    draw_set_tex(enemy_proj_tex);
    for (size_t i = 0; i < MAX_ENEMY_PROJ; i++) {
        if (enemy_proj_used[i]) {
            float x = enemy_proj_x[i];
            float y = enemy_proj_y[i];
            float pos[] = {x-ENEMY_PROJ_WIDTH/2, y-ENEMY_PROJ_HEIGHT};
            float size[] = {ENEMY_PROJ_WIDTH, ENEMY_PROJ_HEIGHT};
            float col[] = {1, 1, 1, 1};
            draw_add_rect(pos, size, col);
        }
    }
    draw_set_tex(0);
    
    draw_set_tex(ammo_pack_tex);
    for (size_t i = 0; i < MAX_AMMO_PACK; i++) {
        if (ammo_pack_used[i]) {
            float x = ammo_pack_x[i];
            float y = ammo_pack_y[i];
            float pos[] = {x-AMMO_PACK_WIDTH/2, y};
            float size[] = {AMMO_PACK_WIDTH, AMMO_PACK_HEIGHT};
            float col[] = {1, 1, 1, 1};
            draw_add_rect(pos, size, col);
        }
    }
    draw_set_tex(0);
    
    draw_set_tex(hp_pack_tex);
    for (size_t i = 0; i < MAX_HP_PACK; i++) {
        if (hp_pack_used[i]) {
            float x = hp_pack_x[i];
            float y = hp_pack_y[i];
            float pos[] = {x-HP_PACK_WIDTH/2, y};
            float size[] = {HP_PACK_WIDTH, HP_PACK_HEIGHT};
            float col[] = {1, 1, 1, 1};
            draw_add_rect(pos, size, col);
        }
    }
    draw_set_tex(0);
    
    draw_set_tex(enemy_tex);
    for (size_t i = 0; i < MAX_ENEMIES; i++) {        
        if (enemy_used[i]) {
            float x = enemy_x[i];
            float y = enemy_y[i];
            float pos[] = {x-ENEMY_WIDTH/2, y};
            float size[] = {ENEMY_WIDTH, ENEMY_HEIGHT};
            float col[] = {1, 1, 1, 1};
            draw_add_rect(pos, size, col);
        }
    }
    draw_set_tex(0);
    
    {
        float pos[] = {0, 0};
        float size[] = {20, 100};
        float col[] = {0.5, 0.5, 0.25, 0.5};
        draw_add_rect(pos, size, col);
        for (size_t i = 0; i < 3; i++) col[i] *= 2;
        size[1] *= player_hp;
        draw_add_rect(pos, size, col);
    }
    
    {
        float pos[] = {20, 0};
        float size[] = {20, 100};
        float col[] = {0.5, 0.25, 0.5, 0.5};
        draw_add_rect(pos, size, col);
        for (size_t i = 0; i < 3; i++) col[i] *= 2;
        size[1] *= player_ammo;
        draw_add_rect(pos, size, col);
    }
    
    {
        char text[256];
        snprintf(text, sizeof(text), "Hit: %zu", player_hit);
        float pos[] = {500-strlen(text)*BUILTIN_FONT_WIDTH, 0};
        float col[] = {0.5, 1, 1, 1};
        draw_text(text, pos, col, BUILTIN_FONT_HEIGHT);
    }
    
    if (player_hit + player_miss) {
        char text[256];
        snprintf(text, sizeof(text), "Accuracy: %.0f%c",
                 player_hit/(double)(player_hit+player_miss)*100, '%');
        float pos[] = {500-strlen(text)*BUILTIN_FONT_WIDTH, BUILTIN_FONT_HEIGHT};
        float col[] = {0.5, 1, 1, 1};
        draw_text(text, pos, col, BUILTIN_FONT_HEIGHT);
    }
    
    draw_end();
    
    background_scroll += frametime * BACKGROUND_SCROLL_SPEED;
}
