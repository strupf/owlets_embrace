// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void objs_update(g_s *g)
{
    for (obj_each(g, o)) {
        o->v_prev_q8 = o->v_q8;

        switch (o->ID) {
        default: break;
        case OBJ_ID_HERO: hero_on_update(g, o); break;
        case OBJ_ID_HOOK: hook_update(g, o); break;
        case OBJ_ID_FLYBLOB: flyblob_on_update(g, o); break;
        case OBJ_ID_PROJECTILE: projectile_on_update(g, o); break;
        case OBJ_ID_BUDPLANT: budplant_on_update(g, o); break;
        case OBJ_ID_SPRITEDECAL: spritedecal_on_update(g, o); break;
        case OBJ_ID_PUSHBLOCK: pushblock_on_update(g, o); break;
        case OBJ_ID_NPC: npc_on_update(g, o); break;
        case OBJ_ID_HERO_POWERUP: hero_powerup_obj_on_update(g, o); break;
        case OBJ_ID_CRUMBLEBLOCK: crumbleblock_on_update(g, o); break;
        case OBJ_ID_CRAWLER: crawler_on_update(g, o); break;
        case OBJ_ID_FROG: frog_on_update(g, o); break;
        case OBJ_ID_FALLINGBLOCK: fallingblock_on_update(g, o); break;
        }
    }
}
