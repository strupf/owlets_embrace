// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "gamedef.h"

typedef struct savefile_s {
    i32     tick;
    flags32 aquired_upgrades;
    flags32 aquired_items;
    int     n_airjumps;
    int     health;
    char    area_filename[LEN_AREA_FILENAME];
    char    hero_name[LEN_HERO_NAME];
} savefile_s;

bool32 savefile_copy(int slotID_from, int slotID_to);
bool32 savefile_delete(int slotID);
bool32 savefile_write(int slotID, savefile_s *sf);
bool32 savefile_read(int slotID, savefile_s *sf);
bool32 savefile_exists(int slotID);

#endif