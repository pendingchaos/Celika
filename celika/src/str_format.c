//TODO: Test this more thoroughly
//TODO: This is not complete
#include "str_format.h"
#include "str.h"

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static bool cmp_str(const uint32_t* str1, const uint32_t* str2) {
    while (true) {
        if (!*str2) return true;
        if (!*str1) return false;
        if (*str1 != *str2) return false;
        str1++;
        str2++;
    }
}

int utf32_vscan(const uint32_t* str, const uint32_t* fmt, va_list list) {
    const uint32_t* cur = str;
    while (*fmt) {
        if (cmp_str(fmt, U"%%")) {
            if (*cur != '%') return -1;
            fmt += 2;
            cur++;
        } else if (cmp_str(fmt, U"%f")) {
            //TODO
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
            const uint32_t* end = cur;
            while (*end>='0' && *end<='9') end++;
            if (cur == end) return -1;
            int mul = sign;
            int res = 0;
            for (const uint32_t* cur2 = end-1; cur<=cur2; cur2--, mul*=10)
                res += (*cur2-'0') * mul;
            *va_arg(list, int*) = res;
            cur = end;
            fmt += 2;
        } else if (cmp_str(fmt, U"%c")) {
            *va_arg(list, uint32_t*) = *cur++;
            fmt++;
        } else {
            if (*cur != *fmt) return -1;
            fmt++;
            cur++;
        }
    }
    
    return cur - str;
}

int utf32_scan(const uint32_t* str, const uint32_t* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    int res = utf32_vscan(str, fmt, list);
    va_end(list);
    return res;
}

typedef enum fmt_spec_flags_t {
    FMT_LEFT_JUSTIFY = 1<<0, //todo
    FMT_FORCE_SIGN = 1<<1, //done
    FMT_SPACE_IN_PLACE_OF_SIGN = 1<<2, //done
    FMT_HASH = 1<<3, //todo
    FMT_LEFT_PAD_WITH_ZEROS = 1<<4, //todo
    FMT_WIDTH_AS_ARG = 1<<5, //done
    FMT_PRECISION_AS_ARG = 1<<6, //done
    FMT_WIDTH = 1<<7, //todo
    FMT_PRECISION = 1<<8, //done for non-scientific floats
    FMT_LENGTH = 1<<9 //done
} fmt_spec_flags_t;

typedef enum fmt_spec_spec_t {
    FMTSPEC_SIGNED_DECIMAL, //d and i
    FMTSPEC_UNSIGNED_DECIMAL, //u
    FMTSPEC_UNSIGNED_OCTAL, //o
    FMTSPEC_UNSIGNED_LOWER_HEX, //x
    FMTSPEC_UNSIGNED_UPPER_HEX, //X
    FMTSPEC_FLOAT, //f and F
    FMTSPEC_SCIENTIFIC_LOWER, //e
    FMTSPEC_SCIENTIFIC_UPPER, //E
    FMTSPEC_FLOAT_SHORTEST_LOWER, //g
    FMTSPEC_FLOAT_SHORTEST_UPPER, //G
    FMTSPEC_FLOAT_HEX_LOWER, //a
    FMTSPEC_FLOAT_HEX_UPPER, //A
    FMTSPEC_CHARACTER, //c
    FMTSPEC_STRING, //s
    FMTSPEC_POINTER, //p
    FMTSPEC_CHARS_WRITTEN, //n
    FMTSPEC_PRECENT //%
} fmt_spec_spec_t;

typedef enum fmt_spec_length_t {
    FMT_CHAR = 1<<0,
    FMT_SHORT = 1<<1,
    FMT_INT = 1<<2,
    FMT_LONG = 1<<3,
    FMT_LONG_LONG = 1<<4,
    FMT_INTMAX = 1<<5,
    FMT_SIZET = 1<<6,
    FMT_PTRDIFF = 1<<7,
    FMT_DOUBLE = 1<<8,
    FMT_LONG_DOUBLE = 1<<9,
    FMT_POINTER = 1<<10
} fmt_spec_length_t;

typedef struct fmt_spec_t {
    fmt_spec_flags_t flags;
    int width;
    int precision;
    fmt_spec_length_t length;
    fmt_spec_spec_t spec;
} fmt_spec_t;

