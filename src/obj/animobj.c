// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void animobj_progress(g_s *g, obj_s *o);

obj_s *animobj_create(g_s *g, v2_i32 p, i32 animobjID)
{
    obj_s *o           = obj_create(g);
    o->pos.x           = p.x;
    o->pos.y           = p.y;
    o->subID           = animobjID;
    o->n_sprites       = 1;
    o->on_update       = animobj_progress;
    o->render_priority = RENDER_PRIO_INFRONT_FLUID_AREA - 1;
    return o;
}

void animobj_progress(g_s *g, obj_s *o)
{
    o->timer++;
    obj_sprite_s *spr = &o->sprites[0];
    spr->flip         = 0;

    i32 fx = 0;
    i32 fy = 0;
    i32 w  = 0;
    i32 h  = 0;

    switch (o->subID) {
    case ANIMOBJ_EXPLOSION_1: {
        w  = 64;
        h  = 64;
        fx = 0;
        fy = ani_frame(ANIID_EXPLOSION_1, o->timer);
        if (fy < 0) {
            obj_delete(g, o);
        }
        break;
    }
    case ANIMOBJ_EXPLOSION_2: {
        w  = 64;
        h  = 64;
        fx = 1;
        fy = ani_frame(ANIID_EXPLOSION_2, o->timer);
        if (fy < 0) {
            obj_delete(g, o);
        }
        break;
    }
    case ANIMOBJ_EXPLOSION_3: {
        w  = 64;
        h  = 64;
        fx = 2;
        fy = ani_frame(ANIID_EXPLOSION_3, o->timer);
        if (fy < 0) {
            obj_delete(g, o);
        }
        break;
    }
    case ANIMOBJ_EXPLOSION_4: {
        w  = 128;
        h  = 128;
        fx = 2;
        fy = ani_frame(ANIID_EXPLOSION_4, o->timer);
        if (fy < 0) {
            obj_delete(g, o);
        } else {
            fy += 5;
        }
        break;
    }
    case ANIMOBJ_EXPLOSION_5: {
        w  = 64;
        h  = 64;
        fx = 2;
        fy = ani_frame(ANIID_EXPLOSION_5, o->timer);
        if (fy < 0) {
            obj_delete(g, o);
        }
        break;
    }
    case ANIMOBJ_ENEMY_SPAWN: {
        u32 r = o->pos.x + o->pos.y;
        switch (rngs_i32(&r) & 3) {
        default: break;
        case 1: spr->flip = SPR_FLIP_X; break;
        case 2: spr->flip = SPR_FLIP_Y; break;
        case 3: spr->flip = SPR_FLIP_XY; break;
        }

        w  = 64;
        h  = 64;
        fx = 3;
        fy = ani_frame(ANIID_ENEMY_SPAWN_1, o->timer);
        if (fy < 0) {
            fy       = 11;
            o->subID = ANIMOBJ_EXPLOSION_2;
            o->timer = 0;
        }
        break;
    }
    case ANIMOBJ_STOMP: {
        w  = 128;
        h  = 64;
        fx = 2;
        fy = ani_frame(ANIID_STOMP_PARTICLE, o->timer);
        if (fy < 0) {
            obj_delete(g, o);
        }
        break;
    }
    }

    spr->offs.x = -(w >> 1);
    spr->offs.y = -(h >> 1);
    spr->trec   = asset_texrec(TEXID_EXPLOSIONS, fx * w, fy * h, w, h);
}