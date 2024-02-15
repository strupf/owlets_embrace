// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "gamedef.h"
#include "obj/hero.h"

typedef struct savefile_s {
    bool8 upgrades[NUM_HERO_UPGRADES];
    int   n_airjumps;
    int   health;
    char  area_filename[LEN_AREA_FILENAME];
    char  hero_name[LEN_HERO_NAME];
    //
    u8    saved_events[256];
} savefile_s;

bool32 savefile_copy(int slotID_from, int slotID_to);
bool32 savefile_delete(int slotID);
bool32 savefile_write(int slotID, savefile_s *sf);
bool32 savefile_read(int slotID, savefile_s *sf);
bool32 savefile_exists(int slotID);

#endif