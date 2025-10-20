// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define FLYBLOB_ATTACK_TICKS     40
#define FLYBLOB_TICKS_HIT_GROUND 100
#define FLYBLOB_AIR_TRIGGER_DSQ  POW2(160)
#define FLYBLOB_ATTACK_DSQ       POW2(48)
#define FLYBLOB_HOVER_DSQ        POW2(64)
#define FLYBLOB_W_ATTACK_DETECT  80

enum {
    FLYBLOB_ST_FLY_IDLE,
    FLYBLOB_ST_DIE,
    FLYBLOB_ST_HURT,
    FLYBLOB_ST_PROPELLER_POP,
    FLYBLOB_ST_FALLING,
    FLYBLOB_ST_FLY_AGGRESSIVE,
    FLYBLOB_ST_FLY_ATTACK,
    FLYBLOB_ST_GROUND_LANDED,
    FLYBLOB_ST_GROUND_IDLE,
    FLYBLOB_ST_GROUND_WALK,
    FLYBLOB_ST_GROUND_REGROW,
    FLYBLOB_ST_FLY_PULL_UP,
};

typedef struct {
    u32 anim_frame;
    u32 anim_propeller;
    i32 force_x;
    u8  attack_timer;
    u8  attack_tick;
    b8  has_propeller;
} flyblob_s;

void flyblob_on_update(g_s *g, obj_s *o);
void flyblob_on_animate(g_s *g, obj_s *o);
void flyblob_on_hook(g_s *g, obj_s *o, i32 hooked);
void flyblob_on_hit(g_s *g, obj_s *o, hitbox_res_s res);

void flyblob_on_hit(g_s *g, obj_s *o, hitbox_res_s res)
{
    flyblob_s *f = (flyblob_s *)o->mem;

    o->health = max_i32((i32)o->health - 1, 0);

    if (!o->health) {
        animobj_create(g, obj_pos_center(o), ANIMOBJ_EXPLOSION_3);
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        o->state     = FLYBLOB_ST_DIE;
        o->on_update = enemy_on_update_die;
        g->enemies_killed++;
        o->on_hitbox          = 0;
        o->hitbox_flags_group = 0;
        o->v_q12.x            = 0;
        o->v_q12.y            = 0;
    } else if (f->has_propeller && o->health < o->health_max - 2) {
        o->state   = FLYBLOB_ST_PROPELLER_POP;
        o->v_q12.x = 0;
        o->v_q12.y = 0;
    } else {
        o->state = FLYBLOB_ST_HURT;
        if (f->has_propeller) {
            o->v_q12.x = Q_VOBJ(2.0) * sgn_i32(res.dx_q4);
            o->v_q12.y = 0;
        } else {
            o->v_q12.x = 0;
            o->v_q12.y = 0;
        }
    }

    if (res.dx_q4) {
        o->facing = -sgn_i32(res.dx_q4);
    }
    o->timer     = 0;
    o->animation = 0;
}

void flyblob_load(g_s *g, map_obj_s *mo)
{
    obj_s     *o          = obj_create(g);
    flyblob_s *f          = (flyblob_s *)o->mem;
    o->editorUID          = mo->UID;
    o->ID                 = OBJID_FLYBLOB;
    o->on_update          = flyblob_on_update;
    o->on_animate         = flyblob_on_animate;
    o->on_hook            = flyblob_on_hook;
    o->on_hitbox          = flyblob_on_hit;
    o->hitbox_flags_group = HITBOX_FLAG_GROUP_ENEMY | HITBOX_FLAG_GROUP_TRIGGERS_CALLBACK;

    o->w = 24;
    o->h = 24;
    obj_place_to_map_obj(o, mo, 0, 0);
    o->health_max = 4;
    o->health     = o->health_max;
    o->facing     = +1;

    o->flags = OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_HOOKABLE |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_CLAMP_TO_ROOM;
    o->moverflags    = OBJ_MOVER_TERRAIN_COLLISIONS;
    f->has_propeller = 1;
}

