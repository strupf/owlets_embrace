// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void objs_animate(g_s *g)
{
    for (obj_each(g, o)) {
        switch (o->ID) {
        default: break;
        case OBJID_HERO: hero_on_animate(g, o); break;
        // case OBJID_HOOK: hook_on_animate(g, o); break;
        case OBJID_FLYBLOB: flyblob_on_animate(g, o); break;
        case OBJID_PROJECTILE: projectile_on_animate(g, o); break;
        case OBJID_BUDPLANT: budplant_on_animate(g, o); break;
        case OBJID_SPRITEDECAL: spritedecal_on_animate(g, o); break;
        case OBJID_NPC: npc_on_animate(g, o); break;
        case OBJID_STAMINARESTORER: staminarestorer_on_animate(g, o); break;
        case OBJID_CRUMBLEBLOCK: crumbleblock_on_animate(g, o); break;
        case OBJID_CRAWLER: crawler_on_animate(g, o); break;
        case OBJID_FROG: frog_on_animate(g, o); break;
        case OBJID_TOGGLEBLOCK: toggleblock_on_animate(g, o); break;
        case OBJID_ROTOR: rotor_on_animate(g, o); break;
        case OBJID_CHEST: chest_on_animate(g, o); break;
        case OBJID_DOOR: door_on_animate(g, o); break;
        case OBJID_BOULDER: boulder_on_animate(g, o); break;
        case OBJID_FALLINGSTONE: fallingstone_on_animate(g, o); break;
        case OBJID_STALACTITE: stalactite_on_animate(g, o); break;
        case OBJID_BITER: biter_on_animate(g, o); break;
        }
    }
}