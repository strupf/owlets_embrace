// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// Crawls along the surface of solid blocks and moving platforms

#include "game.h"

#define CRAWLER_TICKS_BOUNCING 200  // minimum time bouncing
#define CRAWLER_ACTION_HISTORY 2    // number of actions for angle smoothing
#define CRAWLER_DISTSQ_CURL    3000 // distance^2 to player to curl
#define CRAWLER_DISTSQ_UNCURL  3000 // distance^2 to player to uncurl
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

typedef struct {
    int crawl_cur;
    f32 bounce_rotation;
    f32 bounce_angle;
    int n_crawl;
    u8  crawl_history[CRAWLER_ACTION_HISTORY];
} obj_crawler_s;

// Look for new surface to crawl along
// 1) Don't go right back to where it came from
// 2) Need enough space in new direction
// 3) Need a surface on the side to walk along
// Returns NSWE or none. Pass None to allow going right back.
static int crawler_find_new_dir(game_s *g, obj_s *o, int dir_prev);

obj_s *crawler_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_CRAWLER;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_MOVER |
               OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_SPRITE;
    o->gravity_q8.y      = 30;
    o->drag_q8.y         = 255;
    o->drag_q8.x         = 255;
    o->w                 = 14;
    o->h                 = 14;
    sprite_simple_s *spr = &o->sprites[0];
    o->n_sprites         = 1;
    spr->trec            = asset_texrec(TEXID_CRAWLER, 0, 0, 64, 64);
    spr->offs.x          = o->w / 2 - 32;
    spr->offs.y          = o->h / 2 - 48 + 10;

    sys_printf("REMINDER: Fix crawler sprite rotation\n");
    return o;
}

static bool32 crawler_can_crawl(game_s *g, obj_s *o, rec_i32 aabb, int dir)
{
    v2_i32  cp = direction_v2(dir);
    rec_i32 rr = translate_rec(aabb, cp);
    if (!game_traversable(g, rr)) return 0; // direction blocked
    // check if we also have a solid surface on the side
    rec_i32 r1 = {rr.x - 1, rr.y, rr.w + 2, rr.h};
    rec_i32 r2 = {rr.x, rr.y - 1, rr.w, rr.h + 2};
    return (!game_traversable(g, r1) || !game_traversable(g, r2));
}

static int crawler_find_crawl_direction(game_s *g, obj_s *o, int dir)
{
    // check if any surface nearby
    const rec_i32 rbounds = {o->pos.x - 2, o->pos.y - 2, o->w + 4, o->h + 4};
    if (game_traversable(g, rbounds))
        return 0;

    // choose a random preferred direction if not currently crawling
    int           d    = dir == 0 ? rngr_i32(1, 8) : dir;
    const rec_i32 aabb = obj_aabb(o);

    // try crawling in the same direction
    if (crawler_can_crawl(g, o, aabb, d))
        return d;

    // otherwise: try crawling in the next nearest directions
    int dir_a = d;
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
    int domove = 1;
    switch (o->ID) { // movement speed
    case OBJ_ID_CRAWLER:
        if (o->timer & 1) domove = 0;
        break;
    case OBJ_ID_CRAWLER_SNAIL:
        if (o->timer & 3) domove = 0;
        break;
    default: BAD_PATH
    }

    // returns a direction if still sticking to a wall
    int dir_to_crawl = crawler_find_crawl_direction(g, o, o->action);
    o->action        = dir_to_crawl;

    if (!dir_to_crawl) {
        // free fall
        o->state    = CRAWLER_STATE_FALLING;
        o->substate = CRAWLER_SUBSTATE_CURLED;
        o->flags |= OBJ_FLAG_MOVER; // enable gravity and physics
        crawler->crawl_cur = 0;
        memset(crawler->crawl_history, 0, sizeof(crawler->crawl_history));
        return;
    }

    // glued to wall
    o->state = CRAWLER_STATE_CRAWLING;
    o->flags &= ~OBJ_FLAG_MOVER; // disable physics movement

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        u32 distsq = v2_distancesq(obj_pos_center(o), obj_pos_center(ohero));

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
    }

    // if curled: don't move
    if (o->substate == CRAWLER_SUBSTATE_CURLED || o->subtimer < CRAWLER_TICKS_TO_CURL) {
        domove = 0;
    }

    if (domove && dir_to_crawl) {
        v2_i32 v = direction_v2(dir_to_crawl);
        o->pos   = v2_add(o->pos, v);

        // crawl history -> average rotation angle
        crawler->crawl_cur                         = o->action;
        crawler->crawl_history[crawler->n_crawl++] = o->action;
        crawler->n_crawl %= CRAWLER_ACTION_HISTORY;
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
        crawler->bounce_rotation = rngr_f32(-1.f, +1.f);
    }

    o->bumpflags = 0;

    if (0 < o->timer) return;

    // try to return to crawling if not bouncing too much
    if (abs_i(o->vel_q8.x) < 30 && abs_i(o->vel_q8.y) < 5) {
        o->timer    = 0;
        o->state    = CRAWLER_STATE_FALLING;
        o->substate = CRAWLER_SUBSTATE_NORMAL;
        o->action   = DIRECTION_NONE;
    }
}

