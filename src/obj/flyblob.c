// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define FLYBLOB_ATTACK_TICKS     40
#define FLYBLOB_TICKS_POP        15 // ticks for propeller pop
#define FLYBLOB_TICKS_HIT_GROUND 100
#define FLYBLOB_AIR_TRIGGER_DSQ  POW2(160)
#define FLYBLOB_ATTACK_DSQ       POW2(48)
#define FLYBLOB_HOVER_DSQ        POW2(64)

enum {
    FLYBLOB_STATE_IDLE,
    FLYBLOB_STATE_PROPELLER_POP,
    FLYBLOB_STATE_FALLING,
    FLYBLOB_STATE_AGGRESSIVE,
    FLYBLOB_STATE_ATTACK,
    FLYBLOB_STATE_GROUND_LANDED,
    FLYBLOB_STATE_GROUND,
    FLYBLOB_STATE_PULL_UP,
};

typedef struct {
    u32   anim_frame;
    u32   anim_propeller;
    i32   force_x;
    u8    attack_timer;
    u8    attack_tick;
    bool8 invincible;
} flyblob_s;

void flyblob_on_update(g_s *g, obj_s *o);
void flyblob_on_animate(g_s *g, obj_s *o);
void flyblob_on_hook(g_s *g, obj_s *o, bool32 hooked);

void flyblob_load(g_s *g, map_obj_s *mo)
{
    obj_s     *o  = obj_create(g);
    flyblob_s *f  = (flyblob_s *)o->mem;
    o->ID         = OBJID_FLYBLOB;
    o->on_update  = flyblob_on_update;
    o->on_animate = flyblob_on_animate;
    o->on_hook    = flyblob_on_hook;

    o->w = 24;
    o->h = 24;
    obj_place_to_map_obj(o, mo, 0, 0);
    o->health_max = 3;
    o->health     = o->health_max;
    o->facing     = 1;

    o->flags = OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_HOOKABLE |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_CLAMP_TO_ROOM;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS;
}

