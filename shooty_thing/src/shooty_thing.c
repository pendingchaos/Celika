#include "celika/game.h"
#include "celika/draw.h"
#include "celika/list.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500

#define PLAYER_TEX "SpaceShooterRedux/PNG/playerShip3_green.png"
#define ENEMY_TEX "SpaceShooterRedux/PNG/ufoRed.png"
#define PLAYER_PROJ_TEX "SpaceShooterRedux/PNG/Lasers/laserGreen10.png"
#define ENEMY_PROJ_TEX "SpaceShooterRedux/PNG/Lasers/laserRed16.png"
#define HP_PACK_TEX "SpaceShooterRedux/PNG/Power-ups/pill_yellow.png"
#define AMMO_PACK_TEX "SpaceShooterRedux/PNG/Power-ups/bolt_gold.png"
#define BG0_TEX "SpaceShooterRedux/Backgrounds/black.png"
#define BG1_TEX "SpaceShooterRedux/Backgrounds/blue.png"
#define BG2_TEX "SpaceShooterRedux/Backgrounds/darkPurple.png"
#define BG3_TEX "SpaceShooterRedux/Backgrounds/purple.png"

#define BACKGROUND_SCROLL_SPEED 128
#define BACKGROUND_CHANGE_SPEED 0.1

#define PLAYER_FIRE_INTERVAL 0.1

#define PLAYER_PROJ_SPEED 400
#define PLAYER_PROJ_AMMO_USAGE 0.01

#define ENEMY_SPEED 200
#define ENEMY_FIRE_INTERVAL 0.5
#define ENEMY_HIT_DAMAGE 0.05
#define ENEMY_FIRE_DIST_THRESHOLD 30

#define ENEMY_PROJ_SPEED 400
#define ENEMY_PROJ_HIT_DAMAGE 0.025

#define ENEMY_COUNT_UPDATE_RATE 0.25

#define AMMO_PACK_CHANCE 0.3
#define AMMO_PACK_SPEED 100
#define AMMO_PACK_INC 0.25

#define HP_PACK_CHANCE 0.2
#define HP_PACK_SPEED 100
#define HP_PACK_INC 0.1

typedef enum state_t {
    STATE_PLAYING,
    STATE_LOST
} state_t;

typedef struct enemy_t {
    aabb_t aabb;
    float velx;
    float fire_timeout;
} enemy_t;

static draw_tex_t* background_tex[6];
static draw_tex_t* player_tex;
static draw_tex_t* player_proj_tex;
static draw_tex_t* enemy_proj_tex;
static draw_tex_t* enemy_tex;
static draw_tex_t* hp_pack_tex;
static draw_tex_t* ammo_pack_tex;
static draw_effect_t* passthough_effect;

static state_t state;

static float background_scroll;

static aabb_t player_aabb;
static float player_fire_timeout;
static float player_hp;
static float player_ammo;
static size_t player_hit;
static size_t player_miss;

static list_t* player_proj; //list of aabb_t
static list_t* enemy_proj; //list of aabb_t
static list_t* enemies; //list of enemy_t
static list_t* ammo_packs; //list of aabb_t
static list_t* hp_packs; //list of aabb_t

static float req_enemy_count;

static void create_player_proj() {
    aabb_t aabb = draw_get_tex_aabb(player_proj_tex);
    aabb.left = player_aabb.left + player_aabb.width/2;
    aabb.left -= aabb.width/2;
    aabb.bottom = player_aabb.bottom + player_aabb.height;
    list_append(player_proj, &aabb);
}

static void create_enemy_proj(size_t i) {
    aabb_t aabb = draw_get_tex_aabb(enemy_proj_tex);
    enemy_t* enemy = list_nth(enemies, i);
    aabb.left = enemy->aabb.left + player_aabb.width/2;
    aabb.left -= aabb.width/2;
    aabb.bottom = enemy->aabb.bottom - aabb.height;
    list_append(enemy_proj, &aabb);
}

static void init_enemy(enemy_t* enemy) {
    float cx = rand() % WINDOW_WIDTH;
    enemy->aabb = draw_get_tex_aabb(enemy_tex);
    enemy->aabb.left = cx - enemy->aabb.width/2;
    enemy->aabb.bottom = WINDOW_HEIGHT;
    enemy->velx = cx<WINDOW_WIDTH/2 ? 1 : -1;
    enemy->velx *= rand()%25 + 25;
    enemy->fire_timeout = 0;
}

static void create_enemy() {
    enemy_t enemy;
    init_enemy(&enemy);
    list_append(enemies, &enemy);
}

static void create_ammo_pack() {
    aabb_t aabb = draw_get_tex_aabb(ammo_pack_tex);
    aabb.left = rand()%WINDOW_WIDTH - aabb.width/2;
    aabb.bottom = WINDOW_HEIGHT;
    list_append(ammo_packs, &aabb);
}

static void create_hp_pack() {
    aabb_t aabb = draw_get_tex_aabb(hp_pack_tex);
    aabb.left = rand()%WINDOW_WIDTH - aabb.width/2;
    aabb.bottom = WINDOW_HEIGHT;
    list_append(hp_packs, &aabb);
}

