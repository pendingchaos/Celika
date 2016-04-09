#pragma once
#ifndef STR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int utf8_len(const uint8_t* str);
int utf8_size(const uint8_t* str);
size_t utf32_len(const uint32_t* str);
size_t utf32_size(const uint32_t* str);
uint32_t* utf8_to_utf32(const uint8_t* utf8);
uint8_t* utf32_to_utf8(const uint32_t* utf32);
bool utf8_is_ascii(const uint8_t* str);

#endif