static const fmt_spec_length_t supported_lengths[] = {
    [FMTSPEC_SIGNED_DECIMAL] = FMT_CHAR | FMT_SHORT | FMT_INT | FMT_LONG | FMT_LONG_LONG | FMT_INTMAX | FMT_PTRDIFF,
    [FMTSPEC_UNSIGNED_DECIMAL] = FMT_CHAR | FMT_SHORT | FMT_INT | FMT_LONG | FMT_LONG_LONG | FMT_INTMAX | FMT_SIZET,
    [FMTSPEC_UNSIGNED_OCTAL] = FMT_CHAR | FMT_SHORT | FMT_INT | FMT_LONG | FMT_LONG_LONG | FMT_INTMAX | FMT_SIZET,
    [FMTSPEC_UNSIGNED_LOWER_HEX] = FMT_CHAR | FMT_SHORT | FMT_INT | FMT_LONG | FMT_LONG_LONG | FMT_INTMAX | FMT_SIZET,
    [FMTSPEC_UNSIGNED_UPPER_HEX] = FMT_CHAR | FMT_SHORT | FMT_INT | FMT_LONG | FMT_LONG_LONG | FMT_INTMAX | FMT_SIZET,
    [FMTSPEC_FLOAT] = FMT_DOUBLE | FMT_LONG_DOUBLE,
    [FMTSPEC_SCIENTIFIC_LOWER] = FMT_DOUBLE | FMT_LONG_DOUBLE,
    [FMTSPEC_SCIENTIFIC_UPPER] = FMT_DOUBLE | FMT_LONG_DOUBLE,
    [FMTSPEC_FLOAT_SHORTEST_LOWER] = FMT_DOUBLE | FMT_LONG_DOUBLE,
    [FMTSPEC_FLOAT_SHORTEST_UPPER] = FMT_DOUBLE | FMT_LONG_DOUBLE,
    [FMTSPEC_FLOAT_HEX_LOWER] = FMT_DOUBLE | FMT_LONG_DOUBLE,
    [FMTSPEC_FLOAT_HEX_UPPER] = FMT_DOUBLE | FMT_LONG_DOUBLE,
    [FMTSPEC_CHARACTER] = 0,
    [FMTSPEC_STRING] = FMT_POINTER,
    [FMTSPEC_POINTER] = FMT_POINTER,
    [FMTSPEC_CHARS_WRITTEN] = FMT_CHAR | FMT_SHORT | FMT_INT | FMT_LONG | FMT_LONG_LONG | FMT_INTMAX | FMT_SIZET | FMT_PTRDIFF,
    [FMTSPEC_PRECENT] = 0
};

static uint32_t* _format_int(uintmax_t val, size_t base, const uint32_t* chars) {
    if (!val) {
        uint32_t* mem = malloc(8);
        mem[0] = chars[0];
        mem[1] = 0;
        return mem;
    }
    
    size_t digits = 0;
    for (intmax_t v = val; v; v/=base) digits++;
    
    uint32_t* mem = malloc((digits+1)*4);
    mem[digits] = 0;
    
    intmax_t div = 1;
    for (ptrdiff_t i = digits-1; i >= 0; i--) {
        intmax_t digit = val/div % base;
        mem[i] = chars[digit];
        div *= base;
    }
    
    return mem;
}

static uint32_t* format_int(intmax_t val, size_t base, const uint32_t* chars,
                            bool force_sign, bool space_sign) {
    uint32_t sign = 0;
    if (val < 0)
        sign = '-';
    else if (val >= 0 && force_sign)
        sign = '+';
    else if (space_sign)
        sign = ' ';
    
    uint32_t* formatted = _format_int(val<0 ? -val : val, base, chars);
    
    if (sign) {
        uint32_t* res = malloc(utf32_len(formatted)*4+8);
        res[0] = sign;
        memcpy(res+1, formatted, utf32_len(formatted)*4+4);
        return res;
    }
    
    return formatted;
}

