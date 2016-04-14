#pragma once
#ifndef SHOOTY_THING
#define SHOOTY_THING
#include "celika/celika.h"

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
#define HP_COLLECTABLE_TEX "SpaceShooterRedux/Vector/sheet.svg.png"
#define AMMO_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/bolt_gold.png"
#define SHIELD0_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/shield_bronze.png"
#define SHIELD1_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/shield_silver.png"
#define SHIELD2_COLLECTABLE_TEX "SpaceShooterRedux/PNG/Power-ups/shield_gold.png"
#define SHIELD_TEX "SpaceShooterRedux/PNG/Effects/shield1.png"
#define BUTTON_TEX "SpaceShooterRedux/PNG/UI/buttonGreen.png"
#define BG0_TEX "SpaceShooterRedux/Backgrounds/black.png"
#define BG1_TEX "SpaceShooterRedux/Backgrounds/blue.png"
#define BG2_TEX "SpaceShooterRedux/Backgrounds/darkPurple.png"
#define BG3_TEX "SpaceShooterRedux/Backgrounds/purple.png"
#define FONT "SpaceShooterRedux/Bonus/kenvector_future_thin.ttf"

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

#define BUTTON_PADDING 10
#define BUTTON_TEXT_SIZE 0.6

typedef enum meun_t {
    MENU_NONE,
    MENU_MAIN,
    MENU_PAUSE
} menu_t;

typedef enum collectable_type_t {
    COLLECT_TYPE_AMMO,
    COLLECT_TYPE_HP,
    COLLECT_TYPE_SHIELD0,
    COLLECT_TYPE_SHIELD1,
    COLLECT_TYPE_SHIELD2,
    COLLECT_TYPE_MAX
} collectable_type_t;

extern menu_t menu;
extern draw_tex_t* background_tex[6];
extern draw_tex_t* player_tex;
extern draw_tex_t* player_proj_tex;
extern draw_tex_t* enemy_proj_tex;
extern draw_tex_t* enemy_tex[4];
extern draw_tex_t* collectable_tex[COLLECT_TYPE_MAX];
extern draw_tex_t* shield_tex;
extern draw_tex_t* button_tex;
extern sound_t* laser_sounds[2];
extern sound_t* lose_sound;
extern sound_t* shield_up_sound;
extern sound_t* shield_down_sound;
extern sound_t* hp_pickup_sound;
extern sound_t* ammo_pickup_sound;
extern draw_effect_t* passthough_effect;
extern font_t* font;
#endif
