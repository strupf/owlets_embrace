// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef STR_H
#define STR_H

#include "pltf/pltf.h"

static inline i32 char_to_upper(i32 c)
{
    return ((i32)'a' <= c && c <= (i32)'z' ? c - (i32)'a' + (i32)'A' : c);
}

static inline i32 char_to_lower(i32 c)
{
    return ((i32)'A' <= c && c <= (i32)'Z' ? c - (i32)'A' + (i32)'a' : c);
}

static inline u32 hash_str_func(u32 h, u32 next)
{
    return (h * 101 + next);
}

static u32 hash_str(const void *str)
{
    if (!str) return 0;

    u32       h = 0;
    const u8 *s = (const u8 *)str;
    while (1) {
        u32 c = char_to_lower(*s++);
        if (c == 0) break;
        h = hash_str_func(h, c);
    }
    return h;
}

static inline u32 hash_str16(const void *str)
{
    // h * 101 + next
    // -> use low bits, otherwise hash would be 0 for short strings
    return (hash_str(str) & 0xFFFF);
}

static inline u32 hash_str8(const void *str)
{
    // h * 101 + next
    // -> use low bits, otherwise hash would be 0 for short strings
    return (hash_str(str) & 0xFF);
}

#define FILEPATH_GEN(NAME, PATHNAME, FILENAME) \
    char NAME[128];                            \
    str_cpy(NAME, PATHNAME);                   \
    str_append(NAME, FILENAME)

static bool32 char_is_any(i32 c, const void *chars)
{
    if (!chars) return 0;
    for (const u8 *a = (const u8 *)chars; *a != '\0'; a++) {
        if (c == (i32)*a) return 1;
    }
    return 0;
}

static bool32 char_is_ws(i32 c)
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

static void *str_skip_ws(const void *p)
{
    u8 *c = (u8 *)p;
    while (char_is_ws(*c)) {
        c++;
    }
    return c;
}

static bool32 char_is_digit(i32 c)
{
    return ('0' <= c && c <= '9');
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
        if (*y == '\0') return 1;
        if (char_to_upper(*x) != char_to_upper(*y)) return 0;
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
    return -1;
}

static u32 num_from_hex_str(void *str)
{
    u32 n = 0;
    u8 *c = (u8 *)str;
    while (1) {
        i32 k = num_from_hex(*c);
        if (k < 0) break;

        n = (n << 4) | (u32)k;
        c++;
    }
    return n;
}

static i32 hex_from_num(i32 c)
{
    ALIGNAS(32) static const char *g_hex = "0123456789ABCDEF";
    return (0 <= c && c <= 15 ? g_hex[c] : 0);
}

static f32 f32_from_str(const void *str)
{
    const u8 *c    = (const u8 *)str_skip_ws(str);
    f32       res  = 0.f;
    f32       fact = 1.f;
    if (*c == '-') {
        fact = -1.f;
        c++;
    }

    for (i32 pt = 0;; c++) {
        if (*c == '.') {
            pt = 1;
        } else if (char_is_digit((i32)*c)) {
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
    const u8 *c   = (const u8 *)str_skip_ws(str);
    i32       res = 0;
    i32       s   = +1;
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
    const u8 *c   = (const u8 *)str_skip_ws(str);
    u32       res = 0;
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
static i32 QX_gen(const void *str, i32 q)
{
    const u8 *c = (const u8 *)str_skip_ws(str);
    if (*c == '\0') return 0;

    i32 neg = 0;
    if (*c == '-') {
        neg = 1;
        c++;
    }

    i32 n = 0;
    while (*c != '.' && *c != '\0') {
        n = n * 10 + ((*c++ - '0') << q);
    }

    if (*c == '.') {
        c++;
        u32 f   = 0;
        u32 div = 1;
        while (*c != '\0') {
            f = f * 10 + ((*c++ - '0') << q);
            div *= 10;
        }
        n += (i32)(f / div);
    }

    return (neg ? -n : +n);
}

typedef struct {
    ALIGNAS(16)
    u32 v[4];
} uuid128_s;

// expects correct string formatting
static uuid128_s uuid128_from_str(const void *str)
{
    i32       n_hexdig = 0;
    uuid128_s g        = {0};

    for (const u8 *s = (const u8 *)str; *s && n_hexdig < 32; s++) {
        i32 i = num_from_hex(*s);
        if (0 <= i) {
            i32 at  = n_hexdig >> 3;
            g.v[at] = ((u32)g.v[at] << 4) | i;
            n_hexdig++;
        }
    }
    return g;
}

// expects correct string formatting
// reduce 128 to 32 bits and hope for the best
static u32 uuid32_from_uuid128_str(const void *str)
{
    i32 n_hexdig = 0;
    u32 r        = 0;
    u32 v        = 0;

    for (const u8 *s = (const u8 *)str; *s; s++) {
        i32 i = num_from_hex(*s);
        if (i < 0) continue;

        v = (v << 4) | i;
        n_hexdig++;
        if ((n_hexdig & 7) == 0) {
            r ^= v;
            v = 0;

            if (n_hexdig == 32) {
                break;
            }
        }
    }
    return r;
}

// just xor together and hope for the best
static u32 u32_from_uuid128(uuid128_s u)
{
    u32 v = u.v[0] ^ u.v[1] ^ u.v[2] ^ u.v[3];
    return v;
}

#endif