static uint32_t* format_uint(uintmax_t val, size_t base, const uint32_t* chars,
                             bool force_sign, bool space_sign) {
    uint32_t sign = 0;
    if (force_sign)
        sign = '+';
    else if (space_sign)
        sign = ' ';
    
    uint32_t* formatted = _format_int(val<0 ? -val : val, base, chars);
    
    if (sign) {
        uint32_t* res = malloc(utf32_len(formatted)*4+8);
        res[0] = sign;
        memcpy(res+1, formatted, utf32_len(formatted)*4+4);
        return res;
    }
    
    return formatted;
}

static uint32_t* format_float(long double val, size_t base, const uint32_t* chars,
                              bool force_sign, bool space_sign, int width, int prec) {
    if (prec < 0) prec = 6;
    
    uint32_t* int_part = format_int((intmax_t)floor(val), base, chars, force_sign, space_sign);
    size_t int_len = utf32_len(int_part);
    
    uint32_t* res = calloc(int_len+prec+2, 4);
    memcpy(res, int_part, int_len*4);
    res[int_len] = '.';
    
    //Fractional part
    size_t zero_len = 0;
    for (size_t i = 0; i < prec; i++) {
        size_t digit = (intmax_t)(floor(val*pow(base, i+1))) % base;
        if (!digit) zero_len++;
        else zero_len = 0;
        if (zero_len >= 2) {
            res[int_len+i] = 0;
            break;
        }
        res[int_len+1+i] = chars[digit];
    }
    
    if (!res[int_len+1]) res[int_len] = 0;
    
    size_t len = int_len + prec + 1;
    res[len] = 0;
    
    return res;
}