void flyblob_on_update(g_s *g, obj_s *o)
{
    flyblob_s *f = (flyblob_s *)o->mem;

    v2_i32 pctr  = obj_pos_center(o);
    v2_i32 phero = {0};

    obj_s *owl = owl_if_present_and_alive(g);
    if (owl) {
        phero = obj_pos_center(owl);
    }

    bool32 grounded = obj_grounded(g, o);

    switch (o->state) {
    case FLYBLOB_ST_HURT: {
        o->timer++;

        if (ENEMY_HIT_FREEZE_TICKS <= o->timer) {
            o->state   = FLYBLOB_ST_FLY_IDLE;
            o->timer   = 0;
            o->v_q12.x = 0;
            o->v_q12.y = 0;
            if (f->has_propeller) {
                o->state = FLYBLOB_ST_FLY_AGGRESSIVE;
            }
        } else {
            if (f->has_propeller) {
                obj_v_q8_mul(o, 248, 248);
            } else {
            }
        }
        break;
    }
    case FLYBLOB_ST_FLY_IDLE: {
        o->v_q12.x = 0;
        o->v_q12.y = 0;
        // wander behaviour
        break;
    }
    case FLYBLOB_ST_FLY_AGGRESSIVE: {
        o->timer++;
        if (owl) {
            v2_i32 pseek = phero;

            if (pseek.x < pctr.x) {
                o->facing = -1;
            }
            if (pseek.x > pctr.x) {
                o->facing = +1;
            }

            pseek.y -= 12;
            pseek.x += 72 * sgn_i32(pctr.x - phero.x);
            // TODO
#if 0
            v2_i32 steer = steer_arrival(pctr, o->v_q12, pseek, Q_VOBJ(2.0), 32);
            o->v_q12     = v2_i32_add(o->v_q12, steer);
#endif

            rec_i32 rattack_d = {
                o->pos.x + o->w / 2 - (o->facing < 0 ? FLYBLOB_W_ATTACK_DETECT : 0), o->pos.y, FLYBLOB_W_ATTACK_DETECT, 20};
            if (40 <= o->timer && overlap_rec(rattack_d, obj_aabb(owl))) {
                o->state          = FLYBLOB_ST_FLY_ATTACK;
                o->timer          = 0;
                o->v_q12.x        = 0;
                o->v_q12.y        = 0;
                f->anim_frame     = 0;
                f->anim_propeller = 0;
            }
        } else {
            o->state = FLYBLOB_ST_FLY_IDLE;
            o->timer = 0;
        }
        break;
    }
    case FLYBLOB_ST_PROPELLER_POP: {
        o->timer++;

        if (ani_len(ANIID_FBLOB_POP) <= o->timer) {
            o->timer = 0;
            o->state = FLYBLOB_ST_FALLING;

            if (f->force_x == 0) {
                o->v_q12.x = 0;
                o->v_q12.y = Q_VOBJ(2.0);
            } else {
                o->v_q12.x = +Q_VOBJ(4.0) * f->force_x;
                o->v_q12.y = -Q_VOBJ(2.5);
            }
        }
        o->bumpflags = 0;
        break;
    }
    case FLYBLOB_ST_FALLING: {
        o->timer++;

        if (obj_grounded(g, o)) {
            o->state   = FLYBLOB_ST_GROUND_LANDED;
            o->timer   = 0;
            o->v_q12.x = 0;
            o->v_q12.y = 0;
        } else {
            if (o->bumpflags & OBJ_BUMP_X) {
                o->v_q12.x = -o->v_q12.x;
            }
            o->v_q12.y += Q_VOBJ(0.30);
            o->v_q12.y = min_i32(o->v_q12.y, Q_VOBJ(5.0));
            obj_vx_q8_mul(o, 252);
        }
        o->bumpflags = 0;
        break;
    }
    case FLYBLOB_ST_GROUND_LANDED: {
        o->timer++;
        o->v_q12.x = 0;
        o->v_q12.y += Q_VOBJ(0.5);
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q12.y = 0;
        }
        o->bumpflags = 0;

        if (ani_len(ANIID_FBLOB_LAND_GROUND) <= o->timer) {
            o->state = FLYBLOB_ST_GROUND_IDLE;
            o->timer = 0;
        }
        break;
    }
    case FLYBLOB_ST_GROUND_IDLE: {
        o->timer++;
        o->v_q12.x = 0;
        o->v_q12.y += Q_VOBJ(0.5);
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q12.y = 0;
        }
        o->bumpflags = 0;

        if (50 <= o->timer) {
            o->state   = FLYBLOB_ST_GROUND_REGROW;
            o->timer   = 0;
            o->v_q12.x = 0;
            o->v_q12.y = 0;
        }
        break;
    }
    case FLYBLOB_ST_GROUND_WALK: {
        o->timer++;

        if (50 <= o->timer) {
            o->state   = FLYBLOB_ST_GROUND_REGROW;
            o->timer   = 0;
            o->v_q12.x = 0;
            o->v_q12.y = 0;
        } else {
            o->v_q12.y += Q_VOBJ(0.5);
            if (o->bumpflags & OBJ_BUMP_Y) {
                o->v_q12.y = 0;
            }
            if (o->bumpflags & OBJ_BUMP_X) {
                o->v_q12.x = -o->v_q12.x;
            }

            i32 vs = sgn_i32(o->v_q12.x);
            if (vs) {
                o->facing = vs;
            }
        }

        o->bumpflags = 0;
        break;
    }
    case FLYBLOB_ST_GROUND_REGROW: {
        o->v_q12.x = 0;
        o->v_q12.y = 0;

        o->timer++;
        if (ani_len(ANIID_FBLOB_REGROW) <= o->timer) {
            o->state          = FLYBLOB_ST_FLY_AGGRESSIVE;
            o->timer          = 0;
            f->anim_frame     = 0;
            f->anim_propeller = 0;
            f->has_propeller  = 1;
            obj_move(g, o, 0, -4);
            o->bumpflags = 0;
        }
        break;
    }
    case FLYBLOB_ST_FLY_ATTACK: {
        o->timer++;
        i32 k = ani_frame(ANIID_FBLOB_ATTACK, o->timer);

        if (k < 0) {
            o->state = FLYBLOB_ST_FLY_AGGRESSIVE;
            o->timer = 0;
        } else if (6 <= o->timer && o->timer < 10) { // dashing forward
            obj_move(g, o, +o->facing * 2, 0);
        } else if (10 <= o->timer && o->timer < 18) {
            obj_move(g, o, +o->facing * 4, 0);
        } else if (o->timer > 25) {
            obj_move(g, o, -o->facing * 2, 0);
        }

        if (8 <= k) {
#define FLYBLOB_W_HITBOX 30
#if 0
            hitbox_s  hbb = hitbox_gen(g);
            hitbox_s *hb  = &hbb;
            hitbox_set_rec(hb, o->pos.x + o->w / 2 - (o->facing < 0 ? FLYBLOB_W_HITBOX : 0),
                           o->pos.y, FLYBLOB_W_HITBOX, 20);
            hb->ID                       = HITBOXID_FLYBLOB;
            hb->dx                       = o->facing;
            g->hitboxes[g->n_hitboxes++] = hbb;
#endif
        }
        break;
    }
    case FLYBLOB_ST_FLY_PULL_UP: {
        if (150 <= o->timer) {
            g->freeze_tick = 2;
            o->v_q12.y >>= 1;
            o->v_q12.x >>= 1;
            grapplinghook_destroy(g, &g->ghook);
            break;
        }

        g->cam.cowl.force_lower_ceiling = min_i32(o->timer << 2, 60);

        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q12.y = 0;
        }
        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q12.x = -o->v_q12.x;
        }
        o->bumpflags = 0;

        v2_i32 frope = {0};
        i32    force = grapplinghook_f_at_obj_proj_v(&g->ghook, o, (v2_i32){0}, &frope);

        if (100 <= force) {
            if (frope.x < -2000) {
                o->v_q12.x -= 64;
            }
            if (frope.x > +2000) {
                o->v_q12.x += 64;
            }
        }

        i32 acc_y  = min_i32(o->timer << 4, Q_VOBJ(0.5));
        o->v_q12.y = max_i32(o->v_q12.y - acc_y, -Q_VOBJ(6.0));
        break;
    }
    }

    obj_move_by_v_q12(g, o);
}