static void update_player(float frametime) {
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
}

static void update_proj(float frametime) {
    for (size_t i = 0; i < list_len(player_proj); i++) {
        aabb_t* aabb = list_nth(player_proj, i);
        aabb->bottom += frametime * PLAYER_PROJ_SPEED;
        if (aabb->bottom > WINDOW_HEIGHT) {
            list_remove(aabb);
            i--;
            player_miss++;
        }
    }
    
    for (size_t i = 0; i < list_len(enemy_proj); i++) {
        aabb_t* aabb = list_nth(enemy_proj, i);
        
        aabb->bottom -= frametime * ENEMY_PROJ_SPEED;
        if (aabb_top(*aabb) < 0) {
            list_remove(aabb);
            i--;
            continue;
        }
        
        if (intersect_aabb(*aabb, player_aabb)) {
            list_remove(aabb);
            i--;
            player_hp -= ENEMY_PROJ_HIT_DAMAGE;
        }
    }
}

static void update_enemies(float frametime) {
    for (size_t i = 0; i < list_len(enemies); i++) {
        enemy_t* enemy = list_nth(enemies, i);
        
        enemy->aabb.bottom -= frametime * ENEMY_SPEED;
        enemy->aabb.left += enemy->velx * frametime;
        if (aabb_top(enemy->aabb) < 0)
            init_enemy(enemy);
        
        if (intersect_aabb(enemy->aabb, player_aabb)) {
            init_enemy(enemy);
            player_hp -= ENEMY_HIT_DAMAGE;
        }
        
        enemy->fire_timeout -= frametime;
        float ex = aabb_cx(enemy->aabb);
        float px = aabb_cx(player_aabb);
        if (fabs(ex-px)<ENEMY_FIRE_DIST_THRESHOLD &&
            enemy->fire_timeout<=0) {
            create_enemy_proj(i);
            enemy->fire_timeout = ENEMY_FIRE_INTERVAL;
        }
        
        for (size_t j = 0; j < list_len(player_proj); j++) {
            aabb_t* proj = list_nth(player_proj, j);
            if (intersect_aabb(enemy->aabb, *proj)) {
                list_remove(proj);
                j--;
                init_enemy(enemy);
                player_hit++;
            }
        }
    }
}

static void update_packs(float frametime) {
    for (ptrdiff_t i = 0; i < list_len(ammo_packs); i++) {
        aabb_t* ammo_pack = list_nth(ammo_packs, i);
        
        ammo_pack->bottom -= frametime * AMMO_PACK_SPEED;
        if (aabb_top(*ammo_pack) < 0) {
            list_remove(ammo_pack);
            i--;
            continue;
        }
        
        if (intersect_aabb(*ammo_pack, player_aabb)) {
            list_remove(ammo_pack);
            i--;
            player_ammo += AMMO_PACK_INC;
        }
    }
    
    for (size_t i = 0; i < list_len(hp_packs); i++) {
        aabb_t* hp_pack = list_nth(hp_packs, i);
        
        hp_pack->bottom -= frametime * HP_PACK_SPEED;
        if (aabb_top(*hp_pack) < 0) {
            list_remove(hp_pack);
            i--;
            continue;
        }
        
        if (intersect_aabb(*hp_pack, player_aabb)) {
            list_remove(hp_pack);
            i--;
            player_hp += HP_PACK_INC;
        }
    }
}

static void update(float frametime) {
    update_player(frametime);
    update_proj(frametime);
    update_enemies(frametime);
    update_packs(frametime);
    
    req_enemy_count += frametime * ENEMY_COUNT_UPDATE_RATE;
    
    for (size_t i = 0; i < ((int)req_enemy_count)-list_len(enemies); i++)
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
    req_enemy_count = 1;
    
    player_proj = list_new(sizeof(aabb_t), NULL);
    enemy_proj = list_new(sizeof(aabb_t), NULL);
    enemies = list_new(sizeof(enemy_t), NULL);
    ammo_packs = list_new(sizeof(aabb_t), NULL);
    hp_packs = list_new(sizeof(aabb_t), NULL);
    
    state = STATE_PLAYING;
}

static void cleanup_state() {
    list_free(hp_packs);
    list_free(ammo_packs);
    list_free(enemies);
    list_free(enemy_proj);
    list_free(player_proj);
}

void game_init(int* w, int* h) {
    *w = WINDOW_WIDTH;
    *h = WINDOW_HEIGHT;
    
    srand(time(NULL));
    
    player_tex = draw_create_scaled_tex_aabb(PLAYER_TEX, 0, 30, &player_aabb);
    enemy_tex = draw_create_scaled_tex(ENEMY_TEX, 0, 30, NULL, NULL);
    player_proj_tex = draw_create_scaled_tex(PLAYER_PROJ_TEX, 0, 25, NULL, NULL);
    enemy_proj_tex = draw_create_scaled_tex(ENEMY_PROJ_TEX, 0, 25, NULL, NULL);
    hp_pack_tex = draw_create_scaled_tex(HP_PACK_TEX, 0, 25, NULL, NULL);
    ammo_pack_tex = draw_create_scaled_tex(AMMO_PACK_TEX, 0, 25, NULL, NULL);
    background_tex[0] = draw_create_tex(BG0_TEX, NULL, NULL);
    background_tex[1] = draw_create_tex(BG1_TEX, NULL, NULL);
    background_tex[2] = draw_create_tex(BG2_TEX, NULL, NULL);
    background_tex[3] = draw_create_tex(BG3_TEX, NULL, NULL);
    background_tex[4] = background_tex[2];
    background_tex[5] = background_tex[1];
    background_scroll = 0;
    
    passthough_effect = draw_create_effect("shaders/passthough.glsl");
    
    setup_state();
}

