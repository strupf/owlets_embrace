// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

obj_s *puppet_hero_put(g_s *g, obj_s *ohero)
{
    obj_s *o  = puppet_create(g, OBJID_PUPPET_HERO);
    o->pos.x  = ohero->pos.x + ohero->w / 2;
    o->pos.y  = ohero->pos.y + ohero->h;
    o->facing = ohero->facing;
    ohero->flags |= OBJ_FLAG_DONT_SHOW_UPDATE;
    return o;
}

void puppet_hero_replace_and_del(g_s *g, obj_s *ohero, obj_s *o)
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
    case PUPPET_HERO_ANIMID_IDLE: {
        i32 idlea = anim_t / 15;
        fy        = 2;
        fx        = idlea & 3;
        if (((idlea >> 2) % 3) == 1 && fx <= 1) {
            fx += 4; // blink
        }
        break;
    }
    case PUPPET_HERO_ANIMID_UPGR_INTENSE: {
        spr->offs.y += 4;
        fy = 17;
        fx = 2 + ((anim_t >> 2) & 1);
        break;
    }
    case PUPPET_HERO_ANIMID_UPGR_RISE: {
        spr->offs.y += 4;
        fy = 17;
        if (anim_t < 20) {
            fx = 1;
        } else {
            fx = 2 + ((anim_t >> 3) & 1);
        }
        break;
    }
    case PUPPET_HERO_ANIMID_UPGR_CALM: {
        spr->offs.y += 4;
        fy = 17;
        fx = 2 + ((anim_t >> 3) & 1);
        break;
    }
    case PUPPET_HERO_ANIMID_SHOOK: {
        fy = 7;
        fx = 7 + ((anim_t >> 3) & 1);
        break;
    }
    case PUPPET_HERO_ANIMID_AVENGE: {
        fy = 16;
        fx = min_i32(anim_t >> 3, 2);
        break;
    }
    case PUPPET_HERO_ANIMID_QUICKDUCK: {
        fy = 14;
        fx = 2;
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_HERO, fx * fw, fy * fh, fw, fh);
}