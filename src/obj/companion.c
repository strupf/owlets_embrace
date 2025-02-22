// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "hero.h"

enum {
    COMPANION_ST_IDLE,
    COMPANION_ST_SWAP_ITEMS,
};

typedef struct {
    u32    seed;
    b32    sit;
    u8     n_hero_hist;
    v2_i16 hero_hist[8];
} companion_s;

void   companion_on_update(g_s *g, obj_s *o);
void   companion_on_animate(g_s *g, obj_s *o);
void   companion_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void   companion_follow_hero(g_s *g, obj_s *o, obj_s *ohero);
void   companion_swap_items(g_s *g, obj_s *o, obj_s *ohero);
bool32 companion_sit(g_s *g, obj_s *o, obj_s *ohero);

obj_s *companion_create(g_s *g)
{
    obj_s       *o = obj_create(g);
    companion_s *c = (companion_s *)o->mem;
    o->ID          = OBJID_HERO_COMPANION;
    o->on_update   = companion_on_update;
    o->on_animate  = companion_on_animate;
    o->on_draw     = companion_on_draw;
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
    v2_i32 p1 = obj_pos_center(o);
    v2_i32 p2 = {0};
    if (ohero) {
        p2 = obj_pos_center(ohero);
    }

    bool32 should_swap_items = 0;
    if (ohero && g->ui_itemswap.started) {
        should_swap_items = (UI_ITEMSWAP_TICKS_POP_UP <= g->ui_itemswap.ticks_in);
    }

    if (should_swap_items) {
        o->state = COMPANION_ST_SWAP_ITEMS;
        o->timer = 0;
    } else if (o->state == COMPANION_ST_SWAP_ITEMS) {
        o->state = COMPANION_ST_IDLE;
        o->timer = 0;
    }

    switch (o->state) {
    case COMPANION_ST_IDLE: {
        if (v2_i32_distancesq(p1, p2) < 6700) {
            o->subtimer++;
        } else {
            o->subtimer = 0;
        }
        if (g->ui_itemswap.ticks_in) {
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
        break;
    }
    case COMPANION_ST_SWAP_ITEMS: {
        c->sit = 0;
        companion_swap_items(g, o, ohero);
        break;
    }
    }

    if (ohero) {
        if (p1.x + 8 < p2.x) {
            o->facing = +1;
        } else if (p1.x - 8 > p2.x) {
            o->facing = -1;
        }
    }

    if (v2_i16_lensq(o->v_q8) < 3000) { // prevent jittering
        o->v_q8.x = 0;
        o->v_q8.y = 0;
    }
    obj_move_by_v_q8(g, o);
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

    static const u8 ticks[8] = {
        8, 2, 2, 4, 4, 3, 5, 6};

    i32 frx     = 0;
    i32 fry     = 0;
    spr->offs.x = (o->w - 64) / 2;
    spr->offs.y = (o->h - 64) / 2 - 2;

    if (c->sit) {
        frx       = (o->animation >> 4) % 3;
        fry       = 3;
        spr->flip = 0;
        if (o->facing == +1) {
            frx += 3;
        }
    } else {
        i32 ticks_total = 0;

        for (i32 n = 0; n < 8; n++) {
            ticks_total += ticks[n];
        }

        o->animation %= ticks_total;

        i32 t = 0;
        for (i32 n = 0; n < 8; n++) {
            t += ticks[n];
            if (o->animation <= t) {
                frx = n;
                break;
            }
        }
    }

    obj_s *ohero = obj_get_hero(g);
    if (ohero) {
        hero_s *h = (hero_s *)ohero->heap;
        if (!h->holds_spear) {
            fry += 1;
        }
    }

    spr->trec = asset_texrec(TEXID_COMPANION, frx * 64, fry * 64, 64, 64);
}

void companion_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    companion_s *c = (companion_s *)o->mem;
    v2_i32       p = v2_i32_add(o->pos, cam);
}

void companion_swap_items(g_s *g, obj_s *o, obj_s *ohero)
{
    companion_s *c  = (companion_s *)o->mem;
    hero_s      *h  = (hero_s *)ohero->heap;
    v2_i32       p1 = obj_pos_center(o);
    v2_i32       p2 = obj_pos_center(ohero);
    v2_i32       v  = v2_i32_from_i16(o->v_q8);

    p2.x += ohero->facing * 10;
    p2.y -= 10 + (UI_ITEMSWAP_TICKS - g->ui_itemswap.ticks_in);
    v2_i32 arrival_player = steer_arrival(p1, v, p2, 900, 50);

    o->v_q8.x += arrival_player.x;
    o->v_q8.y += arrival_player.y;
}

void companion_follow_hero(g_s *g, obj_s *o, obj_s *ohero)
{
    companion_s *c  = (companion_s *)o->mem;
    v2_i32       p1 = obj_pos_center(o);
    v2_i32       p2 = obj_pos_center(ohero);
    v2_i32       v  = v2_i32_from_i16(o->v_q8);

    for (i32 seekx = -40; seekx <= +40; seekx += 20) {
        for (i32 seeky = -20; seeky <= 0; seeky += 5) {
            v2_i32  pseek = {p2.x + seekx * ohero->facing,
                             p2.y + seeky};
            rec_i32 rseek = {pseek.x - o->w / 2,
                             pseek.y - o->h / 2,
                             o->w, o->h};

            if (!map_blocked(g, rseek)) {
                p2 = pseek;
                goto BREAKLOOP;
            }
        }
    }
BREAKLOOP:;

    v2_i32            arrival_player = steer_arrival(p1, v, p2, 900, 120);
    v2_i32            tile_avoid     = {0};
    rec_i32           rbounds        = {o->pos.x - 32, o->pos.y + 32, o->w + 64, o->h + 64};
    tile_map_bounds_s bounds         = tile_map_bounds_rec(g, rbounds);

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            if (0 == g->tiles[x + y * g->tiles_x].collision) continue;

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

bool32 companion_sit(g_s *g, obj_s *o, obj_s *ohero)
{
    companion_s *c  = (companion_s *)o->mem;
    v2_i32       p1 = obj_pos_center(o);
    v2_i32       p2 = obj_pos_center(ohero);
    v2_i32       v  = v2_i32_from_i16(o->v_q8);

    v2_i32 place_sit = {0};
    bool32 can_sit   = 0;

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

    if (ds < 10) {
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
        v2_i32 arrival = steer_arrival(p1, v, place_sit, 600, 120);
        o->v_q8.x += arrival.x;
        o->v_q8.y += arrival.y;
        if (c->sit) {
            c->sit       = 0;
            o->animation = 0;
        }
    }

    return 1;
}