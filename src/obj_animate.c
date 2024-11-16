// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void objs_animate(g_s *g)
{
    for (obj_each(g, o)) {
        switch (o->ID) {
        default: break;
        case OBJ_ID_HERO: hero_on_animate(g, o); break;
        case OBJ_ID_HOOK: hook_on_animate(g, o); break;
        case OBJ_ID_FLYBLOB: flyblob_on_animate(g, o); break;
        case OBJ_ID_PROJECTILE: projectile_on_animate(g, o); break;
        case OBJ_ID_BUDPLANT: budplant_on_animate(g, o); break;
        case OBJ_ID_SPRITEDECAL: spritedecal_on_animate(g, o); break;
        case OBJ_ID_NPC: npc_on_animate(g, o); break;
        case OBJ_ID_STAMINARESTORER: staminarestorer_on_animate(g, o); break;
        case OBJ_ID_CRUMBLEBLOCK: crumbleblock_on_animate(g, o); break;
        case OBJ_ID_CRAWLER: crawler_on_animate(g, o); break;
        case OBJ_ID_FROG: frog_on_animate(g, o); break;
        case OBJ_ID_TOGGLEBLOCK: toggleblock_on_animate(g, o); break;
        }
    }
}