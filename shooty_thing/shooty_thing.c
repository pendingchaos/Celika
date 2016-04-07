#include "celika/celika.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500

#define ENEMY_TEX_COUNT 4

#define PLAYER_TEX "SpaceShooterRedux/PNG/playerShip3_green.png"
#define ENEMY_TEX0 "SpaceShooterRedux/PNG/ufoRed.png"
#define ENEMY_TEX1 "SpaceShooterRedux/PNG/ufoGreen.png"
#define ENEMY_TEX2 "SpaceShooterRedux/PNG/ufoBlue.png"
#define ENEMY_TEX3 "SpaceShooterRedux/PNG/ufoYellow.png"
#define PLAYER_PROJ_TEX "SpaceShooterRedux/PNG/Lasers/laserGreen10.png"
#define ENEMY_PROJ_TEX "SpaceShooterRedux/PNG/Lasers/laserRed16.png"
#define HP_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/pill_yellow.png"
#define AMMO_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/bolt_gold.png"
#define SHIELD0_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/shield_bronze.png"
#define SHIELD1_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/shield_silver.png"
#define SHIELD2_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/shield_gold.png"
#define SHIELD_TEX "SpaceShooterRedux/PNG/Effects/shield1.png"
#define BG0_TEX "SpaceShooterRedux/Backgrounds/black.png"
#define BG1_TEX "SpaceShooterRedux/Backgrounds/blue.png"
#define BG2_TEX "SpaceShooterRedux/Backgrounds/darkPurple.png"
#define BG3_TEX "SpaceShooterRedux/Backgrounds/purple.png"

#define LASER0_SOUND "SpaceShooterRedux/Bonus/sfx_laser1.ogg"
#define LASER1_SOUND "SpaceShooterRedux/Bonus/sfx_laser2.ogg"
#define LOSE_SOUND "SpaceShooterRedux/Bonus/sfx_lose.ogg"
#define SHIELDUP_SOUND "SpaceShooterRedux/Bonus/sfx_shieldUp.ogg"
#define SHIELDDOWN_SOUND "SpaceShooterRedux/Bonus/sfx_shieldDown.ogg"
#define HP_PICKUP_SOUND "SpaceShooterRedux/Bonus/sfx_twoTone.ogg"
#define AMMO_PICKUP_SOUND "SpaceShooterRedux/Bonus/sfx_zap.ogg"

#define BACKGROUND_SCROLL_SPEED 128
#define BACKGROUND_CHANGE_SPEED 0.1

#define PLAYER_FIRE_INTERVAL 0.1

#define PLAYER_PROJ_SPEED 400
#define PLAYER_PROJ_AMMO_USAGE 0.01

#define ENEMY_SPEED 200
#define ENEMY_FIRE_INTERVAL 0.8
#define ENEMY_HIT_DAMAGE 0.05
#define ENEMY_FIRE_DIST_THRESHOLD 20
#define ENEMY_FADE_RATE 10
#define MAX_ENEMIES 15

#define ENEMY_PROJ_SPEED 400
#define ENEMY_PROJ_HIT_DAMAGE 0.025

#define ENEMY_COUNT_UPDATE_RATE 0.25

#define AMMO_COLLECTABLE_CHANCE 0.3
#define AMMO_COLLECTABLE_SPEED 100
#define AMMO_COLLECTABLE_INC 0.25
#define AMMO_COLLECTABLE_FADE_RATE 10

#define HP_COLLECTABLE_CHANCE 0.2
#define HP_COLLECTABLE_SPEED 100
#define HP_COLLECTABLE_INC 0.1
#define HP_COLLECTABLE_FADE_RATE 10

#define SHIELD0_COLLECTABLE_SPEED 100
#define SHIELD0_COLLECTABLE_FADE_RATE 10
#define SHIELD0_COLLECTABLE_CHANCE 0.1
#define SHIELD1_COLLECTABLE_SPEED 100
#define SHIELD1_COLLECTABLE_FADE_RATE 10
#define SHIELD1_COLLECTABLE_CHANCE 0.05
#define SHIELD2_COLLECTABLE_SPEED 100
#define SHIELD2_COLLECTABLE_FADE_RATE 10
#define SHIELD2_COLLECTABLE_CHANCE 0.025
#define SHIELD_TIMEOUT 10

typedef enum state_t {
    STATE_PLAYING,
    STATE_LOST
} state_t;

