// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    NPC_MOVEMENT_STANDING,
    NPC_MOVEMENT_WANDER,
};

enum {
    NPC_GROUNDED,
    NPC_AIR,
};

typedef struct {
    i32 movement;
    i32 movedir;
} npc_s;

static i32 npc_get_state(g_s *g, obj_s *o)
{
    bool32 grounded = obj_grounded(g, o);
    if (!grounded) return NPC_AIR;
    return NPC_GROUNDED;
}

void npc_on_update(g_s *g, obj_s *o)
{
    bool32 bumpedx = o->bumpflags & OBJ_BUMP_X;
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q8.y = 0;
    }
    if (bumpedx) {
        o->v_q8.x = 0;
    }
    o->bumpflags = 0;

    o->flags &= ~OBJ_FLAG_INTERACTABLE;
    i32 state = npc_get_state(g, o);
    if (state == NPC_AIR) {
        return;
    }

    o->flags |= OBJ_FLAG_INTERACTABLE;

    npc_s *npc = (npc_s *)o->mem;
    switch (npc->movement) {
    case NPC_MOVEMENT_WANDER: {
        // move left, right or stand still based on a random timer
        // ramp up velocity

        if (--o->timer <= 0) {
            o->drag_q8.x = 180; // slowly stop movement
            if (abs_i32(o->v_q8.x) < 10) {
                o->v_q8.x = 0;
            }
            if (o->timer < -100) {
                o->timer     = rngr_i32(80, 150);
                o->v_q8.x    = rngr_i32(-1, +1);
                o->drag_q8.x = 256;
                if (o->v_q8.x != 0) {
                    o->facing = sgn_i32(o->v_q8.x);
                }
            }
        } else {
            o->v_q8.x <<= 1;
        }

        if (obj_would_fall_down_next(g, o, sgn_i32(o->v_q8.x))) {
            o->v_q8.x = 0;
        }
    } break;
    }
}

void npc_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr   = &o->sprites[0];
    i32           frame = 0;
    spr->flip           = SPR_FLIP_X;

    switch (npc_get_state(g, o)) {
    case NPC_GROUNDED: {
        if (o->v_q8.x == 0) {
            o->animation++;

            frame = 0 + ((o->animation / 6) % 6);
        } else {
            o->animation += abs_i32(o->v_q8.x);
            frame = 0 + ((o->animation >> 10) % 6);
        }
        break;
    }
    case NPC_AIR: {
        o->animation++;
        frame = 8 + ((o->animation >> 4) & 1);
        break;
    }
    }

    spr->trec.r.x = frame * 64;
    // spr->flip     = o->facing == 1 ? 0 : SPR_FLIP_X;
}

void npc_on_interact(g_s *g, obj_s *o)
{
    npc_s *npc   = (npc_s *)o->mem;
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        o->facing = ohero->pos.x < o->pos.x ? -1 : +1;
    }
    o->v_q8.x = 0;
    textbox_load_dialog(g, o->filename);
}

void npc_load(g_s *g, map_obj_s *mo)
{
    obj_s        *o   = obj_create(g);
    npc_s        *npc = (npc_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    o->ID    = OBJ_ID_NPC;
    o->flags = OBJ_FLAG_INTERACTABLE |
               OBJ_FLAG_MOVER;
    o->moverflags = OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_SLIDE_Y_NEG;

    o->on_update       = npc_on_update;
    o->on_animate      = npc_on_animate;
    o->on_interact     = npc_on_interact;
    o->render_priority = 1;
    o->w               = 16;
    o->h               = 20;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y + mo->h - o->h;
    o->facing          = 1;
    o->n_sprites       = 1;
    spr->trec          = asset_texrec(TEXID_NPC, 0, 0, 64, 48);
    spr->offs.x        = (o->w - spr->trec.r.w) / 2;
    spr->offs.y        = o->h - spr->trec.r.h;

    map_obj_strs(mo, "Dialogfile", o->filename);
    npc->movement          = map_obj_i32(mo, "Movement");
    o->sprites[0].trec.r.y = map_obj_i32(mo, "Model") * 48;
}
