#pragma once
#ifndef STR_H

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

int utf8_len(const uint8_t* str);
int utf8_size(const uint8_t* str);
size_t utf32_len(const uint32_t* str);
size_t utf32_size(const uint32_t* str);
uint32_t* utf8_to_utf32(const uint8_t* utf8);
uint8_t* utf32_to_utf8(const uint32_t* utf32);
bool utf8_is_ascii(const uint8_t* str);
int utf32_vscan(const uint32_t* str, const uint32_t* fmt, va_list list);
int utf32_scan(const uint32_t* str, const uint32_t* fmt, ...);
int utf32_vformat(uint32_t* dest, size_t dest_count, const uint32_t* fmt, va_list list);
int utf32_format(uint32_t* dest, size_t dest_count, const uint32_t* fmt, ...);
uint32_t* utf32_format_double(double val, int width, int precision);
uint32_t* utf32_format_int(int val);
#endif
