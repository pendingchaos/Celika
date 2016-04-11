#pragma once
#ifndef GAME_H
#define GAME_H

#include <stddef.h>
#include <SDL2/SDL.h>

void celika_game_init(int* w, int* h);
void celika_game_event(SDL_Event event);
void celika_game_frame(size_t w, size_t h, float frametime);
void celika_game_deinit();

#endif
