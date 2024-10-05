// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
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

void flyblob_on_update(game_s *g, obj_s *o);
void flyblob_on_animate(game_s *g, obj_s *o);

void flyblob_on_hit(game_s *g, obj_s *o, hitbox_s hb)
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

void flyblob_load(game_s *g, map_obj_s *mo)
{
    obj_s     *o  = obj_create(g);
    flyblob_s *f  = (flyblob_s *)o->mem;
    o->ID         = OBJ_ID_FLYBLOB;
    o->on_animate = flyblob_on_animate;
    o->on_update  = flyblob_on_update;
    o->w          = 24;
    o->h          = 24;
    o->pos.x      = mo->x + 4;
    o->pos.y      = mo->y + 4;
    o->health_max = 3;
    o->health     = o->health_max;
    o->facing     = 1;

    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY | OBJ_FLAG_PLATFORM_HERO_ONLY |
               OBJ_FLAG_CLAMP_TO_ROOM | OBJ_FLAG_CAN_BE_JUMPED_ON;
    o->n_sprites         = 2;
    tex_s tex            = asset_tex(TEXID_FLYBLOB);
    o->sprites[0].trec.t = tex;
    o->sprites[1].trec.t = tex;
}

void flyblob_on_update(game_s *g, obj_s *o)
{
    flyblob_s *f = (flyblob_s *)o->mem;
    o->timer++;
    o->drag_q8.x    = 256;
    o->drag_q8.y    = 256;
    o->grav_q8.y    = 0;
    obj_s *ohero    = obj_get_tagged(g, OBJ_TAG_HERO);
    i32    dsq_hero = I32_MAX;
    v2_i32 vhero    = {0};
    if (ohero) {
        v2_i32 p1 = obj_pos_center(o);
        v2_i32 p2 = obj_pos_center(ohero);
        p2.y -= 16;
        vhero    = v2_sub(p2, p1);
        dsq_hero = v2_lensq(vhero);
    }

    if (o->state != FLYBLOB_STATE_AGGRESSIVE) {
        f->attack_tick  = 0;
        f->attack_timer = 0;
    }

    switch (o->state) {
    case FLYBLOB_STATE_IDLE: {
        if (o->enemy.hurt_tick) break;
        if (dsq_hero < FLYBLOB_AIR_TRIGGER_DSQ) {
            o->state = FLYBLOB_STATE_AGGRESSIVE;
            o->timer = 0;
            break;
        }

        // wander behaviour
        break;
    }
    case FLYBLOB_STATE_AGGRESSIVE: {
        if (o->enemy.hurt_tick) break;

        if (ohero) {
            i32 s = sgn_i32(vhero.x);
            if (s != 0) {
                o->facing = s;
            }
        }
        if (f->attack_tick) {
            f->attack_tick++;
            if (f->attack_tick == FLYBLOB_ATTACK_TICKS / 3) {
                hitbox_s hb   = {0};
                hb.damage     = 1;
                hb.r.w        = 32;
                hb.r.h        = 32;
                hb.r.y        = o->pos.y - 8;
                hb.force_q8.x = o->facing * 1000;
                if (o->facing == 1) {
                    hb.r.x = o->pos.x + o->w;
                } else {
                    hb.r.x = o->pos.x - hb.r.w;
                }
                obj_game_enemy_attackboxes(g, &hb, 1);
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
            vv     = v2_inv(v2_setlen(vhero, dt));
        } else {
            i32 dt = clamp_i32(hoverd / 2, 8, 256);
            vv     = v2_setlen(vhero, dt);
        }
        o->v_q8 = v2_i16_from_i32(vv, 0);

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

        o->grav_q8.y = 40;
        o->drag_q8.x = 245;
        o->drag_q8.y = 256;
        break;
    }
    case FLYBLOB_STATE_FALLING: {
        if (obj_grounded(g, o)) {
            o->state  = FLYBLOB_STATE_GROUND;
            o->timer  = 0;
            o->v_q8.x = 0;
            o->v_q8.y = 0;
            break;
        }
        o->grav_q8.y = 50;
        o->drag_q8.x = 240;
        o->drag_q8.y = 256;
        break;
    }
    case FLYBLOB_STATE_GROUND: {
        if (o->bumpflags & OBJ_BUMPED_Y) {
            o->v_q8.y = 0;
        }
        if (o->bumpflags & OBJ_BUMPED_X) {
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

        o->grav_q8.y = 50;
        o->drag_q8.x = 256;
        o->drag_q8.y = 256;
        break;
    }
    }
}

void flyblob_on_animate(game_s *g, obj_s *o)
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
        s1->trec.r = (rec_i32){frameprop * 64, 0, 64, 64};
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
        if (o->bumpflags & OBJ_BUMPED_X) {
            o->v_q8.x = (-o->v_q8.x * 200) >> 8;
        }
        if (o->bumpflags & OBJ_BUMPED_Y) {
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
    }

    s0->trec.r = (rec_i32){framex * 64, framey * 64, 64, 64};
}
