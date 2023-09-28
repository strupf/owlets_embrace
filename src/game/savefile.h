// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "game_def.h"

enum {
        NUM_SAVEFILES = 3,
};

typedef struct savefile_s {
        bool32 in_use;
        int    saveslotID;
        i32    tick;
        u32    savepointID;
        char   area_filename[LEN_STR_AREA_FILENAME];
        char   hero_name[LEN_STR_HERO_NAME];
} savefile_s;

bool32 game_savefile_new(game_s *g, int slotID);
bool32 game_savefile_load(game_s *g, int slotID);
bool32 game_savefile_save(game_s *g);

bool32 savefile_copy(int slotID_from, int slotID_to);
bool32 savefile_delete(int slotID);
bool32 savefile_write(int slotID, savefile_s *sf);
bool32 savefile_read(int slotID, savefile_s *sf);

#endif