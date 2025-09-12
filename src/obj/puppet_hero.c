// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *puppet_owl_put(g_s *g, obj_s *ohero)
{
    obj_s *o           = puppet_create(g, OBJID_PUPPET_HERO);
    o->pos.x           = ohero->pos.x + ohero->w / 2;
    o->pos.y           = ohero->pos.y + ohero->h;
    o->facing          = ohero->facing;
    o->render_priority = RENDER_PRIO_OWL;
    ohero->flags |= OBJ_FLAG_DONT_SHOW_UPDATE;
    return o;
}

void puppet_owl_replace_and_del(g_s *g, obj_s *ohero, obj_s *o)
{
    ohero->pos.x  = o->pos.x - ohero->w / 2;
    ohero->pos.y  = o->pos.y - ohero->h;
    ohero->facing = o->facing;
    ohero->flags &= ~OBJ_FLAG_DONT_SHOW_UPDATE;
    obj_delete(g, o);
}

void puppet_hero_on_animate(obj_s *o, i32 animID, i32 anim_t)
{
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];

    i32 fw = 64;
    i32 fh = 64;
    i32 fx = 0;
    i32 fy = 0;

    spr->offs.y = -64;
    spr->offs.x = -32;
    if (o->facing == -1) {
        spr->flip = SPR_FLIP_X;
    } else {
        spr->flip = 0;
    }

    switch (animID) {
    default: break;
    case PUPPET_OWL_ANIMID_IDLE: {
        i32 idlea = anim_t / 15;
        fy        = 2;
        fx        = idlea & 3;
        if (((idlea >> 2) % 3) == 1 && fx <= 1) {
            fx += 4; // blink
        }
        break;
    }
    case PUPPET_OWL_ANIMID_UPGR_INTENSE: {
        spr->offs.y += 4;
        fy = 17;
        fx = 2 + ((anim_t >> 2) & 1);
        break;
    }
    case PUPPET_OWL_ANIMID_UPGR_RISE: {
        spr->offs.y += 4;
        fy = 17;
        if (anim_t < 20) {
            fx = 1;
        } else {
            fx = 2 + ((anim_t >> 3) & 1);
        }
        break;
    }
    case PUPPET_OWL_ANIMID_UPGR_CALM: {
        spr->offs.y += 4;
        fy = 17;
        fx = 2 + ((anim_t >> 3) & 1);
        break;
    }
    case PUPPET_OWL_ANIMID_SHOOK: {
        fy = 7;
        fx = 7 + ((anim_t >> 3) & 1);
        break;
    }
    case PUPPET_OWL_ANIMID_AVENGE: {
        fy = 14;
        fx = 9 + min_i32(anim_t >> 3, 2);
        break;
    }
    case PUPPET_OWL_ANIMID_QUICKDUCK: {
        fy = 11;
        fx = 2;
        break;
    }
    case PUPPET_OWL_ANIMID_HOLD_ARM: {
        fy = 14;
        fx = 6 + min_i32((anim_t >> 2), 5);
        break;
    }
    case PUPPET_OWL_ANIMID_PRESENT_ABOVE:
    case PUPPET_OWL_ANIMID_PRESENT_ABOVE_COMP: {
        fx = 13 + min_i32(anim_t >> 1, 2);
        fy = animID == PUPPET_OWL_ANIMID_PRESENT_ABOVE_COMP ? 25 : 0;
        break;
    }
    case PUPPET_OWL_ANIMID_PRESENT_ABOVE_TO_IDLE:
    case PUPPET_OWL_ANIMID_PRESENT_ABOVE_COMP_TO_IDLE: {
        i32 t = anim_t >> 1;
        fx    = 15 - min_i32(t, 2);
        fy    = animID == PUPPET_OWL_ANIMID_PRESENT_ABOVE_COMP_TO_IDLE ? 25 : 0;
        if (3 <= t) {
            puppet_set_anim(o, PUPPET_OWL_ANIMID_IDLE, 0);
        }
        break;
    }
    case PUPPET_OWL_ANIMID_OFF_BALANCE: {
        i32 t = frame_from_ticks_pingpong(anim_t >> 1, 6);
        fx    = 8 + t;
        fy    = 4;
        break;
    }
    case PUPPET_OWL_ANIMID_OFF_BALANCE_COMP: {
        i32 t = frame_from_ticks_pingpong(anim_t >> 1, 6);
        fx    = 8 + t;
        fy    = 28;
        break;
    }
    case PUPPET_OWL_ANIMID_FALL_ASLEEP: {
        fy = 18;
        fx = ani_frame_loop(ANIID_FALLASLEEP, anim_t);
        break;
    }
    case PUPPET_OWL_ANIMID_SLEEP: {
        fy = 18;
        fx = 11 + ((anim_t >> 5) & 1);
        break;
    }
    case PUPPET_OWL_ANIMID_SLEEP_WAKEUP: {
        i32 f = 0;
        if (anim_t <= ani_len(ANIID_WAKEUP)) {
            f = ani_frame_loop(ANIID_WAKEUP, anim_t);
        }
        fy = 18 + (f >> 4);
        fx = (f & 15);
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_HERO, fx * fw, fy * fh, fw, fh);
}