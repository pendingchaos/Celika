#pragma once
#ifndef BUILTIN_FONT_H
#define BUILTIN_FONT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BUILTIN_FONT_WIDTH 8
#define BUILTIN_FONT_HEIGHT 16

extern const uint8_t builtin_font[128*16];

bool get_builtin_font_at(char c, size_t x, size_t y);
#endif
