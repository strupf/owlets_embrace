// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef INGAMEMENU_H
#define INGAMEMENU_H

#include "gamedef.h"

typedef struct {
    int x;
} ingamemenu_s;

bool32 ingamemenu_active(ingamemenu_s *m);
void   ingamemenu_update(game_s *g, ingamemenu_s *m);
void   ingamemenu_draw(game_s *g, ingamemenu_s *m);

#endif