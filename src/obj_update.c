// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void objs_update(g_s *g)
{
    for (obj_each(g, o)) {
        o->v_prev_q8 = o->v_q8;

        switch (o->ID) {
        default: break;
        // case OBJID_HERO: hero_on_update(g, o); break;
        //  case OBJID_HOOK: hook_update(g, o); break;
        case OBJID_FLYBLOB: flyblob_on_update(g, o); break;
        case OBJID_PROJECTILE: projectile_on_update(g, o); break;
        case OBJID_BUDPLANT: budplant_on_update(g, o); break;
        case OBJID_SPRITEDECAL: spritedecal_on_update(g, o); break;
        case OBJID_PUSHBLOCK: pushblock_on_update(g, o); break;
        case OBJID_NPC: npc_on_update(g, o); break;
        case OBJID_HERO_POWERUP: hero_powerup_obj_on_update(g, o); break;
        case OBJID_CRUMBLEBLOCK: crumbleblock_on_update(g, o); break;
        case OBJID_CRAWLER: crawler_on_update(g, o); break;
        case OBJID_FROG: frog_on_update(g, o); break;
        case OBJID_FALLINGBLOCK: fallingblock_on_update(g, o); break;
        case OBJID_ROTOR: rotor_on_update(g, o); break;
        case OBJID_DUMMYSOLID: dummysolid_on_update(g, o); break;
        case OBJID_CHEST: chest_on_update(g, o); break;
        case OBJID_WATERLEAF: waterleaf_on_update(g, o); break;
        case OBJID_WINDAREA: windarea_on_update(g, o); break;
        case OBJID_COIN: coin_on_update(g, o); break;
        case OBJID_DOOR: door_on_update(g, o); break;
        case OBJID_TRIGGERAREA: triggerarea_on_update(g, o); break;
        case OBJID_WALKER: walker_on_update(g, o); break;
        case OBJID_SPIDERBOSS: spiderboss_on_update(g, o); break;
        case OBJID_TRAMPOLINE: trampoline_on_update(g, o); break;
        case OBJID_CLOCKPULSE: clockpulse_on_update(g, o); break;
        case OBJID_SAVEPOINT: savepoint_on_update(g, o); break;
        case OBJID_BOULDER: boulder_on_update(g, o); break;
        case OBJID_HOOKLEVER: hooklever_on_update(g, o); break;
        case OBJID_FALLINGSTONE: fallingstone_on_update(g, o); break;
        case OBJID_STALACTITE: stalactite_on_update(g, o); break;
        case OBJID_BITER: biter_on_update(g, o); break;
        case OBJID_WATERCOL: watercol_on_update(g, o); break;
        }
    }
}
