// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl/owl.h"

enum {
    COMPANION_ST_IDLE,
    COMPANION_ST_GAMEOVER,
    COMPANION_ST_GATHER_ITEM,
};

#define COMPANION_ATTACK_COOLDOWN      50
#define COMPANION_DSTSQ_ENEMY          8000
#define COMPANION_DSTSQ_HERO_COME_BACK 35000

typedef struct companion_s {
    u32          mode_lerp_q8;
    b8           sit;
    u8           attack_tick;
    u8           hitID;
    u8           may_rest;
    v2_i32       tile_avoid;
    bool32       carries_item;
    obj_handle_s o_item;
} companion_s;

void            companion_on_update(g_s *g, obj_s *o);
void            companion_on_animate(g_s *g, obj_s *o);
void            companion_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void            companion_follow_owl(g_s *g, obj_s *o, obj_s *owl);
obj_s          *companion_enemy_target(g_s *g, obj_s *o, u32 dsq_max);
hitbox_legacy_s companion_spear_hitbox(obj_s *o);
void            companion_seek_pos(g_s *g, obj_s *o, v2_i32 pos);

obj_s *companion_create(g_s *g)
{
    obj_s       *o = obj_create(g);
    companion_s *c = (companion_s *)o->mem;
    o->ID          = OBJID_COMPANION;
    obj_tag(g, o, OBJ_TAG_COMPANION);
    o->on_update  = companion_on_update;
    o->on_animate = companion_on_animate;
    o->on_draw    = companion_on_draw;
    o->w          = 16;
    o->h          = 16;
    o->facing     = +1;
    return o;
}

obj_s *companion_find_gather_item(g_s *g, obj_s *o)
{
    companion_s *c = (companion_s *)o->mem;
    if (c->carries_item) return 0;

    v2_i32 pc        = obj_pos_center(o);
    u32    closest   = 300;
    obj_s *o_closest = 0;

    for (obj_each(g, i)) {
        if (i->ID == OBJID_COIN) {
            v2_i32 p = obj_pos_center(i);
            p.y -= 12;
            u32 d = v2_i32_distance_appr(p, pc);
            if (d < closest) {
                closest   = d;
                o_closest = i;
            }
        }
    }
    return o_closest;
}

void companion_on_update(g_s *g, obj_s *o)
{
    companion_s *c   = (companion_s *)o->mem;
    obj_s       *owl = obj_get_owl(g);

    o->timer++;
    v2_i32 p1      = obj_pos_center(o);
    v2_i32 p2      = {0};
    u32    dst_owl = 0;
    c->tile_avoid.x >>= 1;
    c->tile_avoid.y >>= 1;

    if (owl) {
        owl_s *h = (owl_s *)owl->heap;
        p2       = obj_pos_center(owl);
        dst_owl  = v2_i32_distancesq(p1, p2);

        if (h->stance == OWL_STANCE_ATTACK) {
            o->state = COMPANION_ST_IDLE;
        }
    }

    switch (o->state) {
    case COMPANION_ST_GAMEOVER: {
        if (o->timer < 25) {
            break;
        }
        if (owl) {
            companion_follow_owl(g, o, owl);
            if (p1.x + 8 < p2.x) {
                o->facing = +1;
            } else if (p1.x - 8 > p2.x) {
                o->facing = -1;
            }
        }
        break;
    }
    case COMPANION_ST_IDLE: {
        obj_s *o_item = companion_find_gather_item(g, o);
        if (o_item) {
            c->o_item = handle_from_obj(o_item);
            o->state  = COMPANION_ST_GATHER_ITEM;
            o->timer  = 0;
            break;
        }
        if (owl) {
            companion_follow_owl(g, o, owl);
            if (p1.x + 8 < p2.x) {
                o->facing = +1;
            } else if (p1.x - 8 > p2.x) {
                o->facing = -1;
            }
        }
        break;
    }
    case COMPANION_ST_GATHER_ITEM: {
        if (c->carries_item) {
            if (owl) {
                i32 dst = v2_i32_distance_appr(p1, obj_pos_center(owl));
                if (dst < 20) {
                    coins_change(g, 1);
                    snd_play(SNDID_COIN, 0.5f, rngr_f32(0.95f, 1.05f));
                    c->carries_item = 0;
                    o->state        = COMPANION_ST_IDLE;
                    o->timer        = 0;
                } else {
                    companion_follow_owl(g, o, owl);
                }
            }
        } else {
            obj_s *o_item = obj_from_handle(c->o_item);
            if (o_item) {
                i32 dst = v2_i32_distance_appr(p1, obj_pos_center(o_item));
                if (dst < 20) {
                    c->carries_item = 1;
                    obj_delete(g, o_item);
                    c->o_item.o = 0;
                } else {
                    v2_i32 pitem = obj_pos_center(o_item);
                    pitem.y -= 12;
                    companion_seek_pos(g, o, pitem);
                }
            } else {
                o->state = COMPANION_ST_IDLE;
                o->timer = 0;
            }
        }
        if (o->v_q12.x < 0) o->facing = -1;
        if (o->v_q12.x > 0) o->facing = +1;
        break;
    }
    }

    if (v2_i32_lensq(o->v_q12) < Q_VOBJ(5.0)) { // prevent jittering
        o->v_q12.x = 0;
        o->v_q12.y = 0;
    }
    obj_move_by_v_q12(g, o);
}

