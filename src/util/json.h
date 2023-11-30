// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef JSON_H
#define JSON_H

#include "sys/sys_types.h"

enum {
    JSON_TYPE_NUL,
    JSON_TYPE_ARR,
    JSON_TYPE_OBJ,
    JSON_TYPE_NUM,
    JSON_TYPE_STR,
    JSON_TYPE_BOL,
};

enum {
    JSON_SUCCESS = 0,
    JSON_ERR     = -1,
};

enum {
    TXT_SUCCESS = 0,
    TXT_ERR     = -1,
};

// contains start and end char pointers (inclusive) for the range of the token
// stack used for keeping track of depth and is implemented as a bitset
typedef struct {
    char *c0;
    char *c1; // NULL for arrays and objects
    int   stack;
    u32   nstack;
} json_s;

typedef struct {
    json_s tok;
} json_it_s;

int   txt_load_buf(const char *filename, char *buf, usize bufsize);
int   txt_load(const char *filename, void *(*allocfunc)(usize s), char **txt_out);
//
int   json_root(const char *txt, json_s *tok_out);
int   json_type(json_s j);
int   json_depth(json_s j);
int   json_next(json_s tok, json_s *tok_out);
int   json_fchild(json_s tok, json_s *tok_out);
int   json_sibling(json_s tok, json_s *tok_out);
int   json_num_children(json_s tok);
int   json_key(json_s tok, const char *key, json_s *tok_out);
i32   json_i32(json_s tok);
u32   json_u32(json_s tok);
f32   json_f32(json_s tok);
int   json_bool(json_s tok);
char *json_str(json_s tok, char *buf, usize bufsize);
char *json_strp(json_s tok, int *len);
i32   jsonk_i32(json_s tok, const char *key);
u32   jsonk_u32(json_s tok, const char *key);
f32   jsonk_f32(json_s tok, const char *key);
int   jsonk_bool(json_s tok, const char *key);
char *jsonk_str(json_s tok, const char *key, char *buf, usize bufsize);
char *jsonk_strp(json_s tok, const char *key, int *len);

#define jsonk_strs(JTOK, KEY, BUF) jsonk_str(JTOK, KEY, BUF, sizeof(BUF))

static json_s json_for_init(json_s tok, const char *key)
{
    json_s j;
    json_key(tok, key, &j);
    return j;
}

static int json_for_valid(json_s *it, json_s *jit)
{
    if (it->c0 == NULL) return 0;
    if (json_type(*it) == JSON_TYPE_ARR && json_fchild(*it, it) != JSON_SUCCESS)
        return 0; // no first child
    json_sibling(*it, jit);
    return 1;
}

#define json_each(ITROOT, KEY, IT)                 \
    json_s _##IT, IT = json_for_init(ITROOT, KEY); \
    json_for_valid(&IT, &_##IT);                   \
    IT = _##IT

#endif