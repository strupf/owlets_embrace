// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// the name is a bit too general but a script in this context
// is a piece of logic which can be added to the update/animate pipeline

#ifndef SCRIPT_H
#define SCRIPT_H

#include "gamedef.h"

typedef struct {
    i32   priority;
    i32   tick;
    void *arg;
    void (*on_update)(g_s *g, script_s *s, void *arg);
    void (*on_animate)(g_s *g, script_s *s, void *arg);
} script_s;

#endif
