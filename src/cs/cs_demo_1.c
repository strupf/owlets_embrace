// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CS_DEMO_1_ST_IDLE = -1,
    CS_DEMO_1_ST_INTRO,
    CS_DEMO_1_ST_FIND,
};

void cs_demo_1_update(g_s *g, cs_s *cs);
void cs_demo_1_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_demo_1_cb_1(g_s *g, obj_s *o);
void cs_demo_1_cb_2(g_s *g, obj_s *o);
void cs_demo_1_cb_3(g_s *g, obj_s *o);
void cs_demo_1_cb_4(g_s *g, obj_s *o);

void cs_demo_1_enter(g_s *g)
{
    cs_s *cs = &g->cuts;
    mclr(cs, sizeof(cs_s));
    cs->on_update         = cs_demo_1_update;
    cs->on_trigger        = cs_demo_1_on_trigger;
    g->block_hero_control = 1;
}

void cs_demo_1_update(g_s *g, cs_s *cs)
{
    obj_s *ocomp = obj_get_tagged(g, OBJ_TAG_COMPANION);

    switch (cs->phase) {
    default: break;
    case CS_DEMO_1_ST_INTRO: {
        if (!cs_wait_and_pause_for_hero_idle(g)) break;

        cs->phase       = CS_DEMO_1_ST_IDLE;
        g->block_update = 1;
        companion_cs_start(ocomp);
        companion_cs_set_anim(ocomp, COMPANION_CS_ANIM_FLY, +1);
        companion_cs_move_to(ocomp, (v2_i32){300, 120}, 5, cs_demo_1_cb_1);
        break;
    }
    case CS_DEMO_1_ST_FIND: {
        if (!g->dialog.state) {
            companion_cs_set_anim(ocomp, COMPANION_CS_ANIM_FLY, -1);
            companion_cs_move_to(ocomp, (v2_i32){100, 60}, 5, 0);
            cs->phase = CS_DEMO_1_ST_IDLE;
        }
        break;
    }
    }
}

void cs_demo_1_cb_1(g_s *g, obj_s *o)
{
    companion_cs_set_anim(o, COMPANION_CS_ANIM_NOD, -1);
    g->cuts.phase = CS_DEMO_1_ST_FIND;
    g->cuts.tick  = 0;
    dialog_open_wad(g, "D_001");
}

void cs_demo_1_cb_2(g_s *g, obj_s *o)
{
}

void cs_demo_1_cb_3(g_s *g, obj_s *o)
{
}

void cs_demo_1_cb_4(g_s *g, obj_s *o)
{
}

void cs_demo_1_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
}

void cs_demo_1_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
}