void game_deinit() {
    cleanup_state();
    
    draw_del_effect(passthough_effect);
    
    for (size_t i = 0; i < 4; i++)
        draw_del_tex(background_tex[i]);
    draw_del_tex(ammo_pack_tex);
    draw_del_tex(hp_pack_tex);
    draw_del_tex(enemy_proj_tex);
    draw_del_tex(player_proj_tex);
    draw_del_tex(enemy_tex);
    draw_del_tex(player_tex);
}

void game_frame(size_t w, size_t h, float frametime) {
    if (state == STATE_PLAYING)
        update(frametime);
    else if (state == STATE_LOST) {
        cleanup_state();
        setup_state();
    }
    
    float time = background_scroll / (double)BACKGROUND_SCROLL_SPEED;
    time *= BACKGROUND_CHANGE_SPEED;
    draw_tex_t* background_textures[] = {background_tex[(int)floor(time) % 6],
                                         background_tex[(int)ceil(time) % 6]};
    float background_alpha[] = {1, time-floor(time)};
    for (size_t i = 0; i < 2; i++) {
        draw_tex_t* tex = background_textures[i];
        draw_set_tex(tex);
        aabb_t bb = draw_get_tex_aabb(tex);
        for (size_t x = 0; x < ceil(w/bb.width); x++) {
            for (size_t y = 0; y < ceil(h/bb.height)+1; y++) {
                float pos[] = {x*bb.width, y*bb.height};
                pos[1] -= ((int)background_scroll) % (int)bb.height;
                float size[] = {bb.width, bb.height};
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
    for (size_t i = 0; i < list_len(player_proj); i++)
        draw_add_aabb(*(aabb_t*)list_nth(player_proj, i), draw_rgb(1, 1, 1));
    draw_set_tex(NULL);
    
    draw_set_tex(enemy_proj_tex);
    for (size_t i = 0; i < list_len(enemy_proj); i++)
        draw_add_aabb(*(aabb_t*)list_nth(enemy_proj, i), draw_rgb(1, 1, 1));
    draw_set_tex(NULL);
    
    draw_set_tex(ammo_pack_tex);
    for (size_t i = 0; i < list_len(ammo_packs); i++)
        draw_add_aabb(*(aabb_t*)list_nth(ammo_packs, i), draw_rgb(1, 1, 1));
    draw_set_tex(NULL);
    
    draw_set_tex(hp_pack_tex);
    for (size_t i = 0; i < list_len(hp_packs); i++)
        draw_add_aabb(*(aabb_t*)list_nth(hp_packs, i), draw_rgb(1, 1, 1));
    draw_set_tex(NULL);
    
    draw_set_tex(enemy_tex);
    for (size_t i = 0; i < list_len(enemies); i++)
        draw_add_aabb(*(aabb_t*)list_nth(enemies, i), draw_rgb(1, 1, 1));
    draw_set_tex(NULL);
    
    {
        float pos[] = {0, 0};
        float size[] = {20, 100};
        draw_add_rect(pos, size, draw_rgba(0.5, 0.5, 0.25, 0.5));
        size[1] *= player_hp;
        draw_add_rect(pos, size, draw_rgb(1, 1, 0.5));
    }
    
    {
        float pos[] = {20, 0};
        float size[] = {20, 100};
        draw_add_rect(pos, size, draw_rgba(0.5, 0.25, 0.5, 0.5));
        size[1] *= player_ammo;
        draw_add_rect(pos, size, draw_rgb(1, 0.5, 1));
    }
    
    {
        char text[256];
        snprintf(text, sizeof(text), "Hit: %zu", player_hit);
        float pos[] = {WINDOW_WIDTH-draw_text_width(text, 16), 0};
        draw_text(text, pos, draw_rgb(0.5, 1, 1), 16);
    }
    
    if (player_hit + player_miss) {
        char text[256];
        snprintf(text, sizeof(text), "Accuracy: %.0f%c",
                 player_hit/(double)(player_hit+player_miss)*100, '%');
        float pos[] = {WINDOW_WIDTH-draw_text_width(text, 16), 16};
        draw_text(text, pos, draw_rgb(0.5, 1, 1), 16);
    }
    
    draw_fb_t* res = draw_prims_fb();
    
    draw_effect_param_fb(passthough_effect, "tex", res);
    draw_do_effect(passthough_effect);
    
    draw_free_fb(res);
    
    background_scroll += frametime * BACKGROUND_SCROLL_SPEED;
}
