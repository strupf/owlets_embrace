// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    i32          len_tongue;
    i32          len_tongue_grabbed;
    obj_handle_s owl;
} frog_s;

enum {
    FROG_IDLE,
    FROG_HURT,
    FROG_DIE,
    FROG_WALK,
    FROG_JUMP_PREPARE,
    FROG_JUMP_LAND,
    FROG_AIR,
    FROG_TONGUE_OUT_PREPARE,
    FROG_TONGUE_OUT,
    FROG_TONGUE_PAUSE,
    FROG_TONGUE_IN,
    FROG_TONGUE_IN_END,
    FROG_TONGUE_GRABBED_PAUSE,
    FROG_TONGUE_GRABBED,
    FROG_EAT,
    FROG_SPIT_OUT,
};

#define FROG_L_TRIGGER                  90
#define FROG_L_TONGUE                   112
#define FROG_TICKS_TONGUE_PREPARE       8
#define FROG_TICKS_TONGUE_OUT           8
#define FROG_TICKS_TONGUE_IN            10
#define FROG_TICKS_TONGUE_IN_END        6
#define FROG_TICKS_TONGUE_PAUSE         20
#define FROG_TICKS_TONGUE_GRABBED_PAUSE 12
#define FROG_TICKS_TONGUE_GRABBED       10
#define FROG_TICKS_SPIT_OUT             12
#define FROG_TICKS_EAT                  80

void   frog_on_update(g_s *g, obj_s *o);
void   frog_on_animate(g_s *g, obj_s *o);
void   frog_on_draw(g_s *g, obj_s *o, v2_i32 cam);
bool32 frog_try_grab_with_tongue(g_s *g, obj_s *o, i32 l_tongue);

void frog_on_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);
    pltf_log("spawn");
    o->editorUID  = mo->UID;
    o->ID         = OBJID_FROG;
    o->on_update  = frog_on_update;
    o->on_animate = frog_on_animate;
    o->on_draw    = frog_on_draw;
    o->w          = 32;
    o->h          = 32;
    o->flags =
        OBJ_FLAG_ACTOR |
        OBJ_FLAG_HURT_ON_TOUCH;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS;
    obj_place_to_map_obj(o, mo, 0, +1);
    o->n_sprites  = 1;
    o->facing     = map_obj_bool(mo, "face_left") ? -1 : +1;
    // o->facing    = -o->facing;
    o->state      = 0;
    o->health_max = 3;
    o->health     = o->health_max;
}