void companion_on_animate(g_s *g, obj_s *o)
{
    companion_s *c = (companion_s *)o->mem;
    o->animation++;
}

void companion_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    companion_s *c = (companion_s *)o->mem;

    v2_i32 pc = obj_pos_center(o);
    v2_i32 p  = pc;

    obj_s *owl = obj_get_owl(g);
    if (owl) {
        owl_s *h = (owl_s *)owl->heap;

        if (h->stance == OWL_STANCE_ATTACK) {
            return;
        }
        p = companion_pos_swap(o, owl);
    }

    p = v2_i32_add(p, cam);
    p.x -= 64 >> 1;
    p.y -= 64 >> 1;
    texrec_s tr = asset_texrec(TEXID_COMPANION, 0, 0, 64, 64);

    i32 fl = 0;
    if (0 < o->facing) {
        fl = SPR_FLIP_X;
    }

    i32 fr_x = 0;
    i32 fr_y = 0;

    if (c->sit) {
        fr_x = (o->animation >> 4) % 3;
        fr_y = 3;
        fl   = 0;

        if (o->facing == +1) {
            fr_x += 3;
        }
    } else if (c->attack_tick) {
        // attack animation
        fr_y = 5;
        fr_x = ani_frame_loop(ANIID_COMPANION_ATTACK, c->attack_tick);
    } else {
        // fly animation
        fr_y = 0;
        fr_x = ani_frame_loop(ANIID_COMPANION_FLY, o->animation);
    }

    if (o->state == COMPANION_ST_GAMEOVER) {
        if (o->timer < 25) {
            fr_y = 9;
            fr_x = (o->timer < 20 ? 4 : 5);
        } else {
            fr_y = 2;
        }
    }

    if (owl) {
        owl_s *h = (owl_s *)owl->heap;
        if (h->stance_swap_tick && h->stance_swap_tick < 12) {
            fr_y = 9;
            fr_x = lerp_i32(0, 3, h->stance_swap_tick, 11);
        }
    }
    tr.x = fr_x * tr.w;
    tr.y = fr_y * tr.h;

    gfx_ctx_s ctx = gfx_ctx_display();

    if (c->carries_item) {
        i32    fry  = ani_frame_loop(ANIID_GEMS, o->animation);
        i32    frx  = 0;
        v2_i32 pgem = v2_i32_add(pc, cam);
        pgem.y += 2;
        pgem.x -= 16;
        texrec_s trgem = asset_texrec(TEXID_GEMS, frx * 32, fry * 24, 32, 24);
        gfx_spr(ctx, trgem, pgem, fl, 0);
    }

    gfx_spr(ctx, tr, p, fl, 0);
}

v2_i32 companion_pos_swap(obj_s *ocomp, obj_s *o_owl)
{
    owl_s *h  = (owl_s *)o_owl->heap;
    v2_i32 pc = obj_pos_center(ocomp);

    if (h->stance == OWL_STANCE_GRAPPLE && 8 < h->stance_swap_tick) {
        // move to hero
        v2_i32 hc = obj_pos_center(o_owl);
        hc.x -= o_owl->facing * 12;
        hc.y -= 4;
        pc = v2_i32_lerp(pc, hc, h->stance_swap_tick - 8, owl_swap_ticks() - 8);
    }
    return pc;
}

void companion_on_enter_mode(g_s *g, obj_s *o, i32 mode)
{
    companion_s *c = (companion_s *)o->mem;

    switch (mode) {
    case OWL_STANCE_GRAPPLE: {

        break;
    }
    case OWL_STANCE_ATTACK: {
        if (c->carries_item) {
            coins_change(g, 1);
            snd_play(SNDID_COIN, 0.5f, rngr_f32(0.95f, 1.05f));
            c->carries_item = 0;
        }
        break;
    }
    }
}

