//TODO: Test this more thoroughly
//TODO: Handle invalid data
#include "str.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int utf8_char_len(uint8_t first) {
    if ((first&0x80) == 0x00) return 1;
    else if ((first&0xe0) == 0xc0) return 2;
    else if ((first&0xf0) == 0xe0) return 3;
    else if ((first&0xf8) == 0xf0) return 4;
    else return -1;
}

int utf8_len(const uint8_t* str) {
    const uint8_t* cur8 = str;
    int len = 0;
    while (*cur8) {
        int size = utf8_char_len(*cur8);
        if (size < 0) return -1;
        cur8 += size;
        len++;
    }
    
    return len;
}

int utf8_size(const uint8_t* str) {
    const uint8_t* cur = str;
    while (*cur) {
        int size = utf8_char_len(*cur);
        if (size < 0) return -1;
        cur += size;
    }
    return cur - str;
}

size_t utf32_len(const uint32_t* str) {
    size_t len = 0;
    for (const uint32_t* cur = str; *cur; cur++) len++;
    return len;
}

size_t utf32_size(const uint32_t* str) {
    return utf32_len(str) * 4;
}

uint32_t* utf8_to_utf32(const uint8_t* utf8) {
    int len = utf8_len(utf8);
    if (len < 0) return NULL;
    uint32_t* utf32 = malloc(len*4+4);
    if (!utf32) return NULL;
    
    const uint8_t* cur8 = utf8;
    for (size_t i = 0; i < len; i++) {
        int size = utf8_char_len(*cur8);
        
        if (size < 0) {
            free(utf32);
            return NULL;
        } else if (size == 1) {
            utf32[i] = cur8[0];
        } else if (size == 2) {
            utf32[i] = (((uint32_t)(cur8[0]&0x1f))<<6) |
                       (((uint32_t)(cur8[1]&0x3f)));
        } else if (size == 3) {
            utf32[i] = (((uint32_t)(cur8[0]&0x0f))<<12) |
                       (((uint32_t)(cur8[1]&0x3f))<<6 ) |
                       (((uint32_t)(cur8[2]&0x3f)));
        } else if (size == 4) {
            utf32[i] = (((uint32_t)(cur8[0]&0x07))<<18) |
                       (((uint32_t)(cur8[1]&0x3f))<<12) |
                       (((uint32_t)(cur8[2]&0x3f))<<6 ) |
                       (((uint32_t)(cur8[3]&0x3f)));
        }
        
        cur8 += size;
    }
    
    utf32[len] = 0;
    return utf32;
}

uint8_t* utf32_to_utf8(const uint32_t* utf32) {
    uint8_t* utf8 = malloc(utf32_len(utf32)*6+1);
    uint8_t* cur8 = utf8;
    const uint32_t* cur32 = utf32;
    while (true) {
        uint32_t codepoint = *cur32++;
        if (!codepoint) break;
        
        if (codepoint < 0x80) {
            *cur8++ = codepoint;
        } else if (codepoint < 0x800) {
            *cur8++ = 0xc0 | (uint8_t)((codepoint>>6) &0x1f);
            *cur8++ = 0x80 | (uint8_t) (codepoint     &0x3f);
        } else if (codepoint < 0x10000) {
            *cur8++ = 0xe0 | (uint8_t)((codepoint>>12)&0x0f);
            *cur8++ = 0x80 | (uint8_t)((codepoint>>6) &0x3f);
            *cur8++ = 0x80 | (uint8_t) (codepoint     &0x3f);
        } else if (codepoint < 0x200000) {
            *cur8++ = 0xf0 | (uint8_t)((codepoint>>18)&0x07);
            *cur8++ = 0x80 | (uint8_t)((codepoint>>12)&0x3f);
            *cur8++ = 0x80 | (uint8_t)((codepoint>>6) &0x3f);
            *cur8++ = 0x80 | (uint8_t) (codepoint     &0x3f);
        }
    }
    
    *cur8 = 0;
    return utf8;
}

bool utf8_is_ascii(const uint8_t* str) {
    for (const uint8_t* c = str; *c; c++)
        if (*c > 127) return false;
    return true;
}

bool cmp_str(const uint32_t* str1, const uint32_t* str2) {
    while (true) {
        if (!*str1) return false;
        if (!*str2) return true;
        if (*str1 != *str2) return false;
        str1++;
        str2++;
    }
}

int utf32_vscan(uint32_t* str, uint32_t* fmt, va_list list) {
    uint32_t* cur = str;
    while (*fmt) {
        if (cmp_str(fmt, U"\\%")) {
            if (*cur != '%') return -1;
            fmt += 2;
            cur++;
        } else if (cmp_str(fmt, U"%f")) {
            
        } else if (cmp_str(fmt, U"%d")) {
            int sign = 1;
            if (*cur == '+') {
                str++;
            } else if (*cur == '-') {
                sign = -1;
                str++;
            } else if (*cur<'0' || *cur>'9') {
                return -1;
            }
            uint32_t* end = str;
            while (*end>='0' && *end<='9') end++;
            if (cur == end) return -1;
            int mul = sign;
            int res = 0;
            for (end--; cur<=end; end++)
                res += (*end-'0') * mul;
            *va_arg(list, int*) = res;
        } else {
            if (*cur != *fmt) return -1;
            fmt++;
            cur++;
        }
    }
    return cur - str;
}

int utf32_scan(uint32_t* str, uint32_t* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    int res = utf32_vscan(str, fmt, list);
    va_end(list);
    return res;
}

//TODO: This is far from complete
int utf32_vformat(uint32_t* dest, size_t dest_count, const uint32_t* fmt, va_list list) {
    #define ADD(len_, s_) do {\
        const size_t add_len = (len_);\
        const uint32_t* add_s = (s_);\
        if (left >= add_len) {\
            memcpy(dest, add_s, add_len*4);\
            dest += add_len;\
            left -= add_len;\
        } else {\
            memcpy(dest, add_s, left*4);\
        }\
        needed += add_len;\
    } while (0)
    
    size_t needed = 0;
    size_t left = dest_count;
    while (*fmt) {
        if (cmp_str(fmt, U"%s")) {
            uint32_t* s = va_arg(list, uint32_t*);
            ADD(utf32_len(s), s);
            fmt += 2;
        } else if (cmp_str(fmt, U"%f")) {
            uint32_t* s = utf32_format_double(va_arg(list, double));
            ADD(utf32_len(s), s);
            free(s);
            fmt += 2;
        } else if (cmp_str(fmt, U"%d")) {
            uint32_t* s = utf32_format_int(va_arg(list, int));
            ADD(utf32_len(s), s);
            free(s);
            fmt += 2;
        } else {
            ADD(1, fmt++);
        }
    }
    
    #undef ADD
    
    if (left) *dest = 0;
    
    return needed;
}

int utf32_format(uint32_t* dest, size_t dest_count, const uint32_t* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    int res = utf32_vformat(dest, dest_count, fmt, list);
    va_end(list);
    return res;
}

uint32_t* utf32_format_double(double val) {
    size_t len = snprintf(NULL, 0, "%f", val);
    uint8_t* utf8 = malloc(len+1);
    snprintf((char*)utf8, len, "%f", val);
    uint32_t* utf32 = utf8_to_utf32(utf8);
    free(utf8);
    return utf32;
}

uint32_t* utf32_format_int(int val) {
    size_t len = snprintf(NULL, 0, "%d", val);
    uint8_t* utf8 = malloc(len+1);
    snprintf((char*)utf8, len+1, "%d", val);
    uint32_t* utf32 = utf8_to_utf32(utf8);
    free(utf8);
    return utf32;
}
