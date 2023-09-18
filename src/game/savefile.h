// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "gamedef.h"

enum {
        NUM_SAVEFILES = 3,
};

bool32 savefile_exists(int saveID);
bool32 savefile_write(int saveID, game_s *g);
bool32 savefile_read(int saveID, game_s *g);

#endif
