// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *puppet_companion_put(g_s *g, obj_s *ocomp)
{
    obj_s *o  = puppet_create(g, OBJID_PUPPET_COMPANION);
    o->pos.x  = ocomp->pos.x + ocomp->w / 2;
    o->pos.y  = ocomp->pos.y + ocomp->h / 2;
    o->facing = ocomp->facing;
    ocomp->flags |= OBJ_FLAG_DONT_SHOW_UPDATE;
    return o;
}

void puppet_companion_replace_and_del(g_s *g, obj_s *ocomp, obj_s *o)
{
    ocomp->pos.x  = o->pos.x - ocomp->w / 2;
    ocomp->pos.y  = o->pos.y - ocomp->h / 2;
    ocomp->facing = o->facing;
    ocomp->flags &= ~OBJ_FLAG_DONT_SHOW_UPDATE;
    obj_delete(g, o);
}

void puppet_companion_on_animate(obj_s *o, i32 animID, i32 anim_t)
{
    puppet_s *ocs     = (puppet_s *)o->mem;
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];

    if (o->facing == +1) {
        spr->flip = SPR_FLIP_X;
    } else {
        spr->flip = 0;
    }

    i32 fw = 96;
    i32 fh = 64;
    i32 fx = 0;
    i32 fy = 0;

    spr->offs.x = -96 / 2;
    spr->offs.y = -64 / 2 - 2;

    switch (animID) {
    default: break;
    case PUPPET_COMPANION_ANIMID_FLY: {
        fy = 0;
        fx = ani_frame(ANIID_COMPANION_FLY, anim_t);
        break;
    }
    case PUPPET_COMPANION_ANIMID_SIT: {
        fy = 3;
        fx = (anim_t >> 4) % 3;
        break;
    }
    case PUPPET_COMPANION_ANIMID_TUMBLE: {
        fy = 8;
        fx = (anim_t >> 3) & 3;
        break;
    }
    case PUPPET_COMPANION_ANIMID_BUMP_ONCE: {
        fy    = 6;
        fx    = ani_frame(ANIID_COMPANION_BUMP, anim_t);
        i32 l = ani_len(ANIID_COMPANION_BUMP);
        if (l <= anim_t) {
            ocs->anim_t = 0;
            ocs->animID = PUPPET_COMPANION_ANIMID_FLY;
        }
        break;
    }
    case PUPPET_COMPANION_ANIMID_NOD_ONCE:
    case PUPPET_COMPANION_ANIMID_NOD: {
        fy    = 0;
        fx    = ani_frame(ANIID_COMPANION_FLY, anim_t);
        i32 l = ani_len(ANIID_COMPANION_FLY);
        i32 k = anim_t % (4 * l);
        if (k < 2 * l) { // confirming nod animation, synced to animation
            i32 co  = cos_q15((k << 17) / l) - 32768;
            i32 co2 = pow2_i32(co >> 1) >> 14;
            spr->offs.y += (co2 * 16) >> 16;
        }

        if (animID == PUPPET_COMPANION_ANIMID_NOD_ONCE && l <= anim_t) {
            ocs->animID = PUPPET_COMPANION_ANIMID_FLY;
            ocs->anim_t = 0;
        }
        break;
    }
    case PUPPET_COMPANION_ANIMID_HUH: {
        fy     = 7;
        fx     = ani_frame(ANIID_COMPANION_HUH, anim_t);
        i32 l  = ani_len(ANIID_COMPANION_HUH);
        i32 co = cos_q15(((anim_t % l) << 18) / l) - 32768;
        spr->offs.y += (co * 4) >> 16;
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_COMPANION,
                             fx * fw, fy * fh, fw, fh);
}