void frog_on_update(g_s *g, obj_s *o)
{
    frog_s *fr = (frog_s *)o->mem;
    o->v_q12.y += Q_VOBJ(0.3);

    if (o->bumpflags & OBJ_BUMP_X) {
        o->facing  = -o->facing;
        o->v_q12.x = 0;
    }
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y = 0;
    }
    o->bumpflags = 0;
    if (o->state == FROG_HURT) {
        o->timer++;
        if (ENEMY_HIT_FREEZE_TICKS <= o->timer) {
            o->state = FROG_IDLE;
            o->timer = 0;
        }
    }

    if (!obj_grounded(g, o) && o->state != FROG_EAT) {
        o->state       = FROG_AIR;
        o->timer       = 0;
        fr->len_tongue = 0;
    }
    if (obj_grounded(g, o) && o->state == FROG_AIR) {
        o->state = FROG_IDLE;
        o->timer = 0;
    }

    switch (o->state) {
    case FROG_IDLE: {
        o->v_q12.x = 0;
        o->timer++;

        obj_s *o_owl      = owl_if_present_and_alive(g);
        bool32 owl_nearby = 0;

        if (o_owl) {
            v2_i32 pc   = obj_pos_center(o);
            v2_i32 owlc = obj_pos_center(o_owl);
            owl_s *h    = (owl_s *)o_owl->heap;

            if (!h->special_state && v2_i32_distance_appr(pc, owlc) < 256) {
                owl_nearby = 1;
                if (owlc.x + 8 < pc.x) {
                    o->facing = -1;
                }
                if (owlc.x - 8 > pc.x) {
                    o->facing = +1;
                }
            }
        }

        if (owl_nearby) {
            rec_i32 rtrigger = {o->pos.x + o->w / 2 - (o->facing < 0 ? FROG_L_TRIGGER : 0), o->pos.y, FROG_L_TRIGGER, o->h};

            if (10 <= o->timer && o_owl && overlap_rec(obj_aabb(o_owl), rtrigger)) {
                o->state = FROG_TONGUE_OUT_PREPARE;
                o->timer = 0;
            } else if (40 <= o->timer) {
                // o->state = FROG_JUMP_PREPARE;
                o->timer = 0;
            }
        }

        break;
    }
    case FROG_JUMP_PREPARE: {
        o->timer++;
        o->v_q12.x = 0;
        if (o->timer == 6) {
            o->state   = FROG_AIR;
            o->timer   = 0;
            o->v_q12.y = -Q_VOBJ(3.0);
            o->v_q12.x = o->facing * Q_VOBJ(2.0);
        }
        break;
    }
    case FROG_AIR: {
        if (obj_grounded(g, o)) {
            o->v_q12.x = 0;
            o->state   = FROG_JUMP_LAND;
            o->timer   = 0;
        }
        break;
    }
    case FROG_JUMP_LAND: {
        o->timer++;
        o->v_q12.x = 0;
        if (o->timer == 2) {
            o->state = FROG_IDLE;
            o->timer = 0;
        }
        break;
    }
    case FROG_TONGUE_OUT_PREPARE: {
        o->timer++;
        i32 t = ani_len(ANIID_FROG_PREPARE);
        if (t <= o->timer) {
            o->timer = 0;
            o->state = FROG_TONGUE_OUT;
        }
        break;
    }
    case FROG_TONGUE_OUT: {
        o->timer++;

        fr->len_tongue = ease_out_quad(0, FROG_L_TONGUE, o->timer, FROG_TICKS_TONGUE_OUT);
        bool32 grabbed = frog_try_grab_with_tongue(g, o, fr->len_tongue);

        if (!grabbed && FROG_TICKS_TONGUE_OUT <= o->timer) {
            o->timer = 0;
            o->state = FROG_TONGUE_PAUSE;
        }
        break;
    }
    case FROG_TONGUE_PAUSE: {
        o->timer++;
        fr->len_tongue = FROG_L_TONGUE;
        bool32 grabbed = frog_try_grab_with_tongue(g, o, fr->len_tongue);

        if (!grabbed && FROG_TICKS_TONGUE_PAUSE <= o->timer) {
            o->timer = 0;
            o->state = FROG_TONGUE_IN;
        }
        break;
    }
    case FROG_TONGUE_IN: {
        o->timer++;
        fr->len_tongue = ease_in_quad(FROG_L_TONGUE, 0, o->timer, FROG_TICKS_TONGUE_IN);
        bool32 grabbed = frog_try_grab_with_tongue(g, o, fr->len_tongue);

        if (!grabbed && FROG_TICKS_TONGUE_IN <= o->timer) {
            o->timer = 0;
            o->state = FROG_TONGUE_IN_END;
        }
        break;
    }
    case FROG_TONGUE_IN_END: {
        o->timer++;
        if (FROG_TICKS_TONGUE_IN_END <= o->timer) {
            o->timer = 0;
            o->state = FROG_IDLE;
        }
        break;
    }
    case FROG_TONGUE_GRABBED_PAUSE: {
        o->timer++;
        if (FROG_TICKS_TONGUE_GRABBED_PAUSE <= o->timer) {
            o->timer = 0;
            o->state = FROG_TONGUE_GRABBED;
        }
        break;
    }
    case FROG_TONGUE_GRABBED: {
        o->timer++;

        fr->len_tongue = ease_in_quad(fr->len_tongue_grabbed, 0, o->timer, FROG_TICKS_TONGUE_GRABBED);

        obj_s *o_owl = obj_from_handle(fr->owl);
        o_owl->pos.y = o->pos.y + (o->h - o_owl->h) / 2;

        switch (o->facing) {
        case -1: {
            o_owl->pos.x = o->pos.x + o->w / 2 - fr->len_tongue + o_owl->w / 2;
            break;
        }
        case +1: {
            o_owl->pos.x = o->pos.x + o->w / 2 + fr->len_tongue - o_owl->w / 2;
            break;
        }
        }

        if (FROG_TICKS_TONGUE_GRABBED <= o->timer) {
            o_owl->pos.x = o->pos.x + (o->w - o_owl->w) / 2;
            o->timer     = 0;
            o->state     = FROG_EAT;
        }
        break;
    }
    case FROG_EAT: {
        g->cam.cowl.center_req = 1;
        obj_s *o_owl           = obj_from_handle(fr->owl);
        o_owl->pos.x           = o->pos.x + (o->w - o_owl->w) / 2;
        o_owl->pos.y           = o->pos.y + (o->h - o_owl->h) / 2;
        o->timer++;
        if (FROG_TICKS_EAT <= o->timer) {
            o_owl->flags &= ~OBJ_FLAG_DONT_SHOW;
            o_owl->v_q12.y = -Q_VOBJ(8.0);
            o_owl->v_q12.x = o->facing * Q_VOBJ(8.0);
            owl_s *h       = (owl_s *)o_owl->heap;
#if 0
            h->knockback.y          = -Q_VOBJ(4.0);
            h->knockback.x          = o->facing * Q_VOBJ(4.0);
            h->knockback_tick_total = 30;
            h->knockback_tick       = h->knockback_tick_total;
#endif
            g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
            o->timer      = 0;
            o->state      = FROG_SPIT_OUT;
            h->invincible = 0;
            owl_special_state_unset(o_owl);
        }
        break;
    }
    case FROG_SPIT_OUT: {
        o->timer++;
        if (FROG_TICKS_SPIT_OUT <= o->timer) {
            o->timer = 0;
            o->state = FROG_IDLE;
            o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        }
        break;
    }
    }

    obj_move_by_v_q12(g, o);
}

void frog_on_animate(g_s *g, obj_s *o)
{
}