// Walk in a straight path along the wall. If it bumps against
// a wall or just stepped into empty space it'll try to change directions
// to continue crawling.
void crawler_on_update(game_s *g, obj_s *o)
{
    o->drag_q8.y = 255;
    o->drag_q8.x = 255;

    if (o->state == CRAWLER_STATE_BOUNCING) {
        crawler_do_bounce(g, o);
    } else {
        crawler_do_normal(g, o);
    }
}

void crawler_on_animate(game_s *g, obj_s *o)
{
    sprite_simple_s *spr     = &o->sprites[0];
    obj_crawler_s   *crawler = (obj_crawler_s *)o->mem;

    int is_flipped = 0;
    int imgy       = 0;
    if (o->state == CRAWLER_STATE_CRAWLING) {
        // calculate rotation angle based on crawl history
        // have to average unit vector to get a correct result

    } else if (o->state == CRAWLER_STATE_BOUNCING) {
        crawler->bounce_angle += crawler->bounce_rotation;
        f32 ang = crawler->bounce_angle;
        imgy    = (int)((ang * 8.f) / PI2_FLOAT);
    }

    spr->trec.r.x = 64 * ((o->subtimer >> 3) & 1);
    if (o->substate == CRAWLER_SUBSTATE_CURLED) {
        if (o->subtimer < CRAWLER_TICKS_TO_CURL) {
            spr->trec.r.x = 2 * 64;
        } else {
            spr->trec.r.x = 3 * 64;
        }
    }

    spr->flip = 0;

    if (is_flipped) {
        spr->flip = SPR_FLIP_X;
        imgy += 4; // choose 180 deg rotated frame
    }
    spr->trec.r.y = (imgy & 7) * 64;
}

void crawler_on_weapon_hit(obj_s *o, hitbox_s hb)
{
    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;

    bool32 can_be_hurt = !(o->substate == CRAWLER_SUBSTATE_CURLED &&
                           CRAWLER_TICKS_TO_CURL <= o->subtimer);

    if (can_be_hurt) {
        // kill
    } else {
        o->state    = CRAWLER_STATE_BOUNCING;
        o->substate = CRAWLER_SUBSTATE_CURLED;
        o->timer    = CRAWLER_TICKS_BOUNCING;
        o->flags |= OBJ_FLAG_MOVER;
        o->vel_q8.y              = (hb.force_q8.y * 800) >> 8;
        o->vel_q8.x              = (hb.force_q8.x * 1000) >> 8;
        o->drag_q8.y             = 255;
        o->drag_q8.x             = 255;
        crawler->bounce_rotation = rngr_f32(-1.f, +1.f);
    }
}

static int crawler_find_new_dir(game_s *g, obj_s *o, int dir_prev)
{
    const rec_i32 r = obj_aabb(o);
    if (dir_prev != DIRECTION_S) {
        rec_i32 rr      = {o->pos.x, o->pos.y - 1, o->w, o->h};
        rec_i32 rr_side = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h};
        if (game_traversable(g, rr) && !game_traversable(g, rr_side)) {
            return DIRECTION_N;
        }
    }
    if (dir_prev != DIRECTION_N) {
        rec_i32 rr      = {o->pos.x, o->pos.y + 1, o->w, o->h};
        rec_i32 rr_side = {o->pos.x - 1, o->pos.y + 1, o->w + 2, o->h};
        if (game_traversable(g, rr) && !game_traversable(g, rr_side)) {
            return DIRECTION_S;
        }
    }
    if (dir_prev != DIRECTION_W) {
        rec_i32 rr      = {o->pos.x + 1, o->pos.y, o->w, o->h};
        rec_i32 rr_side = {o->pos.x + 1, o->pos.y - 1, o->w, o->h + 2};
        if (game_traversable(g, rr) && !game_traversable(g, rr_side)) {
            return DIRECTION_E;
        }
    }
    if (dir_prev != DIRECTION_E) {
        rec_i32 rr      = {o->pos.x - 1, o->pos.y, o->w, o->h};
        rec_i32 rr_side = {o->pos.x - 1, o->pos.y - 1, o->w, o->h + 2};
        if (game_traversable(g, rr) && !game_traversable(g, rr_side)) {
            return DIRECTION_W;
        }
    }

    return DIRECTION_NONE;
}