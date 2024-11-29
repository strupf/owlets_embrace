// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// Crawls along the surface of solid blocks and moving platforms

#include "game.h"

enum {
    CRAWLER_STATE_FALLING,
    CRAWLER_STATE_CRAWLING,
};

enum {
    CRAWLER_SUBSTATE_NORMAL,
};

static i32 crawler_find_crawl_direction(g_s *g, obj_s *o, i32 dir);

typedef struct {
    i32 bounce_rotation_q12; // in turns
    i32 bounce_angle_q12;    // in turns
} obj_crawler_s;

static bool32 crawler_can_crawl(g_s *g, obj_s *o, rec_i32 aabb, i32 dir)
{
    v2_i32  cp = direction_v2(dir);
    rec_i32 rr = translate_rec(aabb, cp);
    if (!map_traversable(g, rr)) return 0;         // direction blocked
    rec_i32 r1 = {rr.x - 1, rr.y, rr.w + 2, rr.h}; // check if there is a solid surface on the side
    rec_i32 r2 = {rr.x, rr.y - 1, rr.w, rr.h + 2};
    return !(map_traversable(g, r1) && map_traversable(g, r2));
}

static i32 crawler_find_crawl_direction(g_s *g, obj_s *o, i32 dir)
{
    // const rec_i32 rbounds = {o->pos.x - 2, o->pos.y - 2, o->w + 4, o->h + 4};
    const rec_i32 rbounds = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};
    if (map_traversable(g, rbounds)) // check if any surface nearby
        return 0;

    i32           dr   = rngr_i32(1, 8);
    i32           d    = dir == 0 ? dr : dir; // choose a random preferred direction if not currently crawling
    const rec_i32 aabb = obj_aabb(o);

    if (crawler_can_crawl(g, o, aabb, d)) // try crawling in the same direction
        return d;

    i32 dir_a = d; // otherwise: try crawling in the next nearest directions
    i32 dir_b = d;
    for (i32 k = 0; k < 4; k++) {
        dir_a = direction_nearest(dir_a, 0);
        dir_b = direction_nearest(dir_b, 1);
        if (crawler_can_crawl(g, o, aabb, dir_a))
            return dir_a;
        if (crawler_can_crawl(g, o, aabb, dir_b))
            return dir_b;
    }
    return 0;
}

static void crawler_do_normal(g_s *g, obj_s *o)
{
    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;

    o->subtimer++;
    o->timer++;
    bool32 domove = 1;
    switch (o->ID) { // movement speed
    case OBJ_ID_CRAWLER:
        if (o->timer & 1) domove = 0;
        break;
    case OBJ_ID_CRAWLER_CATERPILLAR:
        if (o->timer & 3) domove = 0;
        break;
    default: break;
    }

    if (!domove) return;

    // returns a direction if still sticking to a wall
    i32 dir_to_crawl = crawler_find_crawl_direction(g, o, o->action);
    o->action        = dir_to_crawl;
    if (dir_to_crawl) { // glued to wall
        o->state = CRAWLER_STATE_CRAWLING;
        o->flags &= ~OBJ_FLAG_MOVER; // disable physics movement

        if (domove && dir_to_crawl) {
            o->pos = v2_add(o->pos, direction_v2(dir_to_crawl));
        }
    } else { // free fall
        o->state = CRAWLER_STATE_FALLING;
        o->flags |= OBJ_FLAG_MOVER; // enable gravity and physics
    }
}

// Walk in a straight path along the wall. If it bumps against
// a wall or just stepped into empty space it'll try to change directions
// to continue crawling.
void crawler_on_update(g_s *g, obj_s *o)
{
    crawler_do_normal(g, o);
}

