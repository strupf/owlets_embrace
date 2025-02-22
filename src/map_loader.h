// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "gamedef.h"
#include "util/lzss.h"

typedef struct {
    u32 IID[4];
    u32 ID;
    u8  name[24];
    u16 bytes;  // total size in bytes
    u16 n_prop; // number of properties
    i16 x;
    i16 y;
    u16 w;
    u16 h;
    u8  tx; // tile data
    u8  ty;
    u8  tw;
    u8  th;
} map_obj_s;

#define map_obj_strs(MO, NAME, B) map_obj_str(MO, NAME, B, sizeof(B))
bool32 map_obj_has_nonnull_prop(map_obj_s *mo, const char *name);
bool32 map_obj_str(map_obj_s *mo, const char *name, void *b, u32 bs);
i32    map_obj_i32(map_obj_s *mo, const char *name);
f32    map_obj_f32(map_obj_s *mo, const char *name);
bool32 map_obj_bool(map_obj_s *mo, const char *name);
v2_i16 map_obj_pt(map_obj_s *mo, const char *name);
void  *map_obj_arr(map_obj_s *mo, const char *name, i32 *num);

void map_obj_parse(g_s *g, map_obj_s *o);
void game_load_map(g_s *g, u32 map_hash);

#endif