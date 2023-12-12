// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "gamedef.h"

typedef struct {
    char filename[64];
    int  x;
    int  y;
    int  w;
    int  h;
} map_worldroom_s;

typedef struct {
    map_worldroom_s *roomcur;
    int              n_rooms;
    map_worldroom_s  rooms[64];
} map_world_s;

void             game_load_map(game_s *g, const char *worldfile);
void             map_world_load(map_world_s *world, const char *mapfile);
map_worldroom_s *map_world_overlapped_room(map_world_s *world, rec_i32 r);
map_worldroom_s *map_world_find_room(map_world_s *world, const char *mapfile);

#endif