// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef STR_H
#define STR_H

#include "sys/sys_types.h"

static int str_eq(const char *a, const char *b)
{
    for (const char *x = a, *y = b; *x != '\0' && *y != '\0'; x++, y++)
        if (*x != *y) return 0;
    return 1;
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
        if (*it == c) return it;
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
    char *c = str;
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

#endif