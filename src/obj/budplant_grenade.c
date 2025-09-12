// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    BPGRENADE_ST_FLYING,
    BPGRENADE_ST_DELAY_DMG,
};

void bpgrenade_on_update(g_s *g, obj_s *o);
void bpgrenade_on_animate(g_s *g, obj_s *o);
void bpgrenade_explode(g_s *g, obj_s *o);

void bpgrenade_create(g_s *g, v2_i32 pos, v2_i32 vel)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJID_BUDPLANT_GRENADE;
    o->on_update  = bpgrenade_on_update;
    o->on_animate = bpgrenade_on_animate;
    o->v_q12      = vel;
    o->w          = 8;
    o->h          = 8;
    o->pos.x      = pos.x - o->w / 2;
    o->pos.y      = pos.y - o->h / 2;
    o->n_sprites  = 2;
    o->flags =
        OBJ_FLAG_ACTOR |
        OBJ_FLAG_KILL_OFFSCREEN;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS |
                    OBJ_MOVER_ONE_WAY_PLAT;
}

void bpgrenade_on_update(g_s *g, obj_s *o)
{
    o->timer++;

    switch (o->state) {
    case BPGRENADE_ST_FLYING: {
        o->animation++;
        if (100 <= o->timer) {
            bpgrenade_explode(g, o);
        } else {
            if (o->bumpflags & OBJ_BUMP_X) {
                o->v_q12.x = -(o->v_q12.x * 140) / 256;
            }
            if (o->bumpflags & OBJ_BUMP_Y) {
                o->v_q12.y = -(o->v_q12.y * 140) / 256;
            }
            o->bumpflags = 0;

            o->v_q12.y += Q_VOBJ(0.2);
            obj_move_by_v_q12(g, o);
        }
        break;
    }
    case BPGRENADE_ST_DELAY_DMG: {
        if (6 <= o->timer) {
            v2_i32 pos = obj_pos_center(o);
            obj_delete(g, o);
        }
        break;
    }
    }
}

void bpgrenade_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr2 = &o->sprites[1];
    obj_sprite_s *spr  = &o->sprites[0];

    i32 imgx  = 8 + ((o->animation >> 2) & 7);
    i32 imgy  = 4;
    i32 imgx2 = 8 + ((o->animation >> 2) & 7);
    i32 imgy2 = 5;

    spr->offs.x  = -12;
    spr->offs.y  = -12;
    spr->trec    = asset_texrec(TEXID_BUDPLANT, imgx << 5, imgy << 5, 32, 32);
    spr2->offs.x = -12;
    spr2->offs.y = -12;
    spr2->trec   = asset_texrec(TEXID_BUDPLANT, imgx2 << 5, imgy2 << 5, 32, 32);
}

void bpgrenade_explode(g_s *g, obj_s *o)
{
    if (o->state == BPGRENADE_ST_FLYING) {
        o->state      = BPGRENADE_ST_DELAY_DMG;
        o->n_sprites  = 0;
        o->timer      = 0;
        o->flags      = 0;
        o->on_animate = 0;
        v2_i32 pos    = obj_pos_center(o);
        objanim_create(g, pos, OBJANIMID_EXPLODE_GRENADE);
    }
}