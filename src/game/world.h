// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef WORLD_H
#define WORLD_H

#include "os/os.h"

typedef struct {
        char    filename[64];
        rec_i32 r; // bounds
} world_area_def_s;

typedef struct {
        char             filename[64];
        world_area_def_s areas[64];
        int              n_areas;
} world_def_s;

// loads a Tiled .world file
void              world_def_init();
world_area_def_s *world_area_by_filename(const char *areafile);
world_def_s      *world_area_parent(const char *areafile);
world_area_def_s *world_get_area(world_def_s *w, world_area_def_s *curr_area, rec_i32 rec);

#endif