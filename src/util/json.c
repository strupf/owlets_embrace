// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "json.h"
#include "str.h"
#include "sys/sys.h"

int txt_load_buf(const char *filename, char *buf, usize bufsize)
{
    void *f = sys_file_open(filename, SYS_FILE_R);
    if (!f) {
        sys_printf("+++ Can't open %s\n", filename);
        return TXT_ERR;
    }
    sys_file_seek(f, 0, SYS_FILE_SEEK_END);
    int size = sys_file_tell(f);
    sys_file_seek(f, 0, SYS_FILE_SEEK_SET);
    if ((usize)size + 1 >= bufsize) {
        sys_printf("+++ txt buf too small %s\n", filename);
        return TXT_ERR;
    }
    int read = sys_file_read(f, buf, size);
    sys_file_close(f);
    buf[read] = '\0';
    return TXT_SUCCESS;
}

int txt_load(const char *filename, void *(*allocfunc)(usize s), char **txt_out)
{
    void *f = sys_file_open(filename, SYS_FILE_R);
    if (!f) {
        sys_printf("+++ Can't open %s\n", filename);
        return TXT_ERR;
    }
    sys_file_seek(f, 0, SYS_FILE_SEEK_END);
    int size = sys_file_tell(f);
    sys_file_seek(f, 0, SYS_FILE_SEEK_SET);
    char *buf  = (char *)allocfunc((usize)size + 1);
    int   read = sys_file_read(f, buf, size);
    sys_file_close(f);
    buf[read] = '\0';
    *txt_out  = (char *)buf;
    return TXT_SUCCESS;
}

int json_root(const char *txt, json_s *tok_out)
{
    if (!txt) return JSON_ERR;
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
            return JSON_SUCCESS;
        default: return JSON_ERR;
        }
    }
    return JSON_ERR;
}

int json_type(json_s j)
{
    if (!j.c0) return JSON_ERR;
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
    return JSON_ERR;
}

int json_depth(json_s j)
{
    // depth of obj and array is one lower
    return (j.c1 ? j.nstack : j.nstack - 1);
}

int json_next(json_s tok, json_s *tok_out)
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
            return JSON_SUCCESS;
        case 'f':
            j.c1 = j.c0 + 4;
            if (tok_out)
                *tok_out = j;
            return JSON_SUCCESS;
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
                    return JSON_SUCCESS;
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
                        return JSON_SUCCESS;
                    }
                    e = 0;
                    break;
                }
            }
            break;
        case '{': // object
        case '[': // array
            j.c1 = NULL;
            j.stack &= ~(1U << j.nstack++);
            if (tok_out)
                *tok_out = j;
            return JSON_SUCCESS;
        }
    }
    return JSON_ERR;
}

int json_fchild(json_s tok, json_s *tok_out)
{
    json_s j;
    if (json_next(tok, &j) != JSON_SUCCESS) return JSON_ERR;
    if (json_depth(j) <= json_depth(tok)) return JSON_ERR;
    if (tok_out)
        *tok_out = j;
    return JSON_SUCCESS;
}

int json_sibling(json_s tok, json_s *tok_out)
{
    if (tok_out)
        *tok_out = (json_s){0};
    int    d = json_depth(tok);
    json_s a = tok;
    while (json_next(a, &a) == JSON_SUCCESS) {
        int t = json_depth(a);
        if (t < d) return JSON_ERR;
        if (t == d) {
            if (tok_out)
                *tok_out = a;
            return JSON_SUCCESS;
        }
    }
    return JSON_ERR;
}

int json_num_children(json_s tok)
{
    json_s j;
    if (json_fchild(tok, &j) != JSON_SUCCESS) return 0;
    int n = 1;
    while (json_sibling(j, &j) == JSON_SUCCESS) {
        n++;
    }
    return n;
}

int json_key(json_s tok, const char *key, json_s *tok_out)
{
    if (json_type(tok) != JSON_TYPE_OBJ) return JSON_ERR;

    json_s a;
    if (json_fchild(tok, &a) != JSON_SUCCESS) return JSON_ERR;
    do {
        if (*a.c0 != '\"') continue;
        json_s b;
        if (json_fchild(a, &b) != JSON_SUCCESS) continue;

        char       *ca = a.c0 + 1;
        const char *cb = key;
        while (ca <= a.c1) {
            if (*cb == '\0') {
                if (tok_out)
                    *tok_out = b;
                return JSON_SUCCESS;
            }

            if (*ca++ != *cb++) break;
        }
    } while (json_sibling(a, &a) == JSON_SUCCESS);
    return JSON_ERR;
}

i32 json_i32(json_s tok)
{
    char *c = tok.c0;
    if (!(('0' <= *c && *c <= '9') || *c == '-')) return 0;

    i32 res = 0;
    int s   = +1;
    if (*c == '-') {
        s = -1;
        c++;
    }
    while (c <= tok.c1) {
        res *= 10;
        res += char_int_from_hex(*c);
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
        res += char_int_from_hex(*c);
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
            res = res * 10.f + (f32)char_int_from_hex(*c);
        }
        c++;
    }
    return res * fact;
}

int json_bool(json_s tok)
{
    return (*tok.c0 == 't' ? 1 : 0);
}

char *json_str(json_s tok, char *buf, usize bufsize)
{
    if (json_type(tok) != JSON_TYPE_STR) {
        buf[0] = '\0';
        return NULL;
    }

    int i = 0;
    for (char *c = tok.c0 + 1; c < tok.c1; c++) {
        buf[i++] = *c;
        if ((usize)(i + 1) == bufsize) break;
    }

    buf[i] = '\0';
    return buf;
}

char *json_strp(json_s tok, int *len)
{
    if (json_type(tok) == JSON_TYPE_STR) {
        if (len) *len = (int)(tok.c1 - tok.c0 - 1);
        return tok.c0 + 1;
    }
    if (len) *len = 0;
    return NULL;
}

i32 jsonk_i32(json_s tok, const char *key)
{
    json_s j;
    if (json_key(tok, key, &j) != JSON_SUCCESS) return 0;
    return json_i32(j);
}

u32 jsonk_u32(json_s tok, const char *key)
{
    json_s j;
    if (json_key(tok, key, &j) != JSON_SUCCESS) return 0;
    return json_u32(j);
}

f32 jsonk_f32(json_s tok, const char *key)
{
    json_s j;
    if (json_key(tok, key, &j) != JSON_SUCCESS) return 0;
    return json_f32(j);
}

int jsonk_bool(json_s tok, const char *key)
{
    json_s j;
    if (json_key(tok, key, &j) != JSON_SUCCESS) return 0;
    return json_bool(j);
}

char *jsonk_str(json_s tok, const char *key, char *buf, usize bufsize)
{
    json_s j;
    if (json_key(tok, key, &j) != JSON_SUCCESS) return 0;
    return json_str(j, buf, bufsize);
}

char *jsonk_strp(json_s tok, const char *key, int *len)
{
    json_s j;
    if (json_key(tok, key, &j) != JSON_SUCCESS) return 0;
    return json_strp(j, len);
}