typedef enum collectable_type_t {
    COLLECT_TYPE_AMMO,
    COLLECT_TYPE_HP,
    COLLECT_TYPE_SHIELD0,
    COLLECT_TYPE_SHIELD1,
    COLLECT_TYPE_SHIELD2,
    COLLECT_TYPE_MAX
} collectable_type_t;

typedef enum shield_strength_t {
    SHIELD_NONE,
    SHIELD_WEAK,
    SHIELD_GOOD,
    SHIELD_STRONG,
    SHIELD_STRENGTH_MAX
} shield_strength_t;

typedef struct enemy_t {
    draw_tex_t* tex;
    aabb_t aabb;
    float velx;
    float fire_timeout;
    bool destroyed;
    float alpha;
} enemy_t;

typedef struct collectable_t {
    aabb_t aabb;
    bool taken;
    float alpha;
} collectable_t;

static const float shield_reduce[] = {
    [SHIELD_NONE] = 0,
    [SHIELD_WEAK] = 0.4,
    [SHIELD_GOOD] = 0.7,
    [SHIELD_STRONG] = 0.9
};

static draw_tex_t* background_tex[6];
static draw_tex_t* player_tex;
static draw_tex_t* player_proj_tex;
static draw_tex_t* enemy_proj_tex;
static draw_tex_t* enemy_tex[4];
static draw_tex_t* collectable_tex[COLLECT_TYPE_MAX];
static draw_tex_t* shield_tex;
static draw_effect_t* passthough_effect;

static state_t state;

static float background_scroll;

static aabb_t player_aabb;
static float player_fire_timeout;
static float player_hp;
static float player_ammo;
static size_t player_hit;
static size_t player_miss;
static int shield_strength;
static float shield_timeleft;

static list_t* player_proj; //list of aabb_t
static list_t* enemy_proj; //list of aabb_t
static list_t* enemies; //list of enemy_t
static list_t* collectables[COLLECT_TYPE_MAX]; //list of collectable_t

static float req_enemy_count;

static sound_t* laser_sounds[2];
static sound_t* lose_sound;
static sound_t* shield_up_sound;
static sound_t* shield_down_sound;
static sound_t* hp_pickup_sound;
static sound_t* ammo_pickup_sound;

static void create_player_proj() {
    aabb_t aabb = draw_get_tex_aabb(player_proj_tex);
    aabb.left = player_aabb.left + player_aabb.width/2;
    aabb.left -= aabb.width/2;
    aabb.bottom = player_aabb.bottom + player_aabb.height;
    list_append(player_proj, &aabb);
    
    audio_play_sound(laser_sounds[rand()%2], 0.1, 0, true);
}

static void create_enemy_proj(size_t i) {
    aabb_t aabb = draw_get_tex_aabb(enemy_proj_tex);
    enemy_t* enemy = list_nth(enemies, i);
    aabb.left = enemy->aabb.left + player_aabb.width/2;
    aabb.left -= aabb.width/2;
    aabb.bottom = enemy->aabb.bottom - aabb.height;
    list_append(enemy_proj, &aabb);
    
    audio_play_sound(laser_sounds[rand()%2], 0.1, 0, true);
}

static void init_enemy(enemy_t* enemy) {
    float cx = rand() % WINDOW_WIDTH;
    enemy->tex = enemy_tex[rand()%ENEMY_TEX_COUNT];
    enemy->aabb = draw_get_tex_aabb(enemy->tex);
    enemy->aabb.left = cx - enemy->aabb.width/2;
    enemy->aabb.bottom = WINDOW_HEIGHT;
    enemy->velx = cx<WINDOW_WIDTH/2 ? 1 : -1;
    enemy->velx *= rand()%25 + 25;
    enemy->fire_timeout = 0;
    enemy->destroyed = false;
    enemy->alpha = 1;
}

static void create_enemy() {
    enemy_t enemy;
    init_enemy(&enemy);
    list_append(enemies, &enemy);
}

static void create_collectable(collectable_type_t type) {
    collectable_t collectable;
    collectable.aabb = draw_get_tex_aabb(collectable_tex[type]);
    collectable.aabb.left = rand()%WINDOW_WIDTH - collectable.aabb.width/2;
    collectable.aabb.bottom = WINDOW_HEIGHT;
    collectable.taken = false;
    collectable.alpha = 1;
    list_append(collectables[type], &collectable);
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
    
    if (shield_strength != SHIELD_NONE) {
        shield_timeleft -= frametime;
        if (shield_timeleft <= 0) {
            shield_strength = SHIELD_NONE;
            audio_play_sound(shield_down_sound, 1, 0, true);
        }
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
            player_hp -= ENEMY_PROJ_HIT_DAMAGE * (1-shield_reduce[shield_strength]);
        }
    }
}