void frog_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    frog_s   *fr  = (frog_s *)o->mem;
    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    pos = v2_i32_add(o->pos, cam);

    i32 w   = 96;
    i32 h   = 48;
    i32 f_x = 0;
    i32 f_y = 0;

    i32    tr_flip = 0 < o->facing ? SPR_FLIP_X : 0;
    v2_i32 tr_pos  = {pos.x + (o->w - w) / 2, pos.y + (o->h - h)};

    switch (o->state) {
    case FROG_IDLE: {
        f_x = (o->timer >> 3) % 6;
        break;
    }
    case FROG_HURT: {
        f_y = 0;
        f_x = (o->timer < ENEMY_HIT_FLASH_TICKS ? 6 : 7);
        tr_pos.x += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        tr_pos.y += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        break;
    }
    case FROG_JUMP_PREPARE: {
        f_y = 1;
        f_x = o->timer < 3 ? 0 : 1;
        break;
    }
    case FROG_AIR: {
        f_y = 1;
        f_x = 2;
        break;
    }
    case FROG_JUMP_LAND: {
        f_y = 1;
        f_x = 1;
        break;
    }
    case FROG_TONGUE_OUT_PREPARE: {
        i32 f = ani_frame_loop(ANIID_FROG_PREPARE, o->timer);
        if (6 <= f) {
            f_y = 2;
            f_x = 2 + (f - 6);
        } else {
            f_y = 3;
            f_x = 2 + f;
        }
        break;
    }
    case FROG_TONGUE_IN_END: {
        f_y = 2;
        f_x = lerp_i32(3, 2, o->timer, FROG_TICKS_TONGUE_IN_END);
        break;
    }
    case FROG_TONGUE_OUT:
    case FROG_TONGUE_PAUSE:
    case FROG_TONGUE_IN:
    case FROG_TONGUE_GRABBED_PAUSE:
    case FROG_TONGUE_GRABBED: {
        f_x = 4;
        f_y = 2;

        if ((o->timer >> 2) & 1) {
            f_x = 5;
        }
        // draw tongue
        v2_i32 pos1 = {pos.x + o->w / 2, pos.y + o->h};

        if (o->state == FROG_TONGUE_PAUSE && ((o->timer >> 2) & 1)) {
            pos1.x += o->facing * 2;
        }
        texrec_s tr_1 = asset_texrec(TEXID_FROG, 0, 2 * 48, 16 + FROG_L_TONGUE, 48);
        tr_1.w        = 16 + fr->len_tongue;

        switch (o->facing) {
        case -1: {
            pos1.x -= tr_1.w;
            break;
        }
        case +1: {
            tr_1.y += 48;
            tr_1.x += 16 + FROG_L_TONGUE - tr_1.w;
            break;
        }
        }

        if (o->state == FROG_TONGUE_GRABBED_PAUSE || o->state == FROG_TONGUE_GRABBED) {
            f_x = 6;
            tr_1.y += 48 * 2;
        }

        pos1.y -= h;

        gfx_spr(ctx, tr_1, pos1, 0, 0);
        break;
    }
    case FROG_EAT: {
        f_y = 4;
        f_x = 2 + frame_from_ticks_pingpong(o->timer >> 3, 3);
        break;
    }
    case FROG_SPIT_OUT: {
        f_y = 2;
        f_x = 3;
        break;
    }
    }

    texrec_s tr = asset_texrec(TEXID_FROG, f_x * w, f_y * h, w, h);
    gfx_spr(ctx, tr, tr_pos, tr_flip, 0);
}

bool32 frog_try_grab_with_tongue(g_s *g, obj_s *o, i32 l_tongue)
{
    obj_s *owl = owl_if_present_and_alive(g);
    if (!owl) return 0;

    owl_s *h = (owl_s *)owl->heap;
    if (h->special_state) return 0;

    v2_i32 p_tongue = {o->pos.x + o->w / 2 + o->facing * l_tongue,
                       o->pos.y + o->h / 2};

    rec_i32 r_tongue = {p_tongue.x - 24 / 2, p_tongue.y - 24 / 2, 24, 24};
    bool32  grabbed  = overlap_rec(r_tongue, obj_aabb(owl));
    if (!grabbed) return 0;

    frog_s *fr             = (frog_s *)o->mem;
    o->state               = FROG_TONGUE_GRABBED_PAUSE;
    o->timer               = 0;
    fr->len_tongue_grabbed = l_tongue;
    fr->owl                = handle_from_obj(owl);
    owl_special_state(g, owl, OWL_SPECIAL_ST_EAT_FROG);
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
    owl->flags |= OBJ_FLAG_DONT_SHOW;
    o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
    h->special_state = OWL_SPECIAL_ST_EAT_FROG;
    return 1;
}

void frog_on_hurt(g_s *g, obj_s *o)
{
    if (o->state == FROG_DIE) return;

    if (o->health) {
        o->state = FROG_HURT;
    } else {
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        o->state     = FROG_DIE;
        o->on_update = enemy_on_update_die;
        g->enemies_killed++;
        g->enemy_killed[ENEMYID_FROG]++;
    }
    o->timer = 0;
}