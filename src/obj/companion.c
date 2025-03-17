// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero.h"

enum {
    COMPANION_ST_IDLE,
    COMPANION_ST_SWAP_ITEMS,
    COMPANION_ST_ATTACK,
    COMPANION_ST_COLLECT_COIN,
};

#define COMPANION_ATTACK_COOLDOWN      50
#define COMPANION_DSTSQ_ENEMY          8000
#define COMPANION_DSTSQ_HERO_COME_BACK 35000

typedef struct {
    b8 sit;
    b8 holds_spear;
    u8 attack_tick;
    u8 hitID;
} companion_s;

void     companion_on_update(g_s *g, obj_s *o);
void     companion_on_animate(g_s *g, obj_s *o);
void     companion_follow_hero(g_s *g, obj_s *o, obj_s *ohero);
void     companion_swap_items(g_s *g, obj_s *o, obj_s *ohero);
bool32   companion_sit(g_s *g, obj_s *o, obj_s *ohero);
obj_s   *companion_enemy_target(g_s *g, obj_s *o, u32 dsq_max);
void     companion_attack_target(g_s *g, obj_s *o, obj_s *otarget);
hitbox_s companion_spear_hitbox(obj_s *o);

obj_s *companion_create(g_s *g)
{
    obj_s       *o = obj_create(g);
    companion_s *c = (companion_s *)o->mem;
    o->ID          = OBJID_HERO_COMPANION;
    o->on_update   = companion_on_update;
    o->on_animate  = companion_on_animate;
    o->w           = 16;
    o->h           = 16;
    o->facing      = +1;
    return o;
}

void companion_on_update(g_s *g, obj_s *o)
{
    companion_s *c     = (companion_s *)o->mem;
    obj_s       *ohero = obj_get_hero(g);

    o->timer++;
    v2_i32 p1             = obj_pos_center(o);
    v2_i32 p2             = {0};
    bool32 swap_items     = 0;
    u32    dst_hero       = 0;
    i32    itemswap_ticks = 0;

    if (ohero) {
        hero_s *h      = (hero_s *)ohero->heap;
        p2             = obj_pos_center(ohero);
        dst_hero       = v2_i32_distancesq(p1, p2);
        itemswap_ticks = h->item_swap_tick;
        swap_items     = 0 < h->item_swap_tick;
    }

    if (swap_items) {
        o->state = COMPANION_ST_SWAP_ITEMS;
        o->timer = 0;
    } else if (o->state == COMPANION_ST_SWAP_ITEMS) {
        o->state = COMPANION_ST_IDLE;
        o->timer = 0;
    }

    switch (o->state) {
    case COMPANION_ST_IDLE: {
        bool32 should_attack =
            c->holds_spear &&
            companion_enemy_target(g, o, COMPANION_DSTSQ_ENEMY) &&
            dst_hero < COMPANION_DSTSQ_HERO_COME_BACK / 2;

        if (should_attack) {
            o->state    = COMPANION_ST_ATTACK;
            o->subtimer = COMPANION_ATTACK_COOLDOWN;
            o->timer    = 0;
        } else {
            if (dst_hero < 6700) {
                o->subtimer++;
            } else {
                o->subtimer = 0;
            }

            if (itemswap_ticks) {
                o->subtimer = 0;
            }

            bool32 do_sit = 0;
            if (300 <= o->subtimer) {
                do_sit = companion_sit(g, o, ohero);
            }

            if (!do_sit) {
                c->sit = 0;

                if (ohero) {
                    companion_follow_hero(g, o, ohero);
                }
            }
        }
        break;
    }
    case COMPANION_ST_SWAP_ITEMS: {
        c->sit = 0;
        companion_swap_items(g, o, ohero);
        break;
    }
    case COMPANION_ST_ATTACK: {
        if (o->subtimer) { // attack cooldown
            o->subtimer--;
        }
        if (c->attack_tick) {
            c->attack_tick++;
            if (5 <= c->attack_tick && c->attack_tick < 8) {
                hero_attackbox(g, companion_spear_hitbox(o));
            }
            if (ani_len(ANIID_COMPANION_ATTACK) <= c->attack_tick) {
                c->attack_tick = 0;
                o->subtimer    = COMPANION_ATTACK_COOLDOWN;
            }
        } else if (COMPANION_DSTSQ_HERO_COME_BACK <= dst_hero) {
            o->state    = COMPANION_ST_IDLE;
            o->timer    = 0;
            o->subtimer = 0;
        } else {
            obj_s *otarget = companion_enemy_target(g, o, COMPANION_DSTSQ_ENEMY);
            if (otarget) {
                companion_attack_target(g, o, otarget);
                v2_i32 pt = obj_pos_center(otarget);
                if (p1.x + 8 < pt.x) {
                    o->facing = +1;
                } else if (p1.x - 8 > pt.x) {
                    o->facing = -1;
                }
            } else {
                o->state    = COMPANION_ST_IDLE;
                o->timer    = 0;
                o->subtimer = 0;
            }
        }

        break;
    }
    }

    switch (o->state) {
    case COMPANION_ST_IDLE:
    case COMPANION_ST_SWAP_ITEMS: {
        if (ohero) {
            if (p1.x + 8 < p2.x) {
                o->facing = +1;
            } else if (p1.x - 8 > p2.x) {
                o->facing = -1;
            }
        }
        break;
    }
    default: break;
    }

    if (v2_i16_lensq(o->v_q8) < 5000) { // prevent jittering
        o->v_q8.x = 0;
        o->v_q8.y = 0;
    }
    obj_move_by_v_q8(g, o);

    if (ohero && !((hero_s *)ohero->heap)->holds_spear) {
        c->holds_spear = 1;
    } else {
        c->holds_spear = 0;
    }
}

