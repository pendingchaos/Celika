#pragma once
#ifndef GAME_H
#define GAME_H

#include <stddef.h>

void game_init();
void game_frame(size_t w, size_t h, float frametime);
void game_deinit();

#endif