void flyblob_on_hook(g_s *g, obj_s *o, i32 hooked)
{
    switch (hooked) {
    case 0:
        o->timer = 0;
        o->state = FLYBLOB_ST_PROPELLER_POP;
        o->flags &= ~(OBJ_FLAG_HURT_ON_TOUCH |
                      OBJ_FLAG_HOOKABLE);
        o->flags |= OBJ_FLAG_OWL_JUMPSTOMPABLE;
        break;
    case 1:
    default:
        o->timer   = 0;
        o->state   = FLYBLOB_ST_FLY_PULL_UP;
        o->v_q12.x = 0;
        o->v_q12.y = 0;
        break;
    }
}

void flyblob_on_animate(g_s *g, obj_s *o)
{
    flyblob_s    *f   = (flyblob_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    if (o->facing == -1) {
        spr->flip = 0;
    } else {
        spr->flip = SPR_FLIP_X;
    }

    i32 w        = 64;
    i32 h        = 64;
    i32 fr_x     = 0;
    i32 fr_y     = 0;
    o->n_sprites = 1;
    spr->offs.x  = (o->w - w) / 2;
    spr->offs.y  = (o->h - h) / 2;

    switch (o->state) {
    case FLYBLOB_ST_DIE:
    case FLYBLOB_ST_HURT: {
        fr_x = (o->timer < ENEMY_HIT_FLASH_TICKS ? 6 : 7);
        fr_y = 0;
        spr->offs.x += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        spr->offs.y += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        break;
    }
    case FLYBLOB_ST_FLY_AGGRESSIVE:
    case FLYBLOB_ST_FLY_IDLE: {
#define FLYBLOB_BOP_DIV 4
#define FLYBLOB_BOP_N   6
        static const i8 y_offs[FLYBLOB_BOP_N * 2] = {-1, -2, -3, -4, -4, -3,
                                                     -2, -1, -1, +0, +0, -1};

        f->anim_propeller++;
        f->anim_frame++;

        spr->offs.y += y_offs[((f->anim_frame << 1) / FLYBLOB_BOP_DIV) % (FLYBLOB_BOP_N * 2)];

        fr_x = ((f->anim_frame / FLYBLOB_BOP_DIV) % FLYBLOB_BOP_N);
        fr_y = ((f->anim_propeller / 3) & 3);
        if (o->state == FLYBLOB_ST_FLY_AGGRESSIVE) {
            fr_y += 16;
        }
        break;
    }
    case FLYBLOB_ST_FLY_ATTACK: {
        i32 k = ani_frame(ANIID_FBLOB_ATTACK, o->timer);
        fr_y  = 8 + (k >> 3);
        fr_x  = k & 7;
        break;
    }
    case FLYBLOB_ST_PROPELLER_POP: {
        fr_y = 4;
        fr_x = ani_frame(ANIID_FBLOB_POP, o->timer);
        break;
    }
    case FLYBLOB_ST_FALLING: {
        fr_y = 4;
        fr_x = 4 + (((o->timer) >> 1) & 3);
        break;
    }
    case FLYBLOB_ST_GROUND_IDLE: {
        fr_y = 12;
        fr_x = ani_frame_loop(ANIID_FBLOB_GROUND_IDLE, o->timer);
        break;
    }
    case FLYBLOB_ST_GROUND_LANDED: {
        i32 k = ani_frame(ANIID_FBLOB_LAND_GROUND, o->timer);
        fr_y  = 5 + (k >> 3);
        fr_x  = k & 7;
        break;
    }
    case FLYBLOB_ST_GROUND_WALK: {
        fr_y = 7;
        fr_x = (o->timer >> 2) & 7;
        break;
    }
    case FLYBLOB_ST_GROUND_REGROW: {
        i32 k = ani_frame(ANIID_FBLOB_REGROW, o->timer);
        fr_y  = 13 + (k >> 3);
        fr_x  = k & 7;
        break;
    }
    case FLYBLOB_ST_FLY_PULL_UP: {
#define FLYBLOB_PULL_UP_T1 12
        if (o->timer < FLYBLOB_PULL_UP_T1) {
            fr_y = 9;
            fr_x = lerp_i32(0, 6 + 1, o->timer, FLYBLOB_PULL_UP_T1);
        } else {
            fr_y = 10;
            fr_x = ((o->timer * 2) / 3) % 3;
        }

        break;
    }
    }
    spr->trec = asset_texrec(TEXID_FLYBLOB, fr_x * w, fr_y * h, w, h);
}