#pragma once
#ifndef STR_FORMAT_H
#define STR_FORMAT_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

int utf32_vscan(const uint32_t* str, const uint32_t* fmt, va_list list);
int utf32_scan(const uint32_t* str, const uint32_t* fmt, ...);
int utf32_vformat(uint32_t* dest, size_t dest_count, const uint32_t* fmt, va_list list);
int utf32_format(uint32_t* dest, size_t dest_count, const uint32_t* fmt, ...);
uint32_t* utf32_format_double(double val, int width, int precision);
uint32_t* utf32_format_int(int val);
#endif
