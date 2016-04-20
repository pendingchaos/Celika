#include "celika/celika.h"
#include "shooty_thing.h"
#include "shared.h"

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

menu_t menu;
draw_tex_t* background_tex[6];
draw_tex_t* player_tex;
draw_tex_t* player_proj_tex;
draw_tex_t* enemy_proj_tex;
draw_tex_t* enemy_tex[4];
draw_tex_t* collectable_tex[COLLECT_TYPE_MAX];
draw_tex_t* shield_tex;
draw_tex_t* button_tex;
sound_t* laser_sounds[2];
sound_t* lose_sound;
sound_t* shield_up_sound;
sound_t* shield_down_sound;
sound_t* hp_pickup_sound;
sound_t* ammo_pickup_sound;
draw_effect_t* passthough_effect;
font_t* font;

void play_init();
void play_deinit();
void play_event(SDL_Event event);
void play_frame(size_t w, size_t h, float frametime);
void mainmenu_frame(size_t w, size_t h, float frametime);
void pause_frame(size_t w, size_t h, float frametime);

void celika_game_init(int* w, int* h) {
    *w = WINDOW_WIDTH;
    *h = WINDOW_HEIGHT;
    
    player_tex = draw_create_tex_scaled(PLAYER_TEX, 0, 30, NULL, NULL, 0);
    enemy_tex[0] = draw_create_tex_scaled(ENEMY_TEX0, 0, 30, NULL, NULL, 0);
    enemy_tex[1] = draw_create_tex_scaled(ENEMY_TEX1, 0, 30, NULL, NULL, 0);
    enemy_tex[2] = draw_create_tex_scaled(ENEMY_TEX2, 0, 30, NULL, NULL, 0);
    enemy_tex[3] = draw_create_tex_scaled(ENEMY_TEX3, 0, 30, NULL, NULL, 0);
    player_proj_tex = draw_create_tex_scaled(PLAYER_PROJ_TEX, 0, 25, NULL, NULL, 0);
    enemy_proj_tex = draw_create_tex_scaled(ENEMY_PROJ_TEX, 0, 25, NULL, NULL, 0);
    collectable_tex[COLLECT_TYPE_HP] = draw_create_tex_scaled(HP_COLLECTABLE_TEX, 0, 25, NULL, NULL, 0);
    collectable_tex[COLLECT_TYPE_AMMO] = draw_create_tex_scaled(AMMO_COLLECTABLE_TEX, 0, 25, NULL, NULL, 0);
    collectable_tex[COLLECT_TYPE_SHIELD0] = draw_create_tex_scaled(SHIELD0_COLLECTABLE_TEX, 0, 25, NULL, NULL, 0);
    collectable_tex[COLLECT_TYPE_SHIELD1] = draw_create_tex_scaled(SHIELD1_COLLECTABLE_TEX, 0, 25, NULL, NULL, 0);
    collectable_tex[COLLECT_TYPE_SHIELD2] = draw_create_tex_scaled(SHIELD2_COLLECTABLE_TEX, 0, 25, NULL, NULL, 0);
    shield_tex = draw_create_tex_scaled(SHIELD_TEX, 0, 43, NULL, NULL, 0);
    button_tex = draw_create_tex_scaled(BUTTON_TEX, WINDOW_WIDTH*0.6, 0, NULL, NULL, 0);
    background_tex[0] = draw_create_tex(BG0_TEX, NULL, NULL, 0);
    background_tex[1] = draw_create_tex(BG1_TEX, NULL, NULL, 0);
    background_tex[2] = draw_create_tex(BG2_TEX, NULL, NULL, 0);
    background_tex[3] = draw_create_tex(BG3_TEX, NULL, NULL, 0);
    background_tex[4] = background_tex[2];
    background_tex[5] = background_tex[1];
    
    laser_sounds[0] = audio_create_sound(LASER0_SOUND);
    laser_sounds[1] = audio_create_sound(LASER1_SOUND);
    lose_sound = audio_create_sound(LOSE_SOUND);
    shield_up_sound = audio_create_sound(SHIELDUP_SOUND);
    shield_down_sound = audio_create_sound(SHIELDDOWN_SOUND);
    hp_pickup_sound = audio_create_sound(HP_PICKUP_SOUND);
    ammo_pickup_sound = audio_create_sound(AMMO_PICKUP_SOUND);
    
    passthough_effect = draw_create_effect("shaders/passthough.glsl");
    
    font = font_create(FONT);
    
    menu = MENU_MAIN;
    play_init();
}

void celika_game_deinit() {
    play_deinit();
    
    font_del(font);
    
    draw_del_effect(passthough_effect);
    
    audio_del_sound(ammo_pickup_sound);
    audio_del_sound(hp_pickup_sound);
    audio_del_sound(shield_down_sound);
    audio_del_sound(shield_up_sound);
    audio_del_sound(lose_sound);
    audio_del_sound(laser_sounds[0]);
    audio_del_sound(laser_sounds[1]);
    
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

void celika_game_event(SDL_Event event) {
    play_event(event);
}

void celika_game_frame(size_t w, size_t h, float frametime) {
    gui_begin_frame(w, h);
    
    play_frame(w, h, frametime);
    
    switch (menu) {
    case MENU_NONE:
        break;
    case MENU_MAIN:
        mainmenu_frame(w, h, frametime);
        break;
    case MENU_PAUSE:
        pause_frame(w, h, frametime);
        break;
    }
    
    gui_end_frame();
    
    celika_set_title(U"Shooty Thing - %.0f fps", 1/celika_get_display_frametime());
}
