// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// TODO: crunchy satisfying sound effects

#include "game.h"

#define GEMPILE_N_HITS          5
#define GEMPILE_GEMS_ON_HIT     2
#define GEMPILE_GEMS_ON_DESTROY 6

void gempile_on_animate(g_s *g, obj_s *o);

void gempile_load(g_s *g, map_obj_s *mo)
{
    i32 saveID = map_obj_i32(mo, "saveID");
    if (saveID_has(g, saveID)) return;

    obj_s *o     = obj_create(g);
    o->editorUID = mo->UID;
    o->ID        = OBJID_GEMPILE;
    o->w         = 40;
    o->h         = 24;
    obj_place_to_map_obj(o, mo, 0, +1);
    o->n_sprites  = 1;
    o->on_animate = gempile_on_animate;
    o->health_max = GEMPILE_N_HITS;
    o->health     = o->health_max;
    o->flags      = OBJ_FLAG_OWL_JUMPSTOMPABLE;
    o->state      = saveID;
}

void gempile_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr   = &o->sprites[0];
    i32           n_off = (GEMPILE_N_HITS - o->health) * 4; // make it smaller with each hit

    spr->trec   = asset_texrec(TEXID_GEMS, 0, 144, 64, 48);
    spr->offs.x = -((spr->trec.w - o->w) >> 1);
    spr->offs.y = -(spr->trec.h - o->h) + n_off;
    spr->trec.h -= n_off;

    if (o->animation) {
        o->animation--;
        spr->trec.y += 48;
    }
}

void gempile_on_hit(g_s *g, obj_s *o)
{
    saveID_put(g, o->state);
    o->health--;

    v2_i32 pc     = obj_pos_center(o);
    i32    n_gems = o->health == 0 ? GEMPILE_GEMS_ON_DESTROY : GEMPILE_GEMS_ON_HIT;
    o->animation  = 12;

    for (i32 n = 0; n < n_gems; n++) {
        obj_s *i = coin_create(g);
        if (!i) {
            coins_change(g, n_gems - n);
            break;
        }
        i->pos.x   = pc.x - (i->w >> 1);
        i->pos.y   = pc.y - (i->h >> 1);
        i->v_q12.y = -rngr_i32(Q_VOBJ(3.5), Q_VOBJ(4.5));
        i->v_q12.x = rngr_sym_i32(Q_VOBJ(1.5));
    }
    if (o->health == 0) {
        obj_delete(g, o);
    }
}