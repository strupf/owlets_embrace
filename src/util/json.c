// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "json.h"
#include "str.h"

bool32 txt_load_buf(const char *filename, char *buf, usize bufsize)
{
    void *f = pltf_file_open_r(filename);
    if (!f) {
        pltf_log("+++ Can't open %s\n", filename);
        return 0;
    }
    pltf_file_seek_end(f, 0);
    i32 size = pltf_file_tell(f);
    pltf_file_seek_set(f, 0);
    if ((usize)size + 1 >= bufsize) {
        pltf_log("+++ txt buf too small %s\n", filename);
        return 0;
    }
    pltf_file_r(f, buf, size);
    pltf_file_close(f);
    buf[size] = '\0';
    return 1;
}

bool32 txt_load(const char *filename, void *(*allocfunc)(usize s), char **txt_out)
{
    void *f = pltf_file_open_r(filename);
    if (!f) {
        pltf_log("+++ Can't open %s\n", filename);
        return 0;
    }
    pltf_file_seek_end(f, 0);
    i32 size = pltf_file_tell(f);
    pltf_file_seek_set(f, 0);
    char *buf = (char *)allocfunc((usize)size + 1);
    if (!buf) {
        pltf_file_close(f);
        pltf_log("+++ err loading %s\n", filename);
        return 0;
    }
    pltf_file_r(f, buf, size);
    pltf_file_close(f);
    buf[size] = '\0';
    *txt_out  = (char *)buf;
    return 1;
}

bool32 json_root(const char *txt, json_s *tok_out)
{
    if (!txt) return 0;
    for (const char *c = txt; *c != '\0'; c++) {
        switch (*c) {
        case ' ':
        case '\n':
        case '\r':
        case '\t': break;
        case '{':
        case '[':
            if (tok_out) {
                json_s j = {0};
                j.c0     = (char *)c;
                j.c1     = NULL;
                j.nstack = 1;
                j.stack  = 0;
                *tok_out = j;
            }
            return 1;
        default: return 0;
        }
    }
    return 0;
}

i32 json_type(json_s j)
{
    if (!j.c0) return -1;
    switch (*j.c0) {
    case '\"': return JSON_TYPE_STR;
    case '{': return JSON_TYPE_OBJ;
    case '[': return JSON_TYPE_ARR;
    case 't':
    case 'f': return JSON_TYPE_BOL;
    case 'n': return JSON_TYPE_NUL;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': return JSON_TYPE_NUM;
    }
    return -1;
}

i32 json_depth(json_s j)
{
    // depth of obj and array is one lower
    return (j.c1 ? j.nstack : j.nstack - 1);
}

bool32 json_next(json_s tok, json_s *tok_out)
{
    json_s j = tok;
    if (0 < j.nstack && (j.stack & (1U << (j.nstack - 1)))) // was key-value
        j.nstack--;
    j.c0 = (j.c1 ? j.c1 : j.c0) + 1;
    for (; *j.c0 != '\0'; j.c0++) {
        switch (*j.c0) {
        case ' ': // white space
        case '\n':
        case '\r':
        case '\t': break;
        case ']':
        case '}':
            // reduce depth if was key value
            if (j.stack & (1U << (--j.nstack - 1))) {
                j.nstack--;
            }
            break;
        case ':':
            // incr depth with key-value indicator
            j.stack |= 1U << j.nstack++;
            break;
        case 'n':
        case 't':
            j.c1 = j.c0 + 3;
            if (tok_out)
                *tok_out = j;
            return 1;
        case 'f':
            j.c1 = j.c0 + 4;
            if (tok_out)
                *tok_out = j;
            return 1;
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            j.c1 = j.c0;
            while (1) {
                switch (*(++j.c1)) {
                case '\0':
                case '\n':
                case '\r':
                case '\t':
                case ' ':
                case '}':
                case ']':
                case ',':
                    j.c1--;
                    if (tok_out)
                        *tok_out = j;
                    return 1;
                }
            }
            break;
        case '\"': // string
            j.c1 = j.c0;
            for (int e = 0;;) {
                switch (*(++j.c1)) {
                case '\\': e = 1; break;
                case '\"':
                    if (e == 0) {
                        if (tok_out)
                            *tok_out = j;
                        return 1;
                    }
                    // fallthrough
                default: e = 0; break;
                }
            }
            break;
        case '{': // object
        case '[': // array
            j.c1 = NULL;
            j.stack &= ~(1U << j.nstack++);
            if (tok_out)
                *tok_out = j;
            return 1;
        }
    }
    return 0;
}