void flyblob_on_update(g_s *g, obj_s *o)
{
    flyblob_s *f = (flyblob_s *)o->mem;

    v2_i32 pctr  = obj_pos_center(o);
    v2_i32 phero = {0};

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        phero = obj_pos_center(ohero);
    }

    o->timer++;
    if (o->state != FLYBLOB_STATE_AGGRESSIVE) {
        f->attack_tick  = 0;
        f->attack_timer = 0;
    }

    bool32 grounded = obj_grounded(g, o);

    switch (o->state) {
    case FLYBLOB_STATE_IDLE: {
        // wander behaviour
        break;
    }
    case FLYBLOB_STATE_AGGRESSIVE: {

        break;
    }
    case FLYBLOB_STATE_PROPELLER_POP: {
        if (o->timer < FLYBLOB_TICKS_POP) break;

        o->timer = 0;
        o->state = FLYBLOB_STATE_FALLING;
        if (f->force_x == 0) {
            o->v_q12.x = 0;
            o->v_q12.y = Q_VOBJ(2.0);
        } else {
            o->v_q12.x = (f->force_x * 1000) >> 8;
            o->v_q12.y = -Q_VOBJ(2.5);
        }
        break;
    }
    case FLYBLOB_STATE_FALLING: {
        if (obj_grounded(g, o)) {
            o->state   = FLYBLOB_STATE_GROUND_LANDED;
            o->timer   = 0;
            o->v_q12.x = 0;
            o->v_q12.y = 0;
            break;
        }

        o->v_q12.y += Q_VOBJ(0.5);
        o->v_q12.y = min_i32(o->v_q12.y, Q_VOBJ(5.0));
        obj_vx_q8_mul(o, 240);
        break;
    }
    case FLYBLOB_STATE_GROUND_LANDED: {
        o->v_q12.x = 0;
        o->v_q12.y += Q_VOBJ(0.5);
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q12.y = 0;
        }
        o->bumpflags = 0;
        if (FLYBLOB_TICKS_HIT_GROUND <= o->timer) {
            o->state = FLYBLOB_STATE_GROUND;
            o->timer = 0;
        }
        break;
    }
    case FLYBLOB_STATE_GROUND: {
        o->v_q12.y += Q_VOBJ(0.5);
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q12.y = 0;
        }
        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q12.x = -o->v_q12.x;
        }
        o->bumpflags = 0;

        i32 vs = sgn_i32(o->v_q12.x);
        if (vs) {
            o->facing = vs;
        }
        break;
    }
    case FLYBLOB_STATE_ATTACK: {
        if (ani_len(ANIID_FBLOB_ATTACK) <= o->timer) {
            o->state = FLYBLOB_STATE_AGGRESSIVE;
            o->timer = 0;
            break;
        }

        i32 t = ani_frame(ANIID_FBLOB_ATTACK, o->timer);
        break;
    }
    case FLYBLOB_STATE_PULL_UP: {
        if (150 <= o->timer) {
            g->freeze_tick = 2;
            o->v_q12.y >>= 1;
            o->v_q12.x >>= 1;
            grapplinghook_destroy(g, &g->ghook);
            break;
        }

        g->cam.chero.force_lower_ceiling = min_i32(o->timer << 2, 60);

        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q12.y = 0;
        }
        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q12.x = -o->v_q12.x;
        }
        o->bumpflags = 0;

        v2_i32 frope = {0};
        i32    f     = grapplinghook_f_at_obj_proj_v(&g->ghook, o, (v2_i32){0}, &frope);

        if (100 <= f) {
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

void flyblob_on_hook(g_s *g, obj_s *o, bool32 hooked)
{
    switch (hooked) {
    case 0:
        o->timer = 0;
        o->state = FLYBLOB_STATE_PROPELLER_POP;
        o->flags &= ~(OBJ_FLAG_HURT_ON_TOUCH |
                      OBJ_FLAG_HOOKABLE);
        o->flags |= OBJ_FLAG_HERO_JUMPSTOMPABLE;
        break;
    case 1:
    default:
        o->timer   = 0;
        o->state   = FLYBLOB_STATE_PULL_UP;
        o->v_q12.x = 0;
        o->v_q12.y = 0;
        break;
    }
}

void flyblob_on_animate(g_s *g, obj_s *o)
{
    flyblob_s    *f   = (flyblob_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    spr->offs.x = (o->w - 64) / 2;
    spr->offs.y = (o->h - 64) / 2;
    if (o->facing == -1) {
        spr->flip = 0;
    } else {
        spr->flip = SPR_FLIP_X;
    }

    i32 state    = o->state;
    i32 fr_x     = 0;
    i32 fr_y     = 0;
    o->n_sprites = 1;

    if (o->enemy.hurt_tick) { // display hurt frame
        if (o->health == 2) {
            fr_x  = 10;
            fr_y  = 1;
            state = -1;
        }
    }

    switch (state) {
    case FLYBLOB_STATE_IDLE: {
#define FLYBLOB_BOP_DIV 4
#define FLYBLOB_BOP_N   6
        static const i8 y_offs[FLYBLOB_BOP_N * 2] = {-1, -2, -3, -4, -4, -3,
                                                     -2, -1, -1, +0, +0, -1};

        f->anim_propeller++;
        f->anim_frame++;

        spr->offs.y += y_offs[(((f->anim_frame << 1) / FLYBLOB_BOP_DIV) % (FLYBLOB_BOP_N * 2))];

        fr_x = 1 + ((f->anim_frame / FLYBLOB_BOP_DIV) % FLYBLOB_BOP_N);
        fr_y = ((f->anim_propeller / 3) & 3);
        break;
    }
    case FLYBLOB_STATE_PROPELLER_POP: {
        fr_y = 4;
        fr_x = (o->timer * 4) / FLYBLOB_TICKS_POP;
        break;
    }
    case FLYBLOB_STATE_FALLING: {
        fr_y = 5;
        fr_x = (((o->timer) >> 1) & 3);
        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q12.x = (-o->v_q12.x * 200) >> 8;
        }
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q12.y = (-o->v_q12.y * 200) >> 8;
        }
        o->bumpflags = 0;
        break;
    }
    case FLYBLOB_STATE_GROUND_LANDED: {
        fr_y = 6;
        fr_x = lerp_i32(0, 16, o->timer, FLYBLOB_TICKS_HIT_GROUND);
        break;
    }
    case FLYBLOB_STATE_GROUND: {
        fr_y = 7;
        fr_x = (o->timer >> 2) & 7;
        break;
    }
#define FLYBLOB_PULL_UP_T1 12

    case FLYBLOB_STATE_PULL_UP: {
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

    spr->trec = asset_texrec(TEXID_FLYBLOB, fr_x * 64, fr_y * 64, 64, 64);
}

void flyblob_on_hit(g_s *g, obj_s *o, hitbox_s hb)
{
    flyblob_s *f = (flyblob_s *)o->mem;

    switch (o->state) {
    case FLYBLOB_STATE_IDLE:
    case FLYBLOB_STATE_AGGRESSIVE: break;
    default: return;
    }

    o->v_q12.x = 0;
    o->v_q12.y = 0;

    f->force_x   = hb.force_q8.x;
    o->animation = 0;
    o->timer     = 0;
    o->state     = FLYBLOB_STATE_PROPELLER_POP;
}