void companion_on_animate(g_s *g, obj_s *o)
{
    companion_s *c = (companion_s *)o->mem;

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
    o->animation++;
    if (o->facing == +1) {
        spr->flip = SPR_FLIP_X;
    } else {
        spr->flip = 0;
    }

    i32 frx     = 0;
    i32 fry     = 0;
    spr->offs.x = (o->w - 96) / 2;
    spr->offs.y = (o->h - 64) / 2 - 2;

    if (c->sit) {
        frx       = (o->animation >> 4) % 3;
        fry       = 4;
        spr->flip = 0;

        if (o->facing == +1) {
            frx += 3;
        }
    } else if (c->attack_tick) {
        // attack animation
        fry = 5;
        frx = ani_frame(ANIID_COMPANION_ATTACK, c->attack_tick);
    } else {
        // fly animation
        fry = 0;
        if (o->state == COMPANION_ST_ATTACK) {
            fry = 2;
        } else if (c->holds_spear) {
            fry = 1;
        }

        frx = ani_frame(ANIID_COMPANION_FLY, o->animation);
    }

    spr->trec = asset_texrec(TEXID_COMPANION, frx * 96, fry * 64, 96, 64);
}

void companion_swap_items(g_s *g, obj_s *o, obj_s *ohero)
{
    companion_s *c  = (companion_s *)o->mem;
    hero_s      *h  = (hero_s *)ohero->heap;
    v2_i32       p1 = obj_pos_center(o);
    v2_i32       p2 = obj_pos_center(ohero);
    v2_i32       v  = v2_i32_from_i16(o->v_q8);

    p2.x += ohero->facing * 10 + clamp_sym_i32(ohero->v_q8.x >> 4, 15);
    p2.y -= 10 + (HERO_ITEMSWAP_TICKS - h->item_swap_tick);
    v2_i32 arrival_player = steer_arrival(p1, v, p2, 2000, 50);

    o->v_q8.x += arrival_player.x;
    o->v_q8.y += arrival_player.y;
}

void companion_follow_hero(g_s *g, obj_s *o, obj_s *ohero)
{
    companion_s *c  = (companion_s *)o->mem;
    v2_i32       p1 = obj_pos_center(o);
    v2_i32       p2 = obj_pos_center(ohero);
    v2_i32       v  = v2_i32_from_i16(o->v_q8);

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
BREAKLOOP:;

    v2_i32            arrival_player = steer_arrival(p1, v, p2, 900, 120);
    v2_i32            tile_avoid     = {0};
    rec_i32           rbounds        = {o->pos.x - 16, o->pos.y - 16, o->w + 32, o->h + 32};
    tile_map_bounds_s bounds         = tile_map_bounds_rec(g, rbounds);

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            if (!g->tiles[x + y * g->tiles_x].collision) continue;

            v2_i32 ptile  = {x * 16 + 8, y * 16 + 8};
            v2_i32 tavoid = steer_flee(p1, v, ptile, 256);
            tile_avoid    = v2_i32_add(tile_avoid, tavoid);
        }
    }

    if (tile_avoid.x | tile_avoid.y) {
        tile_avoid = v2_i32_truncate_fast(tile_avoid, 256);
    }

    v2_i32 to_add = v2_i32_add(tile_avoid, arrival_player);
    to_add        = v2_i32_truncate_fast(to_add, 128);

    o->v_q8.x += to_add.x;
    o->v_q8.y += to_add.y;
}

