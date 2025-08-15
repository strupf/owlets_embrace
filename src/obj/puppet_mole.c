// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *puppet_mole_create(g_s *g, v2_i32 pfeet)
{
    obj_s *o           = puppet_create(g, OBJID_PUPPET_MOLE);
    o->pos.x           = pfeet.x;
    o->pos.y           = pfeet.y;
    o->facing          = 1;
    o->render_priority = RENDER_PRIO_OWL - 1;
    return o;
}

void puppet_mole_on_animate(obj_s *o, i32 animID, i32 anim_t)
{
    puppet_s *ocs     = (puppet_s *)o->mem;
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];

    if (o->facing == +1) {
        spr->flip = SPR_FLIP_X;
    } else {
        spr->flip = 0;
    }

    i32 fw = 64;
    i32 fh = 64;
    i32 fx = 0;
    i32 fy = 0;

    spr->offs.x = -64 / 2;
    spr->offs.y = -64 + 1;

    switch (animID) {

    case PUPPET_MOLE_ANIMID_IDLE: {
        fy = 0;
        fx = (anim_t >> 3) % 6;
        break;
    }
    case PUPPET_MOLE_ANIMID_WALK: {
        fy = 1;
        fx = frame_from_ticks_pingpong(anim_t >> 2, 6);
        break;
    }
    case PUPPET_MOLE_ANIMID_DIG_OUT: {
        i32 f = ani_frame(ANIID_MOLE_DIG_OUT, anim_t);
        fx    = f & 7;
        fy    = 2 + (f >> 3);
        break;
    }
    default:
    case PUPPET_MOLE_ANIMID_DIG_IN: {
        i32 f = ani_frame(ANIID_MOLE_DIG_IN, anim_t);
        fx    = f & 7;
        fy    = 5 + (f >> 3);
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_MOLE,
                             fx * fw, fy * fh, fw, fh);
}