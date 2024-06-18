// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef JSON_H
#define JSON_H

#include "pltf/pltf.h"

enum {
    JSON_TYPE_NUL,
    JSON_TYPE_ARR,
    JSON_TYPE_OBJ,
    JSON_TYPE_NUM,
    JSON_TYPE_STR,
    JSON_TYPE_BOL,
};

// contains start and end char pointers (inclusive) for the range of the token
// stack used for keeping track of depth and is implemented as a bitset
typedef struct {
    char *c0;
    char *c1; // NULL for arrays and objects
    i32   stack;
    u32   nstack;
} json_s;

typedef struct {
    json_s tok;
} json_it_s;

bool32 txt_load_buf(const char *filename, char *buf, u32 bufsize);
bool32 txt_load(const char *filename, void *(*allocfunc)(u32 s), char **txt_out);
//
bool32 json_root(const char *txt, json_s *tok_out);
i32    json_type(json_s j);
i32    json_depth(json_s j);
bool32 json_next(json_s tok, json_s *tok_out);
bool32 json_fchild(json_s tok, json_s *tok_out);
bool32 json_sibling(json_s tok, json_s *tok_out);
i32    json_num_children(json_s tok);
i32    json_key(json_s tok, const char *key, json_s *tok_out);
i32    json_i32(json_s tok);
u32    json_u32(json_s tok);
f32    json_f32(json_s tok);
bool32 json_bool(json_s tok);
char  *json_str(json_s tok, char *buf, u32 bufsize);
char  *json_strp(json_s tok, int *len);
i32    jsonk_i32(json_s tok, const char *key);
u32    jsonk_u32(json_s tok, const char *key);
f32    jsonk_f32(json_s tok, const char *key);
bool32 jsonk_bool(json_s tok, const char *key);
char  *jsonk_str(json_s tok, const char *key, char *buf, u32 bufsize);
char  *jsonk_strp(json_s tok, const char *key, int *len);

#define jsonk_strs(JTOK, KEY, BUF) jsonk_str(JTOK, KEY, BUF, sizeof(BUF))

static json_s json_for_init(json_s tok, const char *key)
{
    json_s j;
    json_key(tok, key, &j);
    return j;
}

static bool32 json_for_valid(json_s *it, json_s *jit)
{
    if (it->c0 == NULL) return 0;
    if (json_type(*it) == JSON_TYPE_ARR && !json_fchild(*it, it))
        return 0; // no first child
    json_sibling(*it, jit);
    return 1;
}

#define json_each(ITROOT, KEY, IT)                 \
    json_s _##IT, IT = json_for_init(ITROOT, KEY); \
    json_for_valid(&IT, &_##IT);                   \
    IT = _##IT

#endif