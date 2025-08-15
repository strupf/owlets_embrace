// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "gamedef.h"
#include "tile_types.h"
#include "util/lzss.h"

typedef struct {
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

typedef struct {
    u32 ID;
    u8  level[MAP_WAD_NAME_LEN];
} map_obj_ref_s;

#define map_obj_strs(MO, NAME, B) map_obj_str(MO, NAME, B, sizeof(B))
bool32         map_obj_has_nonnull_prop(map_obj_s *mo, const char *name);
bool32         map_obj_str(map_obj_s *mo, const char *name, void *b, u32 bs);
i32            map_obj_i32(map_obj_s *mo, const char *name);
f32            map_obj_f32(map_obj_s *mo, const char *name);
bool32         map_obj_bool(map_obj_s *mo, const char *name);
v2_i16         map_obj_pt(map_obj_s *mo, const char *name);
void          *map_obj_arr(map_obj_s *mo, const char *name, i32 *num);
map_obj_ref_s *map_obj_ref(map_obj_s *mo, const char *name);
void           map_obj_parse(g_s *g, map_obj_s *o);
void           game_load_map(g_s *g, u8 *map_name);
map_obj_s     *map_obj_find(g_s *g, const char *name);

// a_x/a_y: -1; 0; +1 -> alignment to map_obj (left, center, right)
void obj_place_to_map_obj(obj_s *o, map_obj_s *mo, i32 a_x, i32 a_y);

void loader_do_terrain(g_s *g, u16 *tmem, i32 w, i32 h, u32 *seed_visuals);

static i32 map_terrain_enum_to_type(i32 v)
{
    switch (v) {
    case 0: return TILE_TYPE_BRIGHT_STONE;
    case 1: return TILE_TYPE_DARK_STONE;
    case 2: return TILE_TYPE_DARK_LEAVES;
    case 3: return TILE_TYPE_DARK_STONE_PEBBLE;
    case 4: return TILE_TYPE_BRIGHT_SNOW;
    case 5: return TILE_TYPE_DARK_OBSIDIAN;
    }
    return TILE_TYPE_BRIGHT_STONE;
}

static inline i32 map_terrain_pack(i32 type, i32 shape)
{
    return ((type << 8) | shape);
}

static inline i32 map_terrain_type(u16 t)
{
    return (t >> 8);
}

static inline i32 map_terrain_shape(u16 t)
{
    return (t & B16(00000000, 11111111));
}

#endif