static void update_enemies(float frametime) {
    for (ptrdiff_t i = 0; i < list_len(enemies); i++) {
        enemy_t* enemy = list_nth(enemies, i);
        
        enemy->aabb.bottom -= frametime * ENEMY_SPEED;
        enemy->aabb.left += enemy->velx * frametime;
        if (aabb_top(enemy->aabb) < 0)
            init_enemy(enemy);
        
        if (enemy->destroyed) {
            enemy->alpha -= ENEMY_FADE_RATE * frametime;
            if (enemy->alpha <= 0) {
                list_remove(enemy);
                i--;
            }
            continue;
        }
        
        if (intersect_aabb(enemy->aabb, player_aabb)) {
            enemy->destroyed = true;
            player_hp -= ENEMY_HIT_DAMAGE * (1-shield_reduce[shield_strength]);
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
                enemy->destroyed = true;
                player_hit++;
            }
        }
    }
}

static void update_collectables(float frametime) {
    static float collectable_speed[] = {
        [COLLECT_TYPE_AMMO] = AMMO_COLLECTABLE_SPEED,
        [COLLECT_TYPE_HP] = HP_COLLECTABLE_SPEED,
        [COLLECT_TYPE_SHIELD0] = SHIELD0_COLLECTABLE_SPEED,
        [COLLECT_TYPE_SHIELD1] = SHIELD1_COLLECTABLE_SPEED,
        [COLLECT_TYPE_SHIELD2] = SHIELD2_COLLECTABLE_SPEED};
    
    static float collectable_fade_rate[] = {
        [COLLECT_TYPE_AMMO] = AMMO_COLLECTABLE_FADE_RATE,
        [COLLECT_TYPE_HP] = HP_COLLECTABLE_FADE_RATE,
        [COLLECT_TYPE_SHIELD0] = SHIELD0_COLLECTABLE_FADE_RATE,
        [COLLECT_TYPE_SHIELD1] = SHIELD1_COLLECTABLE_FADE_RATE,
        [COLLECT_TYPE_SHIELD2] = SHIELD2_COLLECTABLE_FADE_RATE};
    
    for (collectable_type_t type = 0; type < COLLECT_TYPE_MAX; type++) {
        for (ptrdiff_t i = 0; i < list_len(collectables[type]); i++) {
            collectable_t* collectable = list_nth(collectables[type], i);
            
            collectable->aabb.bottom -= frametime * collectable_speed[type];
            if (aabb_top(collectable->aabb) < 0) {
                list_remove(collectable);
                i--;
                continue;
            }
            
            if (collectable->taken) {
                collectable->alpha -= collectable_fade_rate[type] * frametime;
                if (collectable->alpha <= 0) {
                    list_remove(collectable);
                    i--;
                }
                continue;
            }
            
            if (intersect_aabb(collectable->aabb, player_aabb)) {
                collectable->taken = true;
                switch (type) {
                case COLLECT_TYPE_AMMO:
                    player_ammo += AMMO_COLLECTABLE_INC;
                    audio_play_sound(ammo_pickup_sound, 1, 0, true);
                    break;
                case COLLECT_TYPE_HP:
                    player_hp += HP_COLLECTABLE_INC;
                    audio_play_sound(hp_pickup_sound, 1, 0, true);
                    break;
                case COLLECT_TYPE_SHIELD0:
                    if (shield_strength <= SHIELD_WEAK) {
                        shield_strength = SHIELD_WEAK;
                    }
                    shield_timeleft = SHIELD_TIMEOUT;
                    audio_play_sound(shield_up_sound, 1, 0, true);
                    break;
                case COLLECT_TYPE_SHIELD1:
                    if (shield_strength <= SHIELD_GOOD) {
                        shield_strength = SHIELD_GOOD;
                    }
                    shield_timeleft = SHIELD_TIMEOUT;
                    audio_play_sound(shield_up_sound, 1, 0, true);
                    break;
                case COLLECT_TYPE_SHIELD2:
                    if (shield_strength <= SHIELD_STRONG) {
                        shield_strength = SHIELD_STRONG;
                    }
                    shield_timeleft = SHIELD_TIMEOUT;
                    audio_play_sound(shield_up_sound, 1, 0, true);
                    break;
                }
            }
        }
    }
}

