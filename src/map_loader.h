// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "gamedef.h"

typedef struct {
    i32 ID; // 1...
    i32 x;
    i32 y;
    u16 w;
    u16 h;

    char filename[64];
} map_worldroom_s;

typedef struct {
    u32             n_rooms;
    map_worldroom_s rooms[NUM_WORLD_ROOMS];
} map_world_s;

typedef struct {
    u32  IID[4];
    u32  ID;
    u16  bytes;  // total size in bytes
    u16  n_prop; // number of properties
    i16  x;
    i16  y;
    u16  w;
    u16  h;
    u8   tx; // tile data
    u8   ty;
    u8   tw;
    u8   th;
    char name[32];
} map_obj_s;

#define map_obj_strs(MO, NAME, B) map_obj_str(MO, NAME, B, sizeof(B))
bool32 map_obj_has_nonnull_prop(map_obj_s *mo, const char *name);
void   map_obj_str(map_obj_s *mo, const char *name, void *b, u32 bs);
i32    map_obj_i32(map_obj_s *mo, const char *name);
f32    map_obj_f32(map_obj_s *mo, const char *name);
bool32 map_obj_bool(map_obj_s *mo, const char *name);
v2_i16 map_obj_pt(map_obj_s *mo, const char *name);
void  *map_obj_arr(map_obj_s *mo, const char *name, i32 *num);

void             game_load_map(game_s *g, const char *worldfile);
void             game_prepare_new_map(game_s *g);
void             map_world_load(map_world_s *world, const char *mapfile);
map_worldroom_s *map_world_overlapped_room(map_world_s *world, map_worldroom_s *cur, rec_i32 r);
map_worldroom_s *map_world_find_room(map_world_s *world, const char *mapfile);
map_worldroom_s *map_worldroom_by_objID(map_world_s *world, u32 objID);

#endif