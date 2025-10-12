// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    CRAB_IDLE,
    CRAB_HURT,
    CRAB_DIE,
    CRAB_AIR,
    CRAB_WALK,
    CRAB_ATTACK,
    CRAB_ALERT,
    CRAB_SHIELD,
};

#define CRAB_L_TRIGGER_SLASH 48
#define CRAB_TICKS_SHIELD    30

void   crab_on_update(g_s *g, obj_s *o);
void   crab_on_animate(g_s *g, obj_s *o);
bool32 crab_may_slash(g_s *g, obj_s *o);

void crab_load(g_s *g, map_obj_s *mo)
{
    obj_s *o     = obj_create(g);
    o->editorUID = mo->UID;
    o->ID        = OBJID_CRAB;
    o->w         = 12;
    o->h         = 24;
    obj_place_to_map_obj(o, mo, 0, 1);
    o->flags              = OBJ_FLAG_ACTOR | OBJ_FLAG_HURT_ON_TOUCH;
    o->on_animate         = crab_on_animate;
    o->on_update          = crab_on_update;
    o->on_hitbox          = crab_on_hit;
    o->hitbox_flags_group = HITBOX_FLAG_GROUP_ENEMY;
    o->facing             = map_obj_bool(mo, "face_left") ? -1 : +1;
    o->n_sprites          = 1;
    o->state              = CRAB_IDLE;
    o->health_max         = 3;
    o->health             = o->health_max;
    o->moverflags         = OBJ_MOVER_TERRAIN_COLLISIONS |
                    OBJ_MOVER_ONE_WAY_PLAT;
}

void crab_on_update(g_s *g, obj_s *o)
{
    i32 bumpflags = o->bumpflags;
    if (o->bumpflags & OBJ_BUMP_X) {
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y = 0;
    }
    o->bumpflags = 0;
    o->v_q12.y += Q_VOBJ(0.3);

    if (o->state == CRAB_HURT) {
        o->timer++;
        if (ENEMY_HIT_FREEZE_TICKS <= o->timer) {
            o->state = CRAB_IDLE;
            o->timer = 0;
        }
    }

    if (!obj_grounded(g, o) && o->state != CRAB_AIR) {
        o->state = CRAB_AIR;
        o->timer = 0;
    }
    if (obj_grounded(g, o) && o->state == CRAB_AIR) {
        o->state = CRAB_IDLE;
        o->timer = 0;
    }

    switch (o->state) {
    case CRAB_IDLE: {
        o->timer++;
        o->v_q12.x = 0;
        if (crab_may_slash(g, o)) {
        } else {
            if (50 <= o->timer) {
                o->state     = CRAB_WALK;
                o->animation = 0;
                o->timer     = 0;
                o->facing    = rngr_i32(0, 1) * 2 - 1;
            }
        }
        break;
    }
    case CRAB_SHIELD: {
        if (bumpflags & OBJ_BUMP_X) {
            o->v_q12.x = 0;
        }
        o->timer++;
        obj_vx_q8_mul(o, 220);
        if (CRAB_TICKS_SHIELD <= o->timer) {
            o->state   = CRAB_IDLE;
            o->timer   = 0;
            o->v_q12.x = 0;
        }
        break;
    }
    case CRAB_AIR: {
        o->timer++;
        break;
    }
    case CRAB_WALK: {
        o->v_q12.x = 0;
        if (crab_may_slash(g, o)) {
            break;
        }

        o->timer++;
        for (i32 n = 0; n < 1; n++) {
            o->animation++;
            if (!obj_try_move_grounded_sideways_without_falling(g, o, o->facing, 1)) {
                o->facing = -o->facing;
            }
        }

        if (150 <= o->timer) {
            o->state = CRAB_IDLE;
            o->timer = 0;
        }
        break;
    }
    case CRAB_ATTACK: {
        o->timer++;
        i32 fc = ani_frame(ANIID_CRAB_ATTACK, o->timer);
        if (fc < 0) {
            o->state = CRAB_IDLE;
            o->timer = 0;
        } else if (fc == 5 && ani_frame(ANIID_CRAB_ATTACK, o->timer - 1) == fc - 1) {
#if 0
            hitbox_s  hbb = hitbox_gen(g);
            hitbox_s *hb  = &hbb;
            i32       rw  = 48;
            i32       rh  = 24;
            hitbox_set_rec(hb, o->pos.x + (o->w >> 1) - (o->facing < 0 ? rw : 0),
                           o->pos.y, rw, rh);
            hb->ID                       = HITBOXID_CRAB_SLASH;
            hb->dx                       = o->facing;
            g->hitboxes[g->n_hitboxes++] = hbb;
#endif
        }

        break;
    }
    }

    obj_move_by_v_q12(g, o);
}