static void update(float frametime) {
    update_player(frametime);
    update_proj(frametime);
    update_enemies(frametime);
    update_collectables(frametime);
    
    req_enemy_count += frametime * ENEMY_COUNT_UPDATE_RATE;
    if ((int)req_enemy_count > MAX_ENEMIES)
        req_enemy_count = MAX_ENEMIES;
    
    int enemy_count = 0;
    for (size_t i = 0; i < list_len(enemies); i++)
        enemy_count += ((enemy_t*)list_nth(enemies, i))->destroyed ? 0 : 1;
    
    for (size_t i = 0; i < ((int)req_enemy_count)-enemy_count; i++)
        create_enemy();
    
    if (rand()/(double)RAND_MAX > (1.0-AMMO_COLLECTABLE_CHANCE*frametime))
        create_collectable(COLLECT_TYPE_AMMO);
    
    if (rand()/(double)RAND_MAX > (1.0-HP_COLLECTABLE_CHANCE*frametime))
        create_collectable(COLLECT_TYPE_HP);
    
    if (rand()/(double)RAND_MAX > (1.0-SHIELD0_COLLECTABLE_CHANCE*frametime))
        create_collectable(COLLECT_TYPE_SHIELD0);
    
    if (rand()/(double)RAND_MAX > (1.0-SHIELD1_COLLECTABLE_CHANCE*frametime))
        create_collectable(COLLECT_TYPE_SHIELD1);
    
    if (rand()/(double)RAND_MAX > (1.0-SHIELD2_COLLECTABLE_CHANCE*frametime))
        create_collectable(COLLECT_TYPE_SHIELD2);
    
    if (player_hp <= 0) {
        state = STATE_LOST;
        audio_play_sound(lose_sound, 1, 0, true);
    }
    
    player_ammo = fmin(fmax(player_ammo, 0), 1);
    player_hp = fmin(fmax(player_hp, 0), 1);
}

static void setup_state() {
    player_fire_timeout = 0;
    player_hp = 1;
    player_ammo = 1;
    player_hit = 0;
    player_miss = 0;
    shield_strength = 0;
    shield_timeleft = 0;
    req_enemy_count = 1;
    
    player_proj = list_new(sizeof(aabb_t));
    enemy_proj = list_new(sizeof(aabb_t));
    enemies = list_new(sizeof(enemy_t));
    for (size_t i = 0; i < COLLECT_TYPE_MAX; i++)
        collectables[i] = list_new(sizeof(collectable_t));
    
    state = STATE_PLAYING;
}

static void cleanup_state() {
    for (size_t i = 0; i < COLLECT_TYPE_MAX; i++)
        list_free(collectables[i]);
    list_free(enemies);
    list_free(enemy_proj);
    list_free(player_proj);
}

