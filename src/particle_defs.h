// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PARTICLE_DEFS_H
#define PARTICLE_DEFS_H

#include "particle.h"

// sudden, one time particle emission
enum {
    PARTICLE_EMIT_ID_HERO_LAND,
    PARTICLE_EMIT_ID_HERO_LAND_HARD,
    PARTICLE_EMIT_ID_HERO_JUMP,
    PARTICLE_EMIT_ID_HERO_JUMP_AIR,
    PARTICLE_EMIT_ID_HERO_WALK,
    PARTICLE_EMIT_ID_HERO_WALK_FAST,
    PARTICLE_EMIT_ID_HERO_WATER_SPLASH,
};

void particle_emit_ID(g_s *g, i32 ID, v2_i32 p);

// particle emitters are supposed to be continously emitting particles
enum {
    PARTICLE_EMITTER_ID_0,
};

void particle_emitter_ID_create(g_s *g, i32 ID, v2_i32 p, obj_s *o);

#endif