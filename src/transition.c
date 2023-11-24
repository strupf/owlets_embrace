// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "transition.h"
#include "assets.h"
#include "game.h"

enum {
    TRANSITION_PHASE_NONE,
    TRANSITION_PHASE_OUT,
    TRANSITION_PHASE_BLACK,
    TRANSITION_PHASE_IN,
};

#define TRANSITION_TICKS_OUT   20
#define TRANSITION_TICKS_BLACK 10
#define TRANSITION_TICKS_IN    20

void transition_start(transition_s *t, const char *file)
{
    t->tick  = 0;
    t->phase = TRANSITION_PHASE_OUT;
    str_cpy(t->to_load, "assets/map/proj/");
    str_append(t->to_load, file);
    str_append(t->to_load, ".ldtkl");
}

void transition_update(game_s *g, transition_s *t)
{
    t->tick++;
    switch (t->phase) {
    case TRANSITION_PHASE_OUT: {
        if (t->tick < TRANSITION_TICKS_OUT) break;
        t->tick = 0;
        t->phase++;
    } break;
    case TRANSITION_PHASE_BLACK: {
        if (t->tick < TRANSITION_TICKS_BLACK) break;
        t->tick = 0;
        t->phase++;

        game_load_map(g, t->to_load);

        obj_s *hero  = obj_hero_create(g);
        hero->pos.x  = t->heroaabb.x;
        hero->pos.y  = t->heroaabb.y;
        hero->facing = t->hero_face;
        hero->vel_q8 = t->hero_v;
        cam_s *cam   = &g->cam;
        v2_i32 hpos  = obj_pos_center(hero);
        cam_set_pos_px(cam, hpos.x, hpos.y);
        cam_constrain_to_room(g, cam);
    } break;
    case TRANSITION_PHASE_IN: {
        if (t->tick < TRANSITION_TICKS_IN) break;
        t->tick  = 0;
        t->phase = 0;
    } break;
    }
}

bool32 transition_finished(transition_s *t)
{
    return (t->phase == TRANSITION_PHASE_NONE);
}

void transition_draw(transition_s *t)
{
    if (transition_finished(t)) return;

    gfx_pattern_s pat = {0};

    switch (t->phase) {
    case TRANSITION_PHASE_OUT:
        pat = gfx_pattern_interpolate(POW2(t->tick), POW2(TRANSITION_TICKS_OUT));
        break;
    case TRANSITION_PHASE_BLACK:
        pat = gfx_pattern_interpolate(1, 1);
        break;
    case TRANSITION_PHASE_IN: {
        int t2 = POW2(TRANSITION_TICKS_IN);
        int t1 = t2 - POW2(t->tick);
        pat    = gfx_pattern_interpolate(t1, t2);
    } break;
    }

    tex_s display = asset_tex(0);
    u32  *px      = (u32 *)display.px;
    for (int y = 0; y < display.h; y++) {
        u32 p = pat.p[y & 7];
        for (int x = 0; x < display.wword; x++) {
            *px++ &= ~p;
        }
    }
}
