// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef STR_H
#define STR_H

#include "sys/sys_types.h"

#define FILEPATH_GEN(NAME, PATHNAME, FILENAME) \
    char NAME[64];                             \
    str_cpy(NAME, PATHNAME);                   \
    str_append(NAME, FILENAME)

static int char_is_any(char c, const char *chars)
{
    if (!chars) return 0;
    for (const char *a = chars; *a != '\0'; a++) {
        if (c == *a) return 1;
    }
    return 0;
}

static int str_eq(const char *a, const char *b)
{
    for (const char *x = a, *y = b;; x++, y++) {
        if (*x != *y) return 0;
        if (*x == '\0') break;
    }
    return 1;
}

static int str_contains(const char *str, const char *sequence)
{
    for (const char *x = str; *x != '\0'; x++) {
        for (const char *a = x, *b = sequence;; a++, b++) {
            if (*a == '\0' && *b != '\0') break;
            if (*b == '\0') return 1;
            if (*a != *b) break;
        }
    }
    return 0;
}

// number of characters excluding null-char
static int str_len(const char *a)
{
    for (int l = 0;; l++)
        if (a[l] == '\0') return l;
    return 0;
}

static void str_cpy(char *dst, const char *src)
{
    char       *d = dst;
    const char *s = src;
    while (*s != '\0')
        *d++ = *s++;
    *d = '\0';
}

static void str_cpys(char *dst, usize dstsize, const char *src)
{
    char       *d = dst;
    const char *s = src;
    while (*s != '\0' && (d + 1) < (dst + dstsize))
        *d++ = *s++;
    *d = '\0';
}

#define str_cpysb(DST, SRC) str_cpys(DST, (usize)sizeof(DST), SRC)

// assets/tex/file.png -> file.png
static void str_extract_filename(const char *src, char *buf, usize bufsize)
{
    const char *s = src;
    while (*s != '\0' && *s != '.')
        s++;
    while (src < s) {
        s--;
        if (*s == '/') {
            s++;
            break;
        }
    }
    str_cpys(buf, bufsize, s);
}

// appends string b -> overwrites null-char and places a new null-char
static void str_append(char *dst, const char *src)
{
    str_cpy(&dst[str_len(dst)], src);
}

// appends string b -> overwrites null-char and places a new null-char
static void str_append_i(char *dst, int i)
{
    if (i == 0) {
        str_append(dst, "0");
        return;
    }
    char b1[16] = {0};
    char b2[16] = {0};
    int  l      = 0;
    int  j      = i >= 0 ? i : -i;
    while (j > 0) {
        b1[l++] = '0' + (j % 10);
        j /= 10;
    }

    int k1 = 0;
    if (i < 0) {
        k1    = 1;
        b2[0] = '-';
    }
    for (int k = 0; k < l; k++)
        b2[k + k1] = b1[l - k - 1];
    str_append(dst, &b2[0]);
}

static char *str_find_char(const char *str, char c)
{
    for (const char *it = str; *it != '\0'; it++) {
        if (*it == c) return (char *)it;
    }
    return NULL;
}

static int char_to_lower(int c)
{
    return (65 <= c && c <= 90 ? (c | 32) : c);
}

static int char_to_upper(int c)
{
    return (97 <= c && c <= 122 ? (c & ~32) : c);
}

static int char_hex_to_int(int c)
{
    switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A':
    case 'a': return 10;
    case 'B':
    case 'b': return 11;
    case 'C':
    case 'c': return 12;
    case 'D':
    case 'd': return 13;
    case 'E':
    case 'e': return 14;
    case 'F':
    case 'f': return 15;
    }
    return -1;
}

#define char_int_from_hex char_hex_to_int

static int char_is_digit(int c)
{
    return ('0' <= c && c <= '9');
}

static int char_is_xdigit(int c)
{
    return (char_hex_to_int(c) >= 0);
}

static int char_is_ws(int c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r': return 1;
    }
    return 0;
}

f32 f32_from_str(const char *str)
{
    const char *c = str;
    while (char_is_ws(*c))
        c++;

    f32 res  = 0.f;
    f32 fact = 1.f;
    if (*c == '-') {
        fact = -1.f;
        c++;
    }

    for (int pt = 0;; c++) {
        if (*c == '.') {
            pt = 1;
        } else if (char_is_digit(*c)) {
            if (pt) fact *= .1f;
            res = res * 10.f + (f32)char_int_from_hex(*c);
        } else {
            break;
        }
    }
    return res * fact;
}

i32 i32_from_str(const char *str)
{
    const char *c = str;
    while (char_is_ws(*c))
        c++;
    i32 res = 0;
    int s   = +1;
    if (*c == '-') {
        s = -1;
        c++;
    }
    while ('0' <= *c && *c <= '9') {
        res *= 10;
        res += *c - '0';
        c++;
    }
    return (res * s);
}

u32 u32_from_str(const char *str)
{
    const char *c = str;
    while (char_is_ws(*c))
        c++;
    u32 res = 0;
    while ('0' <= *c && *c <= '9') {
        res *= 10;
        res += *c - '0';
        c++;
    }
    return res;
}

// string float to fixed point integer parsing
static int QX_gen(const char *str, int q)
{
    int         neg = 0;
    const char *c   = str;
    if (*c == '-') {
        neg = 1;
        c++;
    }

    int n = 0;
    while (*c != '.' && *c != '\0') {
        n = n * 10 + ((*c++ - '0') << q);
    }

    if (*c == '.') {
        c++;
        uint f   = 0;
        uint div = 1;
        while (*c != '\0') {
            f = f * 10 + ((*c++ - '0') << q);
            div *= 10;
        }
        n += (int)(f / div);
    }

    return (neg ? -n : +n);
}

#define Q_4(NUM)  (int)((NUM)*16.f)
#define Q_8(NUM)  (int)((NUM)*256.f)
#define Q_16(NUM) (int)((NUM)*65536.f)

#endif