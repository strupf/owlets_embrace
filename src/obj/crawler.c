// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// Crawls along the surface of solid blocks and moving platforms

#include "game.h"

#define CRAWLER_TICKS_BOUNCING 200  // minimum time bouncing
#define CRAWLER_DISTSQ_CURL    2000 // distance^2 to player to curl
#define CRAWLER_DISTSQ_UNCURL  2000 // distance^2 to player to uncurl
#define CRAWLER_TICKS_TO_CURL  25   // ticks when crawler is still killable while curling

enum {
    CRAWLER_STATE_FALLING,
    CRAWLER_STATE_CRAWLING,
    CRAWLER_STATE_BOUNCING,
};

enum {
    CRAWLER_SUBSTATE_NORMAL,
    CRAWLER_SUBSTATE_CURLED,
};

static int crawler_find_crawl_direction(game_s *g, obj_s *o, int dir);

typedef struct {
    i32 bounce_rotation_q12; // in turns
    i32 bounce_angle_q12;    // in turns
} obj_crawler_s;

static bool32 crawler_can_crawl(game_s *g, obj_s *o, rec_i32 aabb, int dir)
{
    v2_i32  cp = direction_v2(dir);
    rec_i32 rr = translate_rec(aabb, cp);
    if (!game_traversable(g, rr)) return 0;        // direction blocked
    rec_i32 r1 = {rr.x - 1, rr.y, rr.w + 2, rr.h}; // check if there is a solid surface on the side
    rec_i32 r2 = {rr.x, rr.y - 1, rr.w, rr.h + 2};
    return !(game_traversable(g, r1) && game_traversable(g, r2));
}

static int crawler_find_crawl_direction(game_s *g, obj_s *o, int dir)
{
    const rec_i32 rbounds = {o->pos.x - 2, o->pos.y - 2, o->w + 4, o->h + 4};
    if (game_traversable(g, rbounds)) // check if any surface nearby
        return 0;

    int           dr   = rngr_i32(1, 8);
    int           d    = dir == 0 ? dr : dir; // choose a random preferred direction if not currently crawling
    const rec_i32 aabb = obj_aabb(o);

    if (crawler_can_crawl(g, o, aabb, d)) // try crawling in the same direction
        return d;

    int dir_a = d; // otherwise: try crawling in the next nearest directions
    int dir_b = d;
    for (int k = 0; k < 4; k++) {
        dir_a = direction_nearest(dir_a, 0);
        dir_b = direction_nearest(dir_b, 1);
        if (crawler_can_crawl(g, o, aabb, dir_a))
            return dir_a;
        if (crawler_can_crawl(g, o, aabb, dir_b))
            return dir_b;
    }
    return 0;
}

static void crawler_do_normal(game_s *g, obj_s *o)
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

    // returns a direction if still sticking to a wall
    int dir_to_crawl = crawler_find_crawl_direction(g, o, o->action);
    o->action        = dir_to_crawl;

    if (dir_to_crawl) { // glued to wall
        o->state = CRAWLER_STATE_CRAWLING;
        o->flags &= ~OBJ_FLAG_MOVER; // disable physics movement

        if (o->ID == OBJ_ID_CRAWLER) {
            obj_s *ohero  = obj_get_tagged(g, OBJ_TAG_HERO);
            u32    distsq = U32_MAX;
            if (ohero) {
                distsq = v2_distancesq(obj_pos_center(o), obj_pos_center(ohero));
            }

            switch (o->substate) {
            case CRAWLER_SUBSTATE_NORMAL: {
                if (distsq <= CRAWLER_DISTSQ_CURL) {
                    o->substate = CRAWLER_SUBSTATE_CURLED;
                    o->subtimer = 0;
                }
            } break;
            case CRAWLER_SUBSTATE_CURLED: {
                if (CRAWLER_DISTSQ_UNCURL < distsq) {
                    o->substate = CRAWLER_SUBSTATE_NORMAL;
                    o->subtimer = 0;
                }
            } break;
            }

#if 0
            bool32 can_be_hurt      = !((o->substate == CRAWLER_SUBSTATE_CURLED &&
                                    CRAWLER_TICKS_TO_CURL <= o->subtimer));
            o->enemy.cannot_be_hurt = !can_be_hurt;
#endif

            // if curled: don't move
            if (o->substate == CRAWLER_SUBSTATE_CURLED || o->subtimer < CRAWLER_TICKS_TO_CURL) {
                domove = 0;
            }
        }

        if (domove && dir_to_crawl) {
            o->pos = v2_add(o->pos, direction_v2(dir_to_crawl));
        }
    } else { // free fall
        o->state    = CRAWLER_STATE_FALLING;
        o->substate = CRAWLER_SUBSTATE_CURLED;
        o->flags |= OBJ_FLAG_MOVER; // enable gravity and physics
    }
}