bool32 json_fchild(json_s tok, json_s *tok_out)
{
    json_s j;
    if (!json_next(tok, &j)) return 0;
    if (json_depth(j) <= json_depth(tok)) return 0;
    if (tok_out)
        *tok_out = j;
    return 1;
}

bool32 json_sibling(json_s tok, json_s *tok_out)
{
    i32    d = json_depth(tok);
    json_s a = tok;
    while (json_next(a, &a)) {
        i32 t = json_depth(a);
        if (t < d) return 0;
        if (t == d) {
            if (tok_out)
                *tok_out = a;
            return 1;
        }
    }
    return 0;
}

i32 json_num_children(json_s tok)
{
    json_s j;
    if (!json_fchild(tok, &j)) return 0;
    i32 n = 1;
    while (json_sibling(j, &j)) {
        n++;
    }
    return n;
}

bool32 json_key(json_s tok, const char *key, json_s *tok_out)
{
    if (json_type(tok) != JSON_TYPE_OBJ) return 0;

    json_s a;
    if (!json_fchild(tok, &a)) {
        return 0;
    }

    do {
        if (*a.c0 != '\"') continue;
        json_s b;
        if (!json_fchild(a, &b)) continue;

        char       *ca = a.c0 + 1;
        const char *cb = key;
        while (ca <= a.c1) {
            if (*cb == '\0') {
                if (tok_out)
                    *tok_out = b;
                return 1;
            }

            if (*ca++ != *cb++) break;
        }
    } while (json_sibling(a, &a));
    return 0;
}

i32 json_i32(json_s tok)
{
    char *c = tok.c0;
    if (!(('0' <= *c && *c <= '9') || *c == '-')) return 0;

    i32 res = 0;
    i32 s   = +1;
    if (*c == '-') {
        s = -1;
        c++;
    }
    while (c <= tok.c1) {
        res *= 10;
        res += num_from_hex(*c);
        c++;
    }
    return (res * s);
}

u32 json_u32(json_s tok)
{
    char *c = tok.c0;
    if (!('0' <= *c && *c <= '9')) return 0;

    u32 res = 0;
    while (c <= tok.c1) {
        res *= 10;
        res += num_from_hex(*c);
        c++;
    }
    return res;
}

f32 json_f32(json_s j)
{
    char *c    = j.c0;
    f32   res  = 0.f;
    f32   fact = 1.f;

    if (*c == '-') {
        fact = -1.f;
        c++;
    }

    bool32 pt = 0;
    while (c <= j.c1) {
        if (*c == '.') {
            pt = 1;
        } else {
            if (pt) fact *= .1f;
            res = res * 10.f + (f32)num_from_hex(*c);
        }
        c++;
    }
    return res * fact;
}

bool32 json_bool(json_s tok)
{
    return (*tok.c0 == 't' ? 1 : 0);
}

char *json_str(json_s tok, char *buf, u32 bufsize)
{
    if (json_type(tok) != JSON_TYPE_STR) {
        buf[0] = '\0';
        return NULL;
    }

    i32 i = 0;
    for (char *c = tok.c0 + 1; c < tok.c1; c++) {
        buf[i++] = *c;
        if ((u32)(i + 1) == bufsize) break;
    }

    buf[i] = '\0';
    return buf;
}

char *json_strp(json_s tok, int *len)
{
    if (json_type(tok) == JSON_TYPE_STR) {
        if (len) *len = (i32)(tok.c1 - tok.c0 - 1);
        return tok.c0 + 1;
    }
    if (len) *len = 0;
    return NULL;
}

i32 jsonk_i32(json_s tok, const char *key)
{
    json_s j;
    if (!json_key(tok, key, &j)) return 0;
    return json_i32(j);
}

u32 jsonk_u32(json_s tok, const char *key)
{
    json_s j;
    if (!json_key(tok, key, &j)) return 0;
    return json_u32(j);
}

f32 jsonk_f32(json_s tok, const char *key)
{
    json_s j;
    if (!json_key(tok, key, &j)) return 0;
    return json_f32(j);
}

bool32 jsonk_bool(json_s tok, const char *key)
{
    json_s j;
    if (!json_key(tok, key, &j)) return 0;
    return json_bool(j);
}

char *jsonk_str(json_s tok, const char *key, char *buf, u32 bufsize)
{
    json_s j;
    if (!json_key(tok, key, &j)) return 0;
    return json_str(j, buf, bufsize);
}

char *jsonk_strp(json_s tok, const char *key, int *len)
{
    json_s j;
    if (!json_key(tok, key, &j)) return 0;
    return json_strp(j, len);
}