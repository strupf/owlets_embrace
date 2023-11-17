// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include "gamedef.h"

typedef struct {
    rec_i32 r;
    GUID_s  GUID;
    char    filename[64];
} map_room_s;

typedef struct {
    int         n_rooms;
    map_room_s *roomcur;
    map_room_s  rooms[64];
} map_world_s;

void        game_load_map(game_s *g, const char *filename);
void        map_world_load(map_world_s *world, const char *filename);
map_room_s *map_world_overlapped_room(map_world_s *world, rec_i32 r);
map_room_s *map_world_find_room(map_world_s *world, const char *filename);

#endif