static void crawler_do_bounce(game_s *g, obj_s *o)
{
    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;
    o->timer--;
    i32 mx = (o->timer <= 0 ? 150 : 255);
    i32 my = (o->timer <= 0 ? 150 : 255);

    if (o->timer <= 0) {
        o->drag_q8.x = 250;
    }

    if (o->bumpflags & OBJ_BUMPED_X) {
        o->vel_q8.x = -(o->vel_q8.x * mx) >> 8;
    }
    if (o->bumpflags & OBJ_BUMPED_Y) {
        o->vel_q8.y = -(o->vel_q8.y * my) >> 8;
    }
    if (o->bumpflags & (OBJ_BUMPED_X | OBJ_BUMPED_Y)) {
        crawler->bounce_rotation_q12 = rngr_sym_i32(2000);
    }
    o->bumpflags = 0;

    // try to return to crawling if not bouncing too much
    if (o->timer < 0 && abs_i(o->vel_q8.x) < 100 && abs_i(o->vel_q8.y) < 5) {
        o->timer                     = 0;
        o->state                     = CRAWLER_STATE_FALLING;
        o->substate                  = CRAWLER_SUBSTATE_CURLED;
        o->action                    = DIRECTION_NONE;
        crawler->bounce_rotation_q12 = 0;
        crawler->bounce_angle_q12    = 0;
    }
}

// Walk in a straight path along the wall. If it bumps against
// a wall or just stepped into empty space it'll try to change directions
// to continue crawling.
void crawler_on_update(game_s *g, obj_s *o)
{
#if 0
    o->enemy.cannot_be_hurt = 0;
    o->drag_q8.y            = 255;
    o->drag_q8.x            = 255;

    if (o->state == CRAWLER_STATE_BOUNCING) {
        o->enemy.cannot_be_hurt = 1;
        crawler_do_bounce(g, o);
    } else {
        crawler_do_normal(g, o);
    }
#endif
}

