// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "game_def.h"

enum {
        NUM_SAVEFILES = 3,
};

struct savefile_preview_s {
        int x;
};

bool32 game_savefile_new(game_s *g, int slotID);
bool32 game_savefile_load(game_s *g, int slotID);
bool32 game_savefile_save(game_s *g);
bool32 game_savefile_exists(int slotID);
bool32 game_savefile_copy(int slotID_from, int slotID_to);
bool32 game_savefile_delete(int slotID);

// tries to load a preview of the savefile if present
// (quick glance of the progress)
bool32 game_savefile_try_load_preview(int slotID, savefile_preview_s *p);

#endif