void companion_attack_target(g_s *g, obj_s *o, obj_s *otarget)
{
    companion_s *c        = (companion_s *)o->mem;
    v2_i32       p        = obj_pos_center(o);
    v2_i32       ptargetc = obj_pos_center(otarget);
    v2_i32       ptarget1 = {ptargetc.x - 40, ptargetc.y - 25};
    v2_i32       ptarget2 = {ptargetc.x + 40, ptargetc.y - 25};

    u32    ds1     = v2_i32_distancesq(p, ptarget1);
    u32    ds2     = v2_i32_distancesq(p, ptarget2);
    u32    dtarget = 0;
    v2_i32 ptarget = {0};

    if (ds1 < ds2) {
        ptarget = ptarget1;
        dtarget = ds1;
    } else {
        ptarget = ptarget2;
        dtarget = ds2;
    }

    if (dtarget < 1000 && o->subtimer == 0) { // attack target if close
        c->hitID       = game_hero_hitID_next(g);
        c->attack_tick = 1;
        o->v_q8.x      = 0;
        o->v_q8.y      = 0;
    } else { // fly close to target
        v2_i32 v       = v2_i32_from_i16(o->v_q8);
        v2_i32 arrival = steer_arrival(p, v, ptarget, 900, 50);
        v2_i32 to_add  = arrival;
        to_add         = v2_i32_truncate_fast(to_add, 128);

        o->v_q8.x += to_add.x;
        o->v_q8.y += to_add.y;
    }
}

bool32 companion_sit(g_s *g, obj_s *o, obj_s *ohero)
{
    companion_s *c         = (companion_s *)o->mem;
    v2_i32       p1        = obj_pos_center(o);
    v2_i32       p2        = obj_pos_center(ohero);
    v2_i32       v         = v2_i32_from_i16(o->v_q8);
    v2_i32       place_sit = {0};
    bool32       can_sit   = 0;

    if (c->sit) {
        place_sit.x = o->pos.x + o->w / 2;
        place_sit.y = o->pos.y + o->h / 2;
        if (map_blocked(g, obj_rec_bottom(o)) &&
            !map_blocked(g, obj_aabb(o))) {
            can_sit = 1;
        }
    } else {
        rec_i32           rbounds = {p2.x - 32, p2.y - 32, 64, 64};
        tile_map_bounds_s bounds  = tile_map_bounds_rec(g, rbounds);
        bounds.y1                 = max_i32(bounds.y1, 1);
        for (i32 y = bounds.y1; y <= bounds.y2; y++) {
            for (i32 x = bounds.x1; x <= bounds.x2; x++) {
                i32 k = x + y * g->tiles_x;
                if (g->tiles[k].collision && !g->tiles[k - g->tiles_x].collision) {
                    place_sit.x = x * 16 + 8;
                    place_sit.y = y * 16 - o->h / 2;
                    can_sit     = 1;
                }
            }
        }
    }

    if (!can_sit) {
        c->sit = 0;
        return 0;
    }

    i32 ds = v2_i32_distancesq(p1, place_sit);

    if (ds < 20) {
        o->pos.x  = place_sit.x - o->w / 2;
        o->pos.y  = place_sit.y - o->h / 2;
        o->v_q8.x = 0;
        o->v_q8.y = 0;
        if (!c->sit) {
            c->sit       = 1;
            o->animation = 0;
        }
    } else if (ds < 180) {
        v2_i32 pdt = v2_i32_sub(place_sit, p1);
        pdt        = v2_i32_setlen_fast(pdt, 96);
        obj_move_by_q8(g, o, pdt.x, pdt.y);
        o->v_q8.x = 0;
        o->v_q8.y = 0;
        if (c->sit) {
            c->sit       = 0;
            o->animation = 0;
        }
    } else {
        v2_i32 arrival = steer_arrival(p1, v, place_sit, 900, 120);
        o->v_q8.x += arrival.x;
        o->v_q8.y += arrival.y;
        if (c->sit) {
            c->sit       = 0;
            o->animation = 0;
        }
    }

    return 1;
}

obj_s *companion_spawn(g_s *g, obj_s *ohero)
{
    obj_s *o  = companion_create(g);
    o->pos.x  = ohero->pos.x + 0;
    o->pos.y  = ohero->pos.y - 30;
    o->facing = ohero->facing;
    return o;
}

hitbox_s companion_spear_hitbox(obj_s *o)
{
    companion_s *c  = (companion_s *)o->mem;
    hitbox_s     hb = {0};
    hb.r.w          = 64;
    hb.r.h          = 24;
    hb.r.x          = o->pos.x + (o->w - hb.r.w) / 2 + o->facing * 32;
    hb.r.y          = o->pos.y + (o->h - hb.r.h) / 2 + 16;
    hb.damage       = 1;
    hb.hitID        = c->hitID;
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