void celika_game_init(int* w, int* h) {
    *w = WINDOW_WIDTH;
    *h = WINDOW_HEIGHT;
    
    srand(time(NULL));
    
    player_tex = draw_create_scaled_tex_aabb(PLAYER_TEX, 0, 30, &player_aabb);
    enemy_tex[0] = draw_create_scaled_tex(ENEMY_TEX0, 0, 30, NULL, NULL);
    enemy_tex[1] = draw_create_scaled_tex(ENEMY_TEX1, 0, 30, NULL, NULL);
    enemy_tex[2] = draw_create_scaled_tex(ENEMY_TEX2, 0, 30, NULL, NULL);
    enemy_tex[3] = draw_create_scaled_tex(ENEMY_TEX3, 0, 30, NULL, NULL);
    player_proj_tex = draw_create_scaled_tex(PLAYER_PROJ_TEX, 0, 25, NULL, NULL);
    enemy_proj_tex = draw_create_scaled_tex(ENEMY_PROJ_TEX, 0, 25, NULL, NULL);
    collectable_tex[COLLECT_TYPE_HP] = draw_create_scaled_tex(HP_COLLECTABLE_TEX, 0, 25, NULL, NULL);
    collectable_tex[COLLECT_TYPE_AMMO] = draw_create_scaled_tex(AMMO_COLLECTABLE_TEX, 0, 25, NULL, NULL);
    collectable_tex[COLLECT_TYPE_SHIELD0] = draw_create_scaled_tex(SHIELD0_COLLECTABLE_TEX, 0, 25, NULL, NULL);
    collectable_tex[COLLECT_TYPE_SHIELD1] = draw_create_scaled_tex(SHIELD1_COLLECTABLE_TEX, 0, 25, NULL, NULL);
    collectable_tex[COLLECT_TYPE_SHIELD2] = draw_create_scaled_tex(SHIELD2_COLLECTABLE_TEX, 0, 25, NULL, NULL);
    shield_tex = draw_create_scaled_tex(SHIELD_TEX, 0, 43, NULL, NULL);
    background_tex[0] = draw_create_tex(BG0_TEX, NULL, NULL);
    background_tex[1] = draw_create_tex(BG1_TEX, NULL, NULL);
    background_tex[2] = draw_create_tex(BG2_TEX, NULL, NULL);
    background_tex[3] = draw_create_tex(BG3_TEX, NULL, NULL);
    background_tex[4] = background_tex[2];
    background_tex[5] = background_tex[1];
    background_scroll = 0;
    
    passthough_effect = draw_create_effect("shaders/passthough.glsl");
    
    laser_sounds[0] = audio_create_sound(LASER0_SOUND);
    laser_sounds[1] = audio_create_sound(LASER1_SOUND);
    lose_sound = audio_create_sound(LOSE_SOUND);
    shield_up_sound = audio_create_sound(SHIELDUP_SOUND);
    shield_down_sound = audio_create_sound(SHIELDDOWN_SOUND);
    hp_pickup_sound = audio_create_sound(HP_PICKUP_SOUND);
    ammo_pickup_sound = audio_create_sound(AMMO_PICKUP_SOUND);
    
    setup_state();
}

void celika_game_deinit() {
    cleanup_state();
    
    audio_del_sound(ammo_pickup_sound);
    audio_del_sound(hp_pickup_sound);
    audio_del_sound(shield_down_sound);
    audio_del_sound(shield_up_sound);
    audio_del_sound(lose_sound);
    audio_del_sound(laser_sounds[0]);
    audio_del_sound(laser_sounds[1]);
    
    draw_del_effect(passthough_effect);
    
    for (size_t i = 0; i < 4; i++)
        draw_del_tex(background_tex[i]);
    draw_del_tex(shield_tex);
    for (size_t i = 0; i < COLLECT_TYPE_MAX; i++)
        draw_del_tex(collectable_tex[i]);
    draw_del_tex(enemy_proj_tex);
    draw_del_tex(player_proj_tex);
    for (size_t i = 0; i < ENEMY_TEX_COUNT; i++)
        draw_del_tex(enemy_tex[i]);
    draw_del_tex(player_tex);
}

void celika_game_frame(size_t w, size_t h, float frametime) {
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
        
        if (shield_strength != SHIELD_NONE) {
            aabb_t aabb = draw_get_tex_aabb(shield_tex);
            aabb.left = aabb_cx(player_aabb) - aabb.width/2;
            aabb.bottom = player_aabb.bottom;
            
            float alpha = 1;
            
            if (shield_timeleft > 9) alpha = (10-shield_timeleft);
            if (shield_timeleft < 1) alpha = shield_timeleft;
            
            draw_set_tex(shield_tex);
            draw_col_t col;
            switch (shield_strength) {
            case SHIELD_WEAK: col = draw_rgb(1, 0.5, 0.5); break;
            case SHIELD_GOOD: col = draw_rgb(1, 1, 0.5); break;
            case SHIELD_STRONG: col = draw_rgb(0.5, 1, 0.5); break;
            }
            draw_add_aabb(aabb, draw_rgba(col.r, col.g, col.b, alpha));
        }
        
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
    
    for (size_t i = 0; i < COLLECT_TYPE_MAX; i++) {
        draw_set_tex(collectable_tex[i]);
        for (size_t j = 0; j < list_len(collectables[i]); j++) {
            collectable_t* collectable = list_nth(collectables[i], j);
            draw_add_aabb(collectable->aabb, draw_rgba(1, 1, 1, collectable->alpha));
        }
    }
    draw_set_tex(NULL);
    
    for (size_t i = 0; i < list_len(enemies); i++) {
        enemy_t* enemy = list_nth(enemies, i);
        draw_set_tex(enemy->tex);
        draw_add_aabb(enemy->aabb, draw_rgba(1, 1, 1, enemy->alpha));
    }
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