static uint32_t* format(fmt_spec_t spec, va_list list) {
    if (spec.flags&FMT_LENGTH)
        if (!(supported_lengths[spec.spec]&spec.length)) return NULL;
    
    if (spec.spec == FMTSPEC_PRECENT) {
        uint32_t* data = malloc(8);
        if (!data) return NULL;
        data[0] = '%';
        data[1] = 0;
        return data;
    } else if (spec.spec == FMTSPEC_CHARACTER) {
        uint32_t* data = malloc(8);
        if (!data) return NULL;
        data[0] = va_arg(list, uint32_t);
        data[1] = 0;
        return data;
    } else if (spec.spec == FMTSPEC_CHARS_WRITTEN) {
        //TODO
    }
    
    int width = -1;
    if ((spec.flags&FMT_WIDTH) && (spec.flags&FMT_WIDTH_AS_ARG))
        width = va_arg(list, int);
    else if (spec.flags & FMT_WIDTH)
        width = spec.width;
    
    int precision = -1;
    if ((spec.flags&FMT_PRECISION) && (spec.flags&FMT_PRECISION_AS_ARG))
        precision = va_arg(list, int);
    else if (spec.flags & FMT_PRECISION)
        precision = spec.precision;
    
    fmt_spec_length_t length;
    bool is_signed = false;
    
    switch (spec.spec) {
    case FMTSPEC_SIGNED_DECIMAL:
        length = FMT_INT;
        is_signed = true;
        break;
    case FMTSPEC_UNSIGNED_DECIMAL:
    case FMTSPEC_UNSIGNED_OCTAL:
    case FMTSPEC_UNSIGNED_LOWER_HEX:
    case FMTSPEC_UNSIGNED_UPPER_HEX:
        length = FMT_INT;
        is_signed = false;
        break;
    case FMTSPEC_FLOAT:
    case FMTSPEC_SCIENTIFIC_LOWER:
    case FMTSPEC_SCIENTIFIC_UPPER:
    case FMTSPEC_FLOAT_SHORTEST_LOWER:
    case FMTSPEC_FLOAT_SHORTEST_UPPER:
    case FMTSPEC_FLOAT_HEX_LOWER:
    case FMTSPEC_FLOAT_HEX_UPPER:
        length = FMT_DOUBLE;
        is_signed = true;
        break;
    case FMTSPEC_CHARACTER:
        break;
    case FMTSPEC_STRING:
        length = FMT_POINTER;
        is_signed = false;
        break;
    case FMTSPEC_POINTER:
        length = FMT_POINTER;
        is_signed = false;
        break;
    case FMTSPEC_CHARS_WRITTEN:
        break;
    case FMTSPEC_PRECENT:
        break;
    }
    
    if (spec.flags & FMT_LENGTH) length = spec.length;
    
    intmax_t signed_int;
    uintmax_t unsigned_int;
    long double floating_point;
    void* pointer;
    
    switch (length) {
    case FMT_CHAR:
    case FMT_SHORT:
    case FMT_INT:
        if (is_signed) signed_int = va_arg(list, int);
        else unsigned_int = va_arg(list, unsigned int);
        break;
    case FMT_LONG:
        if (is_signed) signed_int = va_arg(list, long);
        else unsigned_int = va_arg(list, unsigned long);
        break;
    case FMT_LONG_LONG:
        if (is_signed) signed_int = va_arg(list, long long);
        else unsigned_int = va_arg(list, unsigned long long);
        break;
    case FMT_INTMAX:
        if (is_signed) signed_int = va_arg(list, intmax_t);
        else unsigned_int = va_arg(list, uintmax_t);
        break;
    case FMT_SIZET:
        unsigned_int = va_arg(list, size_t);
        break;
    case FMT_PTRDIFF:
        signed_int = va_arg(list, ptrdiff_t);
        break;
    case FMT_DOUBLE:
        floating_point = va_arg(list, double);
        break;
    case FMT_LONG_DOUBLE:
        floating_point = va_arg(list, long double);
        break;
    case FMT_POINTER:
        pointer = va_arg(list, void*);
        break;
    }
    
    bool force_sign = spec.flags & FMT_FORCE_SIGN;
    bool space_sign = spec.flags & FMT_SPACE_IN_PLACE_OF_SIGN;
    
    switch (spec.spec) {
    case FMTSPEC_SIGNED_DECIMAL:
        return format_int(signed_int, 10, U"0123456789", force_sign, space_sign);
    case FMTSPEC_UNSIGNED_DECIMAL:
        return format_uint(unsigned_int, 10, U"0123456789", force_sign, space_sign);
    case FMTSPEC_UNSIGNED_OCTAL:
        return format_uint(unsigned_int, 8, U"01234567", force_sign, space_sign);
    case FMTSPEC_UNSIGNED_LOWER_HEX:
        return format_uint(unsigned_int, 16, U"0123456789abcdef", force_sign, space_sign);
    case FMTSPEC_UNSIGNED_UPPER_HEX:
        return format_uint(unsigned_int, 16, U"0123456789ABCDEF", force_sign, space_sign);
    case FMTSPEC_FLOAT:
        return format_float(floating_point, 10, U"0123456789", force_sign,
                            space_sign, width, precision);
    //TODO
    case FMTSPEC_SCIENTIFIC_LOWER:
        break;
    case FMTSPEC_SCIENTIFIC_UPPER:
        break;
    case FMTSPEC_FLOAT_SHORTEST_LOWER:
        break;
    case FMTSPEC_FLOAT_SHORTEST_UPPER:
        break;
    case FMTSPEC_FLOAT_HEX_LOWER:
        return format_float(floating_point, 16, U"0123456789abcdef", force_sign,
                            space_sign, width, precision);
    case FMTSPEC_FLOAT_HEX_UPPER:
        return format_float(floating_point, 16, U"0123456789ABCDEF", force_sign,
                            space_sign, width, precision);
    case FMTSPEC_CHARACTER:
        break;
    case FMTSPEC_STRING: {
        uint32_t* res = malloc(utf32_len(pointer)*4+4);
        memcpy(res, pointer, utf32_len(pointer)*4+4);
        return res;
    } case FMTSPEC_POINTER:
        return format_uint((size_t)pointer, 16, U"0123456789abcdef", force_sign, space_sign);
    case FMTSPEC_CHARS_WRITTEN:
        break;
    case FMTSPEC_PRECENT:
        break;
    }
    
    assert(false);
}

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
        if (*fmt == '%') {
            fmt++;
            
            fmt_spec_t spec;
            spec.flags = 0;
            
            //Flags
            while (*fmt=='-' || *fmt=='+' || *fmt==' ' || *fmt=='#' || *fmt=='0') {
                uint32_t flag = *fmt++;
                switch (flag) {
                case '-':
                    spec.flags |= FMT_LEFT_JUSTIFY;
                    break;
                case '+':
                    spec.flags |= FMT_FORCE_SIGN;
                    break;
                case ' ':
                    spec.flags |= FMT_SPACE_IN_PLACE_OF_SIGN;
                    break;
                case '#':
                    spec.flags |= FMT_HASH;
                    break;
                case '0':
                    spec.flags |= FMT_LEFT_PAD_WITH_ZEROS;
                    break;
                }
            }
            
            //Width
            if (*fmt>='0' && *fmt<='9') {
                int amount = utf32_scan(fmt, U"%d", &spec.width); //TODO: Use %u
                if (amount<0 || spec.width<0) return 0;
                fmt += amount;
                spec.flags |= FMT_WIDTH;
            } else if (*fmt == '*') {
                fmt++;
                spec.flags |= FMT_WIDTH | FMT_WIDTH_AS_ARG;
            }
            
            //Precision
            if (*fmt == '.') {
                int amount = utf32_scan(fmt, U".%d", &spec.precision); //TODO: Use %u
                if (amount<0 || spec.precision<0) return 0;
                fmt += amount;
                spec.flags |= FMT_PRECISION;
            } else if (cmp_str(fmt, U".*")) {
                fmt += 2;
                spec.flags |= FMT_PRECISION | FMT_PRECISION_AS_ARG;
            }
            
            //Length
            spec.length = 0;
            if (cmp_str(fmt, U"hh")) {
                spec.length = FMT_CHAR;
                fmt += 2;
            } else if (cmp_str(fmt, U"h")){
                spec.length = FMT_SHORT;
                fmt++;
            } else if (cmp_str(fmt, U"ll")) {
                spec.length = FMT_LONG_LONG;
                fmt += 2;
            } else if (cmp_str(fmt, U"l")) {
                spec.length = FMT_LONG;
                fmt++;
            } else if (cmp_str(fmt, U"j")) {
                spec.length = FMT_INTMAX;
                fmt++;
            } else if (cmp_str(fmt, U"z")) {
                spec.length = FMT_SIZET;
                fmt++;
            } else if (cmp_str(fmt, U"t")) {
                spec.length = FMT_PTRDIFF;
                fmt++;
            } else if (cmp_str(fmt, U"L")) {
                spec.length = FMT_LONG_DOUBLE;
                fmt++;
            }
            spec.flags |= spec.length ? FMT_LENGTH : 0;
            
            //Specifier
            if (*fmt=='d' || *fmt=='i')
                spec.spec = FMTSPEC_SIGNED_DECIMAL;
            else if (*fmt == 'u')
                spec.spec = FMTSPEC_UNSIGNED_DECIMAL;
            else if (*fmt == 'o')
                spec.spec = FMTSPEC_UNSIGNED_OCTAL;
            else if (*fmt == 'x')
                spec.spec = FMTSPEC_UNSIGNED_LOWER_HEX;
            else if (*fmt == 'X')
                spec.spec = FMTSPEC_UNSIGNED_UPPER_HEX;
            else if (*fmt=='f' || *fmt=='F')
                spec.spec = FMTSPEC_FLOAT;
            else if (*fmt == 'e')
                spec.spec = FMTSPEC_SCIENTIFIC_LOWER;
            else if (*fmt == 'E')
                spec.spec = FMTSPEC_SCIENTIFIC_UPPER;
            else if (*fmt == 'g')
                spec.spec = FMTSPEC_FLOAT_SHORTEST_LOWER;
            else if (*fmt == 'G')
                spec.spec = FMTSPEC_FLOAT_SHORTEST_UPPER;
            else if (*fmt == 'a')
                spec.spec = FMTSPEC_FLOAT_HEX_LOWER;
            else if (*fmt == 'A')
                spec.spec = FMTSPEC_FLOAT_HEX_UPPER;
            else if (*fmt == 'c')
                spec.spec = FMTSPEC_CHARACTER;
            else if (*fmt == 's')
                spec.spec = FMTSPEC_STRING;
            else if (*fmt == 'p')
                spec.spec = FMTSPEC_POINTER;
            else if (*fmt == 'n')
                spec.spec = FMTSPEC_CHARS_WRITTEN;
            else if (*fmt == '%')
                spec.spec = FMTSPEC_PRECENT;
            fmt++;
            
            uint32_t* s = format(spec, list);
            if (!s) return -1;
            ADD(utf32_len(s), s);
            free(s);
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
