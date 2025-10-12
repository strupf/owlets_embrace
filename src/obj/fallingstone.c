// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void   fallingstonespawn_on_update(g_s *g, obj_s *o);
obj_s *fallingstone_spawn(g_s *g);
void   fallingstone_on_update(g_s *g, obj_s *o);
void   fallingstone_on_animate(g_s *g, obj_s *o);
void   fallingstone_burst(g_s *g, obj_s *o);

void fallingstonespawn_load(g_s *g, map_obj_s *mo)
{
    obj_s *o          = obj_create(g);
    o->editorUID      = mo->UID;
    o->ID             = OBJID_FALLINGSTONE_SPAWN;
    o->pos.x          = mo->x;
    o->pos.y          = mo->y;
    o->w              = 16;
    o->h              = 16;
    o->on_update      = fallingstonespawn_on_update;
    o->subtimer       = map_obj_i32(mo, "Spawnticks");
    o->timer          = map_obj_i32(mo, "Spawnbegin") % o->subtimer;
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_BOULDER,
                                     64, 64,
                                     32, 26);
    spr->offs.x       = -8;
    spr->offs.y       = -12;
}

void fallingstonespawn_on_update(g_s *g, obj_s *o)
{
    o->timer++;
    if (o->subtimer <= o->timer) {
        o->timer = 0;

        obj_s *i = fallingstone_spawn(g);
        i->pos.x = o->pos.x + (o->w - i->w) / 2;
        i->pos.y = o->pos.y;
    }
}

enum {
    FALLINGSTONE_ST_SPAWNING,
    FALLINGSTONE_ST_FALLING
};

#define FALLINGSTONE_TICKS_SPAWN 20
#define FALLINGSTONE_VY          (256 * 4)

obj_s *fallingstone_spawn(g_s *g)
{
    obj_s *o             = obj_create(g);
    o->ID                = OBJID_FALLINGSTONE;
    o->w                 = 12;
    o->h                 = 12;
    o->on_update         = fallingstone_on_update;
    o->on_animate        = fallingstone_on_animate;
    o->on_touchhurt_hero = fallingstone_burst;
    o->render_priority   = RENDER_PRIO_DEFAULT_OBJ - 1;
    o->substate          = rngr_i32(0, 1) ? +1 : -1;
    o->n_sprites         = 1;
    return o;
}

void fallingstone_on_update(g_s *g, obj_s *o)
{
    o->timer++;
    switch (o->state) {
    case FALLINGSTONE_ST_SPAWNING: {
        if (FALLINGSTONE_TICKS_SPAWN <= o->timer) {
            o->timer      = 0;
            o->state      = FALLINGSTONE_ST_FALLING;
            o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS |
                            OBJ_MOVER_ONE_WAY_PLAT;
            o->flags = OBJ_FLAG_ACTOR |
                       OBJ_FLAG_HURT_ON_TOUCH |
                       OBJ_FLAG_KILL_OFFSCREEN;
        }
        break;
    }
    case FALLINGSTONE_ST_FALLING: {
        if (o->bumpflags) {
            v2_i32 p = obj_pos_center(o);
            sfx_cuef_pos(SFXID_EXPLO1, 1.0f, rngr_f32(0.9f, 1.1f), 0, p.x, p.y, 500);
            fallingstone_burst(g, o);
        } else {
            o->v_q12.y += Q_VOBJ(0.25);
            if (Q_VOBJ(4.0) <= o->v_q12.y) {
                o->v_q12.y = Q_VOBJ(4.0);
                o->animation += o->substate;
            } else if (o->timer & 1) {
                o->animation += o->substate;
            }
            obj_move_by_v_q12(g, o);
        }
        break;
    }
    }
}

void fallingstone_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_BOULDER,
                                     0,
                                     ((o->animation >> 2) & 3) * 32 + 4,
                                     32, 24);
    spr->offs.x       = (o->w - spr->trec.w) / 2;
    spr->offs.y       = (o->h - spr->trec.h) / 2;

    if (o->state == FALLINGSTONE_ST_SPAWNING) {
        spr->offs.y -= ease_out_quad(20, 0, o->timer, FALLINGSTONE_TICKS_SPAWN);
    }
}

void fallingstone_burst(g_s *g, obj_s *o)
{
    obj_delete(g, o);
}