void companion_avoid_terrain(g_s *g, obj_s *o)
{
    return;
    companion_s *c  = (companion_s *)o->mem;
    v2_i32       p1 = obj_pos_center(o);

    // steer towards tile which is "the most free" from solid tiles
    i32 tx1   = max_i32((p1.x - 32) >> 4, 0);
    i32 ty1   = max_i32((p1.y - 32) >> 4, 0);
    i32 tx2   = min_i32((p1.x + 32) >> 4, g->tiles_x - 1);
    i32 ty2   = min_i32((p1.y + 32) >> 4, g->tiles_y - 1);
    i32 vmin  = I32_MIN;
    i32 txmin = 0;
    i32 tymin = 0;

    if (I32_MIN < vmin) {
        v2_i32 tile_avoid = {(txmin << 4) + 8 - p1.x,
                             (tymin << 4) + 8 - p1.y};
        tile_avoid        = v2_i32_setlen_fast(tile_avoid, 192);
        c->tile_avoid.x += tile_avoid.x >> 1;
        c->tile_avoid.y += tile_avoid.y >> 1;
    }
}

void companion_seek_pos(g_s *g, obj_s *o, v2_i32 pos)
{
    companion_s *c   = (companion_s *)o->mem;
    v2_i32       p1  = obj_pos_center(o);
    v2_i32       vel = o->v_q12;

    companion_avoid_terrain(g, o);

    i32 rad  = 90;
    i32 vmax = Q_VOBJ(5.0);
    i32 dst  = v2_i32_distance_appr(p1, pos);
    i32 vm   = vmax;
    if (dst < rad) {
        vm = lerp_i32(Q_VOBJ(0.025), vm, dst, rad);
    }
    v2_i32 vseek  = steer_seek(p1, vel, pos, vm);
    v2_i32 to_add = vseek;
    if (100 <= dst) {
        to_add = v2_i32_add(to_add, c->tile_avoid);
    }
    to_add = v2_i32_truncate_fast(to_add, Q_VOBJ(0.35));

    o->v_q12.x += to_add.x;
    o->v_q12.y += to_add.y;
}

void companion_gather_item(g_s *g, obj_s *o)
{
}

void companion_follow_owl(g_s *g, obj_s *o, obj_s *ohero)
{
    companion_s *c  = (companion_s *)o->mem;
    v2_i32       p1 = obj_pos_center(o);
    v2_i32       p2 = obj_pos_center(ohero);

    if (c->carries_item) {
        p2.y -= 12;
    } else {
        for (i32 seekx = -30; seekx <= +30; seekx += 20) {
            for (i32 seeky = -40; seeky <= 0; seeky += 5) {
                v2_i32  pseek = {p2.x + seekx * ohero->facing,
                                 p2.y + seeky};
                rec_i32 rseek = {pseek.x - o->w / 2 - 8,
                                 pseek.y - o->h / 2 - 8,
                                 o->w + 16, o->h + 16};

                if (!map_blocked(g, rseek)) {
                    p2 = pseek;
                    goto BREAKLOOP;
                }
            }
        }
    }

BREAKLOOP:;
    companion_seek_pos(g, o, p2);
}

obj_s *companion_spawn(g_s *g, obj_s *owl)
{
    obj_s *o  = companion_create(g);
    o->pos.x  = owl->pos.x + 0;
    o->pos.y  = owl->pos.y - 30;
    o->facing = owl->facing;
    return o;
}

hitbox_legacy_s companion_spear_hitbox(obj_s *o)
{
    companion_s    *c  = (companion_s *)o->mem;
    hitbox_legacy_s hb = {0};
    hb.r.w             = 64;
    hb.r.h             = 24;
    hb.r.x             = o->pos.x + (o->w - hb.r.w) / 2 + o->facing * 32;
    hb.r.y             = o->pos.y + (o->h - hb.r.h) / 2 + 16;
    hb.damage          = 1;
    hb.hitID           = c->hitID;
    return hb;
}

obj_s *companion_enemy_target(g_s *g, obj_s *o, u32 dsq_max)
{
    u32    d_closest = dsq_max;
    v2_i32 p         = obj_pos_center(o);
    obj_s *target    = 0;

    for (obj_each(g, i)) {
        if (!(i->flags & OBJ_FLAG_ENEMY) || !i->health) continue;

        v2_i32 penemy = obj_pos_center(i);
        u32    d      = v2_i32_distancesq(p, penemy);
        if (d < d_closest) {
            d      = d_closest;
            target = i;
        }
    }
    return target;
}

void companion_on_owl_died(g_s *g, obj_s *o)
{
    o->state = COMPANION_ST_GAMEOVER;
    o->timer = 0;
    o->v_q12.x >>= 1;
    o->v_q12.y >>= 1;
}