void crawler_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s  *spr     = &o->sprites[0];
    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;
    spr->flip              = 0;
    i32 imgy               = 0;

    if (o->state == CRAWLER_STATE_CRAWLING) {
        switch (o->action) {
        case DIRECTION_S: {
            imgy = 2;
            if (!map_traversable(g, obj_rec_right(o)))
                spr->flip = SPR_FLIP_X;
        } break;
        case DIRECTION_N: {
            imgy = 6;
            if (!map_traversable(g, obj_rec_left(o)))
                spr->flip = SPR_FLIP_X;
        } break;
        case DIRECTION_E: {
            if (!map_traversable(g, obj_rec_top(o))) {
                imgy      = 4;
                spr->flip = SPR_FLIP_X;
            } else {
                imgy = 0;
            }
        } break;
        case DIRECTION_W: {
            if (!map_traversable(g, obj_rec_bottom(o))) {
                imgy      = 0;
                spr->flip = SPR_FLIP_X;
            } else {
                imgy = 4;
            }
        } break;
        case DIRECTION_SE: {
            rec_i32 rr = {o->pos.x - 1, o->pos.y, o->w + 1, o->h + 1};
            if (!map_traversable(g, rr)) {
                imgy = 1;
            } else {
                imgy      = 3;
                spr->flip = SPR_FLIP_X;
            }
        } break;
        case DIRECTION_SW: {
            rec_i32 rr = {o->pos.x - 1, o->pos.y - 1, o->w + 1, o->h + 1};
            if (!map_traversable(g, rr)) {
                imgy = 3;
            } else {
                imgy      = 1;
                spr->flip = SPR_FLIP_X;
            }
        } break;
        case DIRECTION_NE: {
            rec_i32 rr = {o->pos.x, o->pos.y, o->w + 1, o->h + 1};
            if (!map_traversable(g, rr)) {
                imgy = 7;
            } else {
                imgy      = 5;
                spr->flip = SPR_FLIP_X;
            }
        } break;
        case DIRECTION_NW: {
            rec_i32 rr = {o->pos.x, o->pos.y - 1, o->w + 1, o->h + 1};
            if (!map_traversable(g, rr)) {
                imgy = 5;
            } else {
                imgy      = 7;
                spr->flip = SPR_FLIP_X;
            }
        } break;
        }
    } else {
        crawler->bounce_angle_q12 += crawler->bounce_rotation_q12;
        imgy = (crawler->bounce_angle_q12 >> 9) & 7;
    }

    if (o->ID == OBJ_ID_CRAWLER_CATERPILLAR) {
        spr->trec.r.x = 64 * ((o->subtimer / 18) & 3) + 256;
    } else {
        spr->trec.r.x = 64 * ((o->subtimer >> 3) & 1);
    }

    spr->trec.r.y = imgy * 64;
}

static void crawler_load_i(g_s *g, map_obj_s *mo, i32 ID)
{
    assert(ID == OBJ_ID_CRAWLER || ID == OBJ_ID_CRAWLER_CATERPILLAR);
    obj_s *o = obj_create(g);
    o->ID    = ID;
    o->flags = OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY;
    o->render_priority = 1;
    o->w               = 15;
    o->h               = 15;
    o->health_max      = ID == OBJ_ID_CRAWLER ? 2 : 1;
    o->health          = o->health_max;
    o->enemy           = enemy_default();
    obj_sprite_s *spr  = &o->sprites[0];
    o->n_sprites       = 1;
    spr->trec          = asset_texrec(TEXID_CRAWLER, 0, 0, 64, 64);
    spr->offs.x        = o->w / 2 - 32;
    spr->offs.y        = o->h / 2 - 48 + 10;

    // difference between tilesize and object dimension
    for (i32 y = 0; y <= 1; y++) {
        for (i32 x = 0; x <= 1; x++) {
            o->pos.x = mo->x + x;
            o->pos.y = mo->y + y;
            i32 dir  = crawler_find_crawl_direction(g, o, 0);
            if (dir) {
                o->state  = CRAWLER_STATE_CRAWLING;
                o->action = dir;
                return;
            }
        }
    }
}

void crawler_load(g_s *g, map_obj_s *mo)
{
    crawler_load_i(g, mo, OBJ_ID_CRAWLER);
}

void crawler_caterpillar_load(g_s *g, map_obj_s *mo)
{
    crawler_load_i(g, mo, OBJ_ID_CRAWLER_CATERPILLAR);
}

void crawler_on_weapon_hit(g_s *g, obj_s *o, hitbox_s hb)
{
    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;
    o->flags |= OBJ_FLAG_MOVER;
    o->v_q8.y                    = (hb.force_q8.y * 800) >> 8;
    o->v_q8.x                    = (hb.force_q8.x * 1000) >> 8;
    crawler->bounce_rotation_q12 = rngr_sym_i32(2000);
}