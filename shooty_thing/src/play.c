#include "celika/celika.h"
#include "shooty_thing.h"
#include "shared.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <math.h>

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

static float background_scroll = 0;

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

static menu_t last_menu = MENU_MAIN;
static int last_mouse_x = WINDOW_WIDTH / 2;
static int last_mouse_y = WINDOW_HEIGHT / 2;

static float req_enemy_count;

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
    if (last_menu != menu && menu==MENU_NONE)
        SDL_WarpMouseInWindow(celika_window, last_mouse_x, last_mouse_y);
    
    int mx, my;
    bool fire = SDL_GetMouseState(&mx, &my) & SDL_BUTTON_LMASK;
    
    last_mouse_x = mx;
    last_mouse_y = my;
    
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
                case COLLECT_TYPE_MAX:
                    assert(false);
                    break;
                }
            }
        }
    }
}

static void setup_state();
static void cleanup_state();

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
        cleanup_state();
        setup_state();
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
}

static void cleanup_state() {
    for (size_t i = 0; i < COLLECT_TYPE_MAX; i++)
        list_free(collectables[i]);
    list_free(enemies);
    list_free(enemy_proj);
    list_free(player_proj);
}

void play_init() {
    srand(time(NULL));
    
    player_aabb = draw_get_tex_aabb(player_tex);
    
    setup_state();
}

void play_deinit() {
    cleanup_state();
}

void play_frame(size_t w, size_t h, float frametime) {
    if (menu == MENU_NONE) update(frametime);
    
    if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_ESCAPE] && menu==MENU_NONE)
        menu = MENU_PAUSE;
    if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_PAUSE] && menu==MENU_NONE)
        menu = MENU_PAUSE;
    
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
    
    background_scroll += frametime * BACKGROUND_SCROLL_SPEED;
    
    {
        draw_set_tex(player_tex);
        draw_add_aabb(player_aabb, draw_rgb(1, 1, 1));
        
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
        uint32_t* utf32 = utf8_to_utf32((uint8_t*)text);
        draw_text_font(font, utf32, pos, draw_rgb(0.5, 1, 1), 16);
        free(utf32);
    }
    
    if (player_hit + player_miss) {
        char text[256];
        snprintf(text, sizeof(text), "Accuracy: %.0f%c",
                 player_hit/(double)(player_hit+player_miss)*100, '%');
        float pos[] = {WINDOW_WIDTH-draw_text_width(text, 16), 16};
        uint32_t* utf32 = utf8_to_utf32((uint8_t*)text);
        draw_text_font(font, utf32, pos, draw_rgb(0.5, 1, 1), 16);
        free(utf32);
    }
    
    draw_fb_t* res = draw_prims_fb();
    
    draw_effect_param_fb(passthough_effect, "tex", res);
    draw_do_effect(passthough_effect);
    
    draw_free_fb(res);
    
    last_menu = menu;
}
