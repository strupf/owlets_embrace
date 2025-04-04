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

void flyblob_on_unhooked(g_s *g, obj_s *o);

void flyblob_on_hit(g_s *g, obj_s *o, hitbox_s hb)
{
    flyblob_s *f = (flyblob_s *)o->mem;
    switch (o->state) {
    case FLYBLOB_STATE_IDLE:
    case FLYBLOB_STATE_AGGRESSIVE: break;
    default: return;
    }

    o->v_q8.x = 0;
    o->v_q8.y = 0;
    if (o->health == 2) {
        o->state        = FLYBLOB_STATE_AGGRESSIVE;
        f->attack_timer = 0;
        f->attack_tick  = 0;
        return;
    }

    f->force_x   = hb.force_q8.x;
    o->animation = 0;
    o->timer     = 0;
    o->state     = FLYBLOB_STATE_PROPELLER_POP;
}

void flyblob_load(g_s *g, map_obj_s *mo)
{
    obj_s     *o  = obj_create(g);
    flyblob_s *f  = (flyblob_s *)o->mem;
    o->ID         = OBJID_FLYBLOB;
    o->on_update  = flyblob_on_update;
    o->on_animate = flyblob_on_animate;

    o->w          = 24;
    o->h          = 24;
    o->pos.x      = mo->x + 4;
    o->pos.y      = mo->y + 4;
    o->health_max = 3;
    o->health     = o->health_max;
    o->facing     = 1;

    o->flags = OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_HOOKABLE |
               OBJ_FLAG_ACTOR |
               OBJ_FLAG_ENEMY |
               OBJ_FLAG_HERO_STOMPABLE |
               OBJ_FLAG_HERO_JUMPABLE |
               OBJ_FLAG_CLAMP_TO_ROOM;
    o->moverflags        = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->n_sprites         = 2;
    tex_s tex            = asset_tex(TEXID_FLYBLOB);
    o->sprites[0].trec.t = tex;
    o->sprites[1].trec.t = tex;
}

void flyblob_on_update(g_s *g, obj_s *o)
{
    flyblob_s *f = (flyblob_s *)o->mem;
    o->timer++;
    obj_s *ohero    = obj_get_tagged(g, OBJ_TAG_HERO);
    i32    dsq_hero = I32_MAX;
    v2_i32 vhero    = {0};
    if (ohero) {
        v2_i32 p1 = obj_pos_center(o);
        v2_i32 p2 = obj_pos_center(ohero);
        p2.y -= 16;
        vhero    = v2_i32_sub(p2, p1);
        dsq_hero = v2_i32_lensq(vhero);
    }

    if (o->state != FLYBLOB_STATE_AGGRESSIVE) {
        f->attack_tick  = 0;
        f->attack_timer = 0;
    }

    switch (o->state) {
    case FLYBLOB_STATE_IDLE: {
        if (dsq_hero < FLYBLOB_AIR_TRIGGER_DSQ) {
            o->state = FLYBLOB_STATE_AGGRESSIVE;
            o->timer = 0;
            break;
        }

        // wander behaviour
        break;
    }
    case FLYBLOB_STATE_AGGRESSIVE: {
        if (ohero) {
            i32 s = sgn_i32(vhero.x);
            if (s != 0) {
                o->facing = s;
            }
        }
        if (f->attack_tick) {
            f->attack_tick++;
            if (f->attack_tick == FLYBLOB_ATTACK_TICKS / 3) {
            }
            if (FLYBLOB_ATTACK_TICKS <= f->attack_tick) {
                f->attack_tick = 0;
            }
            break;
        }

        if (FLYBLOB_AIR_TRIGGER_DSQ <= dsq_hero) {
            o->subtimer++;
            if (!ohero || 50 <= o->subtimer) {
                o->state    = FLYBLOB_STATE_IDLE;
                o->subtimer = 0;
                o->timer    = 0;
                break;
            }
        } else {
            o->subtimer = 0;
        }

        i32    hover_dsq = f->attack_timer ? FLYBLOB_HOVER_DSQ : FLYBLOB_ATTACK_DSQ / 2;
        i32    hoverd    = dsq_hero - hover_dsq;
        v2_i32 vv        = v2_i32_from_i16(o->v_q8);

        if (hoverd < 0) { // keep distance
            i32 dt = clamp_i32(-hoverd / 8, 8, 256);
            vv     = v2_i32_inv(v2_i32_setlen(vhero, dt));
        } else {
            i32 dt = clamp_i32(hoverd / 2, 8, 256);
            vv     = v2_i32_setlen(vhero, dt);
        }
        o->v_q8 = v2_i16_from_i32(vv);

        if (f->attack_timer) {
            f->attack_timer--;
        } else if (dsq_hero < FLYBLOB_ATTACK_DSQ) {
            f->attack_tick  = 1;
            f->attack_timer = 100;
            o->v_q8.x       = 0;
            o->v_q8.y       = 0;
        }
        break;
    }
    case FLYBLOB_STATE_PROPELLER_POP: {
        if (o->timer < FLYBLOB_TICKS_POP) break;
        o->timer = 0;
        o->state = FLYBLOB_STATE_FALLING;
        if (f->force_x == 0) {
            o->v_q8.x = 0;
            o->v_q8.y = 500;
        } else {
            o->v_q8.x = (f->force_x * 1000) >> 8;
            o->v_q8.y = -600;
        }
        break;
    }
    case FLYBLOB_STATE_FALLING: {
        if (20 <= o->timer)
            o->flags &= OBJ_FLAG_HURT_ON_TOUCH;
        if (obj_grounded(g, o)) {
            o->state  = FLYBLOB_STATE_GROUND;
            o->timer  = 0;
            o->v_q8.x = 0;
            o->v_q8.y = 0;
            break;
        }

        o->v_q8.y += 50;
        obj_vx_q8_mul(o, 240);
        break;
    }
    case FLYBLOB_STATE_GROUND: {
        o->v_q8.y += 50;
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q8.y = 0;
        }
        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q8.x = -o->v_q8.x;
        }
        o->bumpflags = 0;
        if (o->timer < FLYBLOB_TICKS_HIT_GROUND) {
            o->v_q8.x = 0;
            break;
        }
        if (o->timer == FLYBLOB_TICKS_HIT_GROUND) {
            o->v_q8.x = 128;
        }

        i32 vs = sgn_i32(o->v_q8.x);
        if (vs) {
            o->facing = vs;
        }
        break;
    }
    case FLYBLOB_STATE_PULL_UP: {
        if (100 <= o->timer) {
            g->freeze_tick = 2;
            o->v_q8.y >>= 1;
            flyblob_on_unhooked(g, o);
            break;
        }
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q8.y = 0;
        }
        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q8.x = -o->v_q8.x;
        }
        o->bumpflags = 0;

        i32 acc_y = min_i32(o->timer << 1, 64 + 32);
        o->v_q8.y = max_i32(o->v_q8.y - acc_y, -256 * 6);

        break;
    }
    }

    obj_move_by_v_q8(g, o);
}

