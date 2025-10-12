// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef CREDITS_H
#define CREDITS_H

#include "gamedef.h"

// TODO

typedef struct {
    i32 x;
} credits_s;

void credits_enter(credits_s *c);
void credits_update(credits_s *c);
void credits_draw(credits_s *c);

#endif