void crawler_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *spr     = &o->sprites[0];
    obj_crawler_s   *crawler = (obj_crawler_s *)o->mem;
    spr->flip                = 0;
    int imgy                 = 0;

    if (o->state == CRAWLER_STATE_CRAWLING) {
        switch (o->action) {
        case DIRECTION_S: {
            imgy = 2;
            if (!game_traversable(g, obj_rec_right(o)))
                spr->flip = SPR_FLIP_X;
        } break;
        case DIRECTION_N: {
            imgy = 6;
            if (!game_traversable(g, obj_rec_left(o)))
                spr->flip = SPR_FLIP_X;
        } break;
        case DIRECTION_E: {
            if (!game_traversable(g, obj_rec_top(o))) {
                imgy      = 4;
                spr->flip = SPR_FLIP_X;
            } else {
                imgy = 0;
            }
        } break;
        case DIRECTION_W: {
            if (!game_traversable(g, obj_rec_bottom(o))) {
                imgy      = 0;
                spr->flip = SPR_FLIP_X;
            } else {
                imgy = 4;
            }
        } break;
        case DIRECTION_SE: {
            rec_i32 rr = {o->pos.x - 1, o->pos.y, o->w + 1, o->h + 1};
            if (!game_traversable(g, rr)) {
                imgy = 1;
            } else {
                imgy      = 3;
                spr->flip = SPR_FLIP_X;
            }
        } break;
        case DIRECTION_SW: {
            rec_i32 rr = {o->pos.x - 1, o->pos.y - 1, o->w + 1, o->h + 1};
            if (!game_traversable(g, rr)) {
                imgy = 3;
            } else {
                imgy      = 1;
                spr->flip = SPR_FLIP_X;
            }
        } break;
        case DIRECTION_NE: {
            rec_i32 rr = {o->pos.x, o->pos.y, o->w + 1, o->h + 1};
            if (!game_traversable(g, rr)) {
                imgy = 7;
            } else {
                imgy      = 5;
                spr->flip = SPR_FLIP_X;
            }
        } break;
        case DIRECTION_NW: {
            rec_i32 rr = {o->pos.x, o->pos.y - 1, o->w + 1, o->h + 1};
            if (!game_traversable(g, rr)) {
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

    if (o->ID == OBJ_ID_CRAWLER && o->substate == CRAWLER_SUBSTATE_CURLED) {
        if (o->subtimer < CRAWLER_TICKS_TO_CURL) {
            spr->trec.r.x = 2 * 64;
        } else {
            spr->trec.r.x = 3 * 64;
        }
    }

    spr->trec.r.y = imgy * 64;
}

static void crawler_load_i(game_s *g, map_obj_s *mo, int ID)
{
    assert(ID == OBJ_ID_CRAWLER || ID == OBJ_ID_CRAWLER_CATERPILLAR);
    obj_s *o = obj_create(g);
    o->ID    = ID;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_SPRITE |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ENEMY;
    o->on_update         = crawler_on_update;
    o->on_animate        = crawler_on_animate;
    o->render_priority   = 1;
    o->gravity_q8.y      = 30;
    o->drag_q8.y         = 255;
    o->drag_q8.x         = 255;
    o->w                 = 15;
    o->h                 = 15;
    o->health_max        = ID == OBJ_ID_CRAWLER ? 2 : 1;
    o->health            = o->health_max;
    o->enemy             = enemy_default();
    sprite_simple_s *spr = &o->sprites[0];
    o->n_sprites         = 1;
    spr->trec            = asset_texrec(TEXID_CRAWLER, 0, 0, 64, 64);
    spr->offs.x          = o->w / 2 - 32;
    spr->offs.y          = o->h / 2 - 48 + 10;

    // difference between tilesize and object dimension
    for (int y = 0; y <= 1; y++) {
        for (int x = 0; x <= 1; x++) {
            o->pos.x = mo->x + x - mo->w / 2;
            o->pos.y = mo->y + y - mo->h;
            if (crawler_find_crawl_direction(g, o, 0)) {
                return;
            }
        }
    }
}

void crawler_load(game_s *g, map_obj_s *mo)
{
    crawler_load_i(g, mo, OBJ_ID_CRAWLER);
}

void crawler_caterpillar_load(game_s *g, map_obj_s *mo)
{
    crawler_load_i(g, mo, OBJ_ID_CRAWLER_CATERPILLAR);
}

void crawler_on_weapon_hit(game_s *g, obj_s *o, hitbox_s hb)
{
    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;
    o->flags |= OBJ_FLAG_MOVER;
    o->state                     = CRAWLER_STATE_BOUNCING;
    o->substate                  = CRAWLER_SUBSTATE_CURLED;
    o->timer                     = CRAWLER_TICKS_BOUNCING;
    o->vel_q8.y                  = (hb.force_q8.y * 800) >> 8;
    o->vel_q8.x                  = (hb.force_q8.x * 1000) >> 8;
    o->drag_q8.y                 = 255;
    o->drag_q8.x                 = 255;
    crawler->bounce_rotation_q12 = rngr_sym_i32(2000);
}