void flyblob_on_hook(g_s *g, obj_s *o)
{
    o->timer         = 0;
    o->state         = FLYBLOB_STATE_PULL_UP;
    o->v_q8.x        = 0;
    o->v_q8.y        = 0;
    o->cam_attract_r = 250;
}

void flyblob_on_unhooked(g_s *g, obj_s *o)
{
    o->timer         = 0;
    o->state         = FLYBLOB_STATE_PROPELLER_POP;
    o->cam_attract_r = 0;
    grapplinghook_destroy(g, &g->ghook);
}

void flyblob_on_animate(g_s *g, obj_s *o)
{
    flyblob_s    *f  = (flyblob_s *)o->mem;
    obj_sprite_s *s0 = &o->sprites[0];
    obj_sprite_s *s1 = &o->sprites[1];
    s0->offs.x       = -20;
    s0->offs.y       = -20;
    s1->offs         = s0->offs;
    if (o->facing == -1) {
        s0->flip = 0;
        s1->flip = 0;
    } else {
        s0->flip = SPR_FLIP_X;
        s1->flip = SPR_FLIP_X;
    }

    i32 state    = o->state;
    i32 framex   = 0;
    i32 framey   = 0;
    o->n_sprites = 1;
    if (o->enemy.hurt_tick) { // display hurt frame
        if (o->health == 2) {
            framex = 10;
            framey = 1;
            state  = -1;
        }
    }

    switch (state) {
    case FLYBLOB_STATE_AGGRESSIVE:
    case FLYBLOB_STATE_IDLE: {
        o->n_sprites = 2;
        if (f->attack_tick) {
            o->n_sprites = 1;
            i32 fr       = (f->attack_tick * 10 * 2) / FLYBLOB_ATTACK_TICKS;
            if (10 <= fr) {
                fr = 19 - fr;
            }
            framex = fr;
            framey = 5;
            break;
        }

        framey = 1;
        f->anim_propeller++;
        f->anim_frame++;

        // offset of propeller on each bob frame
        static const i8 propeller_offs[] = {-5, -6, -6, -5, -3,
                                            +0, +2, +3, +2, -2};

        framex        = 1 + frame_from_ticks_pingpong(f->anim_frame / 4, 4);
        u32 frameprop = (f->anim_propeller >> 1) & 3;

        s1->offs.x = s0->offs.x;
        s1->offs.y = s0->offs.y + propeller_offs[framex];
        s1->trec.x = frameprop * 64;
        s1->trec.y = 0;
        s1->trec.w = 64;
        s1->trec.h = 64;
        break;
    }
    case FLYBLOB_STATE_PROPELLER_POP: {
        framey = 2;
        framex = (o->timer * 4) / FLYBLOB_TICKS_POP;
        break;
    }
    case FLYBLOB_STATE_FALLING: {
        framey = 2;
        framex = 6 + (((o->timer - FLYBLOB_TICKS_POP) >> 1) & 3);
        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q8.x = (-o->v_q8.x * 200) >> 8;
        }
        if (o->bumpflags & OBJ_BUMP_Y) {
            o->v_q8.y = (-o->v_q8.y * 200) >> 8;
        }
        o->bumpflags = 0;
        break;
    }
    case FLYBLOB_STATE_GROUND: {
        if (o->timer < FLYBLOB_TICKS_HIT_GROUND) {
            framey = 3;
            framex = (o->timer * 16) / FLYBLOB_TICKS_HIT_GROUND;
            break;
        }
        // walking
        framex = (o->timer >> 2) & 7;
        framey = 4;
        break;
    }
#define FLYBLOB_PULL_UP_T1 12

    case FLYBLOB_STATE_PULL_UP: {
        if (o->timer < FLYBLOB_PULL_UP_T1) {
            framey = 6;
            framex = lerp_i32(0, 6 + 1, o->timer, FLYBLOB_PULL_UP_T1);
        } else {
            framey = 7;
            framex = ((o->timer * 2) / 3) % 3;
        }

        break;
    }
    }
    s0->trec.x = framex * 64;
    s0->trec.y = framey * 64;
    s0->trec.w = 64;
    s0->trec.h = 64;
}
