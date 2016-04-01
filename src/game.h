#pragma once
#ifndef GAME_H
#define GAME_H

#include <stddef.h>
void snake_game_init();
void snake_game_frame(size_t w, size_t h, float frametime);
void snake_game_deinit();
void shooty_thing_game_init();
void shooty_thing_game_frame(size_t w, size_t h, float frametime);
void shooty_thing_game_deinit();

#if 0
#define game_init snake_game_init
#define game_frame snake_game_frame
#define game_deinit snake_game_deinit
#else
#define game_init shooty_thing_game_init
#define game_frame shooty_thing_game_frame
#define game_deinit shooty_thing_game_deinit
#endif

#endif