void crab_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    spr->flip         = 0 < o->facing ? SPR_FLIP_X : 0;

    i32 w       = 96;
    i32 h       = 48;
    i32 fx      = 0;
    i32 fy      = 0;
    spr->offs.x = (o->w - w) / 2;
    spr->offs.y = (o->h - h);

    switch (o->state) {
    case CRAB_IDLE: {
        fy = 0;
        fx = ((o->timer / 6) % 6);
        break;
    }
    case CRAB_SHIELD: {
        fx = (o->timer < 4 ? 6 : 7);
        fy = 1;
        break;
    }
    case CRAB_DIE:
    case CRAB_HURT: {
        fy = 0;
        fx = (o->timer < ENEMY_HIT_FLASH_TICKS ? 6 : 7);
        spr->offs.x += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        spr->offs.y += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        break;
    }
    case CRAB_AIR: {
        fy = 4;
        fx = frame_from_ticks_pingpong(o->timer >> 2, 3);
        break;
    }
    case CRAB_WALK: {
        fy = 1;
        fx = (o->animation >> 2) % 6;
        break;
    }
    case CRAB_ATTACK: {
        i32 f = ani_frame_loop(ANIID_CRAB_ATTACK, o->timer);
        fy    = 2 + (f >> 3);
        fx    = (f & 7);
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_CRAB, fx * w, fy * h, w, h);
}

void crab_do_hurt(g_s *g, obj_s *o, i32 dmg)
{
    o->health = max_i32(0, (i32)o->health - dmg);
    if (o->health) {
        o->state = CRAB_HURT;
    } else {
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        o->state     = CRAB_DIE;
        o->on_update = enemy_on_update_die;
        g->enemies_killed++;
        g->enemy_killed[ENEMYID_CRAB]++;
    }
    o->v_q12.x = 0;
    o->v_q12.y = 0;
    o->timer   = 0;
}

void crab_do_shield(g_s *g, obj_s *o, i32 dx)
{
    o->state   = CRAB_SHIELD;
    o->v_q12.x = Q_VOBJ(3.0) * dx;
    o->timer   = 0;
}

void crab_on_hit(g_s *g, obj_s *o, hitbox_res_s res)
{
}

void crab_on_hitbox(g_s *g, obj_s *o, hitbox_s *hb)
{
#if 0
    switch (o->state) {
    case CRAB_DIE: break;
    case CRAB_SHIELD: {
        if (hb->dx == o->facing) {
            crab_do_hurt(g, o, 1);
        } else {
            crab_do_shield(g, o, hb->dx);
        }
        break;
    }
    default: {
        if (o->state == CRAB_ATTACK && 7 <= ani_frame(ANIID_CRAB_ATTACK, o->timer)) {
            crab_do_shield(g, o, hb->dx);
        } else {
            crab_do_hurt(g, o, 1);
        }

        if (hb->dx) {
            o->facing = -hb->dx;
        }
        break;
    }
    }
#endif
}

bool32 crab_may_slash(g_s *g, obj_s *o)
{
    obj_s *o_owl = owl_if_present_and_alive(g);
    if (!o_owl) return 0;

    v2_i32 pc   = obj_pos_center(o);
    v2_i32 owlc = obj_pos_center(o_owl);
    owl_s *h    = (owl_s *)o_owl->heap;

    rec_i32 rtrigger_l = {o->pos.x + o->w / 2 - CRAB_L_TRIGGER_SLASH, o->pos.y, CRAB_L_TRIGGER_SLASH, o->h};
    rec_i32 rtrigger_r = {o->pos.x + o->w / 2, o->pos.y, CRAB_L_TRIGGER_SLASH, o->h};

    bool32 do_attack = 0;
    if (overlap_rec(obj_aabb(o_owl), rtrigger_l)) {
        o->facing = -1;
        do_attack = 1;
    }
    if (overlap_rec(obj_aabb(o_owl), rtrigger_r)) {
        o->facing = +1;
        do_attack = 1;
    }
    if (do_attack) {
        o->state = CRAB_ATTACK;
        o->timer = 0;
        return 1;
    }
    return 0;
}