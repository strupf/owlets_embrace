// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "particle_defs.h"
#include "game.h"

void particle_emit_ID(g_s *g, i32 ID, v2_i32 p)
{
    particle_sys_s *ps = &g->particle_sys;
    particle_emit_s e  = {0};
    e.p                = v2_i16_from_i32(p);

    bool32 shuffle = 0;
    i32    nprev   = ps->n;
    i32    n       = 0;

    switch (ID) {
    case PARTICLE_EMIT_ID_STOMPBLOCK_DESTROY: {
        n              = 1;
        //
        e.type         = PARTICLE_TYPE_TEX;
        e.tex.w        = 64 / 8;
        e.tex.h        = 64 / 8;
        e.tex.y        = 32;
        e.tex.n_frames = 9;
        e.ticks_min    = 22;
        e.ticks_max    = e.ticks_min;
        e.p.x -= 32;
        e.p.y -= 40;
        break;
    }
    case PARTICLE_EMIT_ID_CHEST: {
        n              = 16;
        e.type         = PARTICLE_TYPE_REC;
        //
        e.size_beg_min = 2;
        e.size_beg_max = 4;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 35;
        e.ticks_max    = e.ticks_min + 35;
        e.p_range.x    = 18;
        e.p_range.y    = 4;
        e.v_q8.y       = -600;
        e.v_q8_range.y = 200;
        e.v_q8_range.x = 200;
        e.a_q8.y       = 30;

        e.mode = PRIM_MODE_BLACK;
        particle_emitter_emit(g, &e, n);
        e.mode  = PRIM_MODE_WHITE;
        shuffle = 1;
        break;
    }
    case PARTICLE_EMIT_ID_HERO_SQUISH: {
        n              = 30;
        e.type         = PARTICLE_TYPE_CIR | PARTICLE_FLAG_FADE_OUT;
        //
        e.size_beg_min = 4;
        e.size_beg_max = 8;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 25;
        e.ticks_max    = e.ticks_min + 20;
        e.p_range_r    = 14;
        e.v_q8_range.y = 1800;
        e.v_q8_range.x = 1800;
        e.a_q8.y       = 10;
        e.drag         = 30;
        e.mode         = PRIM_MODE_BLACK;
        particle_emitter_emit(g, &e, n);
        e.mode  = PRIM_MODE_WHITE;
        shuffle = 1;
        break;
    }
    case PARTICLE_EMIT_ID_HERO_SWING: {
        n              = 1;
        e.type         = PARTICLE_TYPE_REC | PARTICLE_FLAG_FADE_OUT;
        //
        e.size_beg_min = 2;
        e.size_beg_max = 4;
        e.size_end_min = 1;
        e.size_end_max = 2;
        e.ticks_min    = 10;
        e.ticks_max    = e.ticks_min + 8;
        e.p_range_r    = 8;
        e.v_q8_range.y = 100;
        e.v_q8_range.x = 100;
        e.a_q8.y       = 0;
        e.drag         = 0;
        e.mode         = rngr_i32(0, 1) ? PRIM_MODE_BLACK : PRIM_MODE_WHITE;
        // particle_emitter_emit(g, &e, n);
        //  e.mode  = PRIM_MODE_WHITE;
        //  shuffle = 1;
        break;
    }
    case PARTICLE_EMIT_ID_STOMP: {
        n              = 10;
        e.type         = PARTICLE_TYPE_CIR;
        //
        e.size_beg_min = 4;
        e.size_beg_max = 8;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 18;
        e.ticks_max    = e.ticks_min + 10;
        e.p_range.x    = 14;
        e.p_range.y    = 4;
        e.v_q8.y       = -200;
        e.v_q8_range.y = 100;
        e.v_q8_range.x = 600;
        e.a_q8.y       = 10;

        e.mode = PRIM_MODE_BLACK;
        particle_emitter_emit(g, &e, n);
        e.mode  = PRIM_MODE_WHITE;
        shuffle = 1;
        break;
    }
    case PARTICLE_EMIT_ID_STOMPBLOCK_HINT: {
        n              = 8;
        e.type         = PARTICLE_TYPE_REC;
        //
        e.size_beg_min = 1;
        e.size_beg_max = 3;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 16;
        e.ticks_max    = e.ticks_min + 8;
        e.p_range.x    = 8;
        e.p_range.y    = 4;
        e.v_q8.y       = -100;
        e.v_q8_range.y = 100;
        e.v_q8_range.x = 50;
        e.a_q8.y       = 20;

        e.mode = PRIM_MODE_BLACK;
        particle_emitter_emit(g, &e, n);
        e.mode  = PRIM_MODE_WHITE;
        shuffle = 1;
        break;
    }
    case PARTICLE_EMIT_ID_CRUMBLEBLOCK: {
        n              = 12;
        e.type         = PARTICLE_TYPE_REC;
        //
        e.size_beg_min = 1;
        e.size_beg_max = 3;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 14;
        e.ticks_max    = e.ticks_min + 10;
        e.p_range.x    = 8;
        e.p_range.y    = e.p_range.x;
        e.v_q8.y       = -200;
        e.v_q8_range.y = 80;
        e.v_q8_range.x = 80;
        e.a_q8.y       = 20;

        e.mode = PRIM_MODE_BLACK;
        particle_emitter_emit(g, &e, n);
        e.mode  = PRIM_MODE_WHITE;
        shuffle = 1;
        break;
    }
    case PARTICLE_EMIT_ID_HERO_LAND_HARD:
    case PARTICLE_EMIT_ID_HERO_LAND: {
        i32 k = (ID == PARTICLE_EMIT_ID_HERO_LAND_HARD);

        n      = 12 + 4 * k;
        e.type = PARTICLE_TYPE_REC |
                 PARTICLE_FLAG_FADE_OUT;
        e.mode         = PRIM_MODE_BLACK;
        //
        e.size_beg_min = 2 + 1 * k;
        e.size_beg_max = 4 + 1 * k;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 20 + 5 * k;
        e.ticks_max    = e.ticks_min + 20;
        e.p_range.x    = 12;
        e.p_range.y    = 4;
        e.v_q8.y       = -260 - 50 * k;
        e.v_q8_range.y = 100;
        e.v_q8_range.x = 250 + 50 * k;
        e.a_q8.y       = 60;
        break;
    }
    case PARTICLE_EMIT_ID_HERO_JUMP: {
        n      = 12;
        e.type = PARTICLE_TYPE_REC |
                 PARTICLE_FLAG_FADE_OUT;
        e.mode         = PRIM_MODE_BLACK;
        //
        e.size_beg_min = 2;
        e.size_beg_max = 3;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 15;
        e.ticks_max    = e.ticks_min + 15;
        e.p_range.x    = 14;
        e.p_range.y    = 4;
        e.v_q8.y       = -350;
        e.v_q8_range.y = 100;
        e.v_q8_range.x = 100;
        e.a_q8.y       = 40;
        break;
    }
    case PARTICLE_EMIT_ID_HERO_JUMP_AIR: {
        n              = 1;
        e.type         = PARTICLE_TYPE_TEX;
        e.mode         = rngr_i32(0, 1) ? SPR_FLIP_X : 0;
        //
        e.ticks_min    = 16;
        e.ticks_max    = e.ticks_min;
        e.tex.n_frames = 6;
        e.tex.x        = 0;
        e.tex.y        = 2;
        e.tex.w        = 4;
        e.tex.h        = 6;
        break;
    }
    case PARTICLE_EMIT_ID_HERO_WALK_FAST:
    case PARTICLE_EMIT_ID_HERO_WALK: {
        i32 k = (ID == PARTICLE_EMIT_ID_HERO_WALK_FAST);

        n              = 4 + 2 * k;
        e.type         = PARTICLE_TYPE_REC;
        e.mode         = PRIM_MODE_BLACK;
        //
        e.size_beg_min = 2 + 1 * k;
        e.size_beg_max = 4 + 1 * k;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 12 + 4 * k;
        e.ticks_max    = e.ticks_min + 10;
        e.p_range.x    = 12;
        e.v_q8.y       = -200 - 50 * k;
        e.v_q8_range.y = 50;
        e.v_q8_range.x = 200;
        e.a_q8.y       = 30;
        break;
    }
    case PARTICLE_EMIT_ID_HERO_WATER_SPLASH_BIG:
    case PARTICLE_EMIT_ID_HERO_WATER_SPLASH: {
        i32 k = (ID == PARTICLE_EMIT_ID_HERO_WATER_SPLASH_BIG);

        n      = 20 + 8 * k;
        e.type = PARTICLE_TYPE_REC |
                 PARTICLE_FLAG_FADE_OUT;
        //
        e.size_beg_min = 2;
        e.size_beg_max = 4 + 2 * k;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 30 + 10 * k;
        e.ticks_max    = e.ticks_min + 30;
        e.p_range.x    = 12;
        e.p_range.y    = 4;
        e.v_q8.y       = -360 - 100 * k;
        e.v_q8_range.y = 200 + 50 * k;
        e.v_q8_range.x = 350;
        e.a_q8.y       = 40;

        e.mode = PRIM_MODE_BLACK;
        particle_emitter_emit(g, &e, n);
        e.mode  = PRIM_MODE_WHITE;
        shuffle = 1;
        break;
    }
    case PARTICLE_EMIT_ID_HOOK_TERRAIN: {
        n              = 30;
        e.type         = PARTICLE_TYPE_REC;
        //
        e.size_beg_min = 2;
        e.size_beg_max = 4;
        e.size_end_min = 0;
        e.size_end_max = 0;
        e.ticks_min    = 12;
        e.ticks_max    = e.ticks_min + 12;
        e.p_range_r    = 12;
        e.v_q8_range.y = 250;
        e.v_q8_range.x = e.v_q8_range.y;

        e.mode = PRIM_MODE_WHITE;
        particle_emitter_emit(g, &e, n);
        e.mode  = PRIM_MODE_BLACK;
        shuffle = 1;
        break;
    }
    }

    particle_emitter_emit(g, &e, n);

    if (shuffle) {
        particle_sys_shuffle(ps, nprev, ps->n - nprev - 1);
    }
}

void particle_emitter_ID_create(g_s *g, i32 ID, v2_i32 p, obj_s *o)
{
    switch (ID) {
    case PARTICLE_EMITTER_ID_0: {
        break;
    }
    }
}
