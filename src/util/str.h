// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef STR_H
#define STR_H

#include "pltf/pltf.h"

static inline u32 hash_str(const char *s)
{
    u32 h = 0;
    for (i32 n = 0; s[n] != '\0'; n++) {
        h = h * 101 + (u32)s[n];
    }
    return h;
}

#define FILEPATH_GEN(NAME, PATHNAME, FILENAME) \
    char NAME[128];                            \
    str_cpy(NAME, PATHNAME);                   \
    str_append(NAME, FILENAME)

static inline i32 char_upper(i32 c)
{
    return ((i32)'a' <= c && c <= (i32)'z' ? c - (i32)'a' + (i32)'A' : c);
}

static inline i32 char_lower(i32 c)
{
    return ((i32)'A' <= c && c <= (i32)'Z' ? c - (i32)'A' + (i32)'a' : c);
}

static bool32 char_is_any(i32 c, const char *chars)
{
    if (!chars) return 0;
    for (const char *a = chars; *a != '\0'; a++) {
        if (c == (i32)*a) return 1;
    }
    return 0;
}

static inline i32 str_cmp(const void *a, const void *b)
{
    const u8 *x = (const u8 *)a;
    const u8 *y = (const u8 *)b;
    while (1) {
        if (*y == '\0') return 0;
        if (*x != *y) return ((i32)*x - (i32)*y);
        x++;
        y++;
    }
}

// if equal including the null terminators!
static inline bool32 str_eq(const void *a, const void *b)
{
    return str_cmp(a, b) == 0;
}

// if equal (no case) including the null terminators!
static inline bool32 str_eq_nc(const void *a, const void *b)
{
    const u8 *x = (const u8 *)a;
    const u8 *y = (const u8 *)b;
    while (1) {
        if (char_upper(*x) != char_upper(*y)) return 0;
        if (*x == '\0') return 1;
        x++;
        y++;
    }
}

// finds a character inside a str (including the null terminator)
static inline void *str_chr(const void *str, i32 c)
{
    const u8 *x = (const u8 *)str;
    while (1) {
        if (*x == c) return (void *)x;
        if (*x == '\0') return 0;
        x++;
    }
    return 0;
}

static inline void *str_contains(const void *str, const void *sequence)
{
    const u8 *x  = (const u8 *)str;
    i32       fc = *(const u8 *)sequence;
    while (1) {
        x = (const u8 *)str_chr(x, fc);
        if (!x) return 0;

        i32 e = str_cmp(x, sequence);
        if (e == 0) return (void *)x; // equal until null of sequence

        x++;
    }
    return 0;
}

// number of characters excluding null-char
static inline i32 str_len(const void *a)
{
    const u8 *x = (const u8 *)a;
    while (1) {
        if (*x == '\0') return (i32)(x - (const u8 *)a);
        x++;
    }
}

static void str_cpy(void *dst, const void *src)
{
    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    while (1) {
        *d = *s;
        if (*s == '\0') break;
        d++;
        s++;
    }
}

static void str_cpys(void *dst, usize dstsize, const void *src)
{
    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    usize     n = 0;
    while (*s != '\0' && n < dstsize - 1) {
        *d++ = *s++;
        n++;
    }
    *d = '\0';
}

// assets/tex/file.png -> file.png
static void str_extract_filename(const char *src, char *buf, u32 bufsize)
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
static inline void str_append(void *dst, const void *src)
{
    u8 *d = (u8 *)str_chr(dst, '\0');
    if (d) {
        str_cpy(d, src);
    }
}

static inline void str_append_char(void *dst, i32 c)
{
    u8 ch[2] = {(u8)c, '\0'};
    str_append(dst, ch);
}

// appends string b -> overwrites null-char and places a new null-char
static void str_append_i(void *dst, i32 i)
{
    if (i == 0) {
        str_append(dst, "0");
        return;
    }
    u8  b1[16] = {0};
    u8  b2[16] = {0};
    i32 l      = 0;
    for (i32 j = 0 <= i ? +i : -i; 0 < j; j /= 10) {
        b1[l++] = '0' + (j % 10);
    }

    i32 k1 = 0;
    if (i < 0) {
        k1    = 1;
        b2[0] = '-';
    }
    for (i32 k = 0; k < l; k++) {
        b2[k + k1] = b1[l - k - 1];
    }
    str_append(dst, &b2[0]);
}

static i32 num_from_hex(i32 c)
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
    return 0;
}

static i32 hex_from_num(i32 c)
{
    static const char *g_hex = "0123456789ABCDEF";
    return (0x0 <= c && c <= 0xF ? g_hex[c] : 0);
}

static f32 f32_from_str(const void *str)
{
    const u8 *c = (const u8 *)str;
    while (isspace((int)*c))
        c++;

    f32 res  = 0.f;
    f32 fact = 1.f;
    if (*c == '-') {
        fact = -1.f;
        c++;
    }

    for (i32 pt = 0;; c++) {
        if (*c == '.') {
            pt = 1;
        } else if (isdigit((int)*c)) {
            if (pt) fact *= .1f;
            res = res * 10.f + (f32)num_from_hex(*c);
        } else {
            break;
        }
    }
    return res * fact;
}

static i32 i32_from_str(const void *str)
{
    const u8 *c = (const u8 *)str;
    while (isspace((int)*c))
        c++;
    i32 res = 0;
    i32 s   = +1;
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

static u32 u32_from_str(const void *str)
{
    const u8 *c = (const u8 *)str;
    while (isspace((int)*c))
        c++;
    u32 res = 0;
    while ('0' <= *c && *c <= '9') {
        res *= 10;
        res += *c - '0';
        c++;
    }
    return res;
}

#define strs_from_u32(V, BUF) str_from_u32(V, BUF, sizeof(BUF))
static i32 str_from_u32(u32 v, void *dst, u32 dstsize)
{
    if (!dst || dstsize < 2) return 0;
    u8 *buf = (u8 *)dst;
    if (v == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 2;
    }
    i32 n     = 0;
    u8  b[16] = {0};

    for (u32 x = v; x && (u32)n < dstsize; x /= 10) {
        b[n++] = '0' + (x % 10);
    }

    i32 len = --n;
    while (0 <= n) { // reverse
        buf[len - n] = b[n];
        n--;
    }
    buf[len + 1] = '\0';
    return len + 1;
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

#endif