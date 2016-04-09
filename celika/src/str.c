//TODO: Test this more thoroughly
//TODO: Handle invalid data
#include "str.h"

#include <string.h>
#include <stdlib.h>

static int utf8_char_len(uint8_t first) {
    if ((first&0b10000000) == 0b00000000) return 1;
    else if ((first&0b11100000) == 0b11000000) return 2;
    else if ((first&0b11110000) == 0b11100000) return 3;
    else if ((first&0b11111000) == 0b11110000) return 4;
    else if ((first&0b11111100) == 0b11111000) return 5;
    else if ((first&0b11111110) == 0b11111100) return 6;
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
            utf32[i] = (((uint32_t)(cur8[0]&0b00011111))<<6) |
                       (((uint32_t)(cur8[1]&0b00111111)));
        } else if (size == 3) {
            utf32[i] = (((uint32_t)(cur8[0]&0b00001111))<<12) |
                       (((uint32_t)(cur8[1]&0b00111111))<<6) |
                       (((uint32_t)(cur8[2]&0b00111111)));
        } else if (size == 4) {
            utf32[i] = (((uint32_t)(cur8[0]&0b00000111))<<18) |
                       (((uint32_t)(cur8[1]&0b00111111))<<12) |
                       (((uint32_t)(cur8[2]&0b00111111))<<6) |
                       (((uint32_t)(cur8[3]&0b00111111)));
        } else if (size == 5) {
            utf32[i] = (((uint32_t)(cur8[0]&0b00000011))<<24) |
                       (((uint32_t)(cur8[1]&0b00111111))<<18) |
                       (((uint32_t)(cur8[2]&0b00111111))<<12) |
                       (((uint32_t)(cur8[3]&0b00111111))<<6) |
                       (((uint32_t)(cur8[4]&0b00111111)));
        } else if (size == 6) {
            utf32[i] = (((uint32_t)(cur8[0]&0b00000001))<<30) |
                       (((uint32_t)(cur8[1]&0b00111111))<<24) |
                       (((uint32_t)(cur8[2]&0b00111111))<<18) |
                       (((uint32_t)(cur8[3]&0b00111111))<<12) |
                       (((uint32_t)(cur8[4]&0b00111111))<<6) |
                       (((uint32_t)(cur8[5]&0b00111111)));
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
            *cur8++ = 0b11000000 | (uint8_t)((codepoint>>6) &0b00011111);
            *cur8++ = 0b10000000 | (uint8_t) (codepoint     &0b00111111);
        } else if (codepoint < 0x10000) {
            *cur8++ = 0b11100000 | (uint8_t)((codepoint>>12)&0b00001111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>6) &0b00111111);
            *cur8++ = 0b10000000 | (uint8_t) (codepoint     &0b00111111);
        } else if (codepoint < 0x200000) {
            *cur8++ = 0b11110000 | (uint8_t)((codepoint>>18)&0b00000111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>12)&0b00111111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>6) &0b00111111);
            *cur8++ = 0b10000000 | (uint8_t) (codepoint     &0b00111111);
        } else if (codepoint < 0x4000000) {
            *cur8++ = 0b11111000 | (uint8_t)((codepoint>>24)&0b00000011);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>18)&0b00111111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>12)&0b00111111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>6) &0b00111111);
            *cur8++ = 0b10000000 | (uint8_t) (codepoint     &0b00111111);
        } else if (codepoint < 0x80000000) {
            *cur8++ = 0b11111100 | (uint8_t)((codepoint>>30)&0b00000001);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>24)&0b00111111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>18)&0b00111111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>12)&0b00111111);
            *cur8++ = 0b10000000 | (uint8_t)((codepoint>>6) &0b00111111);
            *cur8++ = 0b10000000 | (uint8_t) (codepoint     &0b00111111);
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
