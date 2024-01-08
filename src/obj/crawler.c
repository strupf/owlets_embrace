// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// Crawls along the surface of solid blocks and moving platforms

#include "game.h"

#define CRAWLER_TICKS_BOUNCING     200  // minimum time bouncing
#define CRAWLER_ACTION_HISTORY     4    // number of actions for angle smoothing
#define CRAWLER_DISTSQ_CURL        3000 // distance^2 to player to curl
#define CRAWLER_DISTSQ_UNCURL      3000 // distance^2 to player to uncurl
#define CRAWLER_TICKS_HURT_CURLING 30   // ticks when crawler is still killable while curling

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
    o->flags |= OBJ_FLAG_ACTOR |
                OBJ_FLAG_MOVER |
                OBJ_FLAG_KILL_OFFSCREEN;
    o->gravity_q8.y      = 30;
    o->drag_q8.y         = 255;
    o->drag_q8.x         = 255;
    o->w                 = 14;
    o->h                 = 14;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_CRAWLER, 0, 0, 64, 64);
    spr->offs.x          = -16;
    spr->offs.y          = -16;
    return o;
}

static void crawler_do_normal(game_s *g, obj_s *o)
{
    o->timer++;
    switch (o->ID) { // movement speed
    case OBJ_ID_CRAWLER:
        if (o->timer & 1) return;
        break;
    case OBJ_ID_CRAWLER_SNAIL:
        if (o->timer & 3) return;
        break;
    default: BAD_PATH
    }

    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;

    const rec_i32 r  = obj_aabb(o);
    const state_prev = o->action;
    bool32 newdir    = 0;
    switch (o->action) {
    case DIRECTION_NONE: newdir = 1; break;
    case DIRECTION_N: {
        rec_i32 rside = {r.x - 1, r.y, r.w + 2, r.h};
        if (game_traversable(g, obj_rec_top(o)) && !game_traversable(g, rside))
            break;
        newdir = 1;
    } break;
    case DIRECTION_S: {
        rec_i32 rside = {r.x - 1, r.y, r.w + 2, r.h};
        if (game_traversable(g, obj_rec_bottom(o)) && !game_traversable(g, rside))
            break;
        newdir = 1;
    } break;
    case DIRECTION_E: {
        rec_i32 rside = {r.x, r.y - 1, r.w, r.h + 2};
        if (game_traversable(g, obj_rec_right(o)) && !game_traversable(g, rside))
            break;
        newdir = 1;
    } break;
    case DIRECTION_W: {
        rec_i32 rside = {r.x, r.y - 1, r.w, r.h + 2};
        if (game_traversable(g, obj_rec_left(o)) && !game_traversable(g, rside))
            break;
        newdir = 1;
    } break;
    }

    if (newdir) {
        // look for a new crawl direction
        o->action = crawler_find_new_dir(g, o, o->action); // find new direction
        if (o->action == DIRECTION_NONE)
            o->action = crawler_find_new_dir(g, o, DIRECTION_NONE); // no new direction, now allow going back
    }

    if (o->action == DIRECTION_NONE) {
        // free fall
        o->state    = CRAWLER_STATE_FALLING;
        o->substate = CRAWLER_SUBSTATE_CURLED;
        o->flags |= OBJ_FLAG_MOVER; // enable gravity and physics
        switch (state_prev) {
        case DIRECTION_N: o->vel_q8.y = -500; break;
        case DIRECTION_S: o->vel_q8.y = +500; break;
        case DIRECTION_W: o->vel_q8.x = -500; break;
        case DIRECTION_E: o->vel_q8.x = +500; break;
        }
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
                o->timer    = 0;
            }
        } break;
        case CRAWLER_SUBSTATE_CURLED: {
            o->timer++;
            if (CRAWLER_DISTSQ_UNCURL < distsq) {
                o->substate = CRAWLER_SUBSTATE_NORMAL;
            }
        } break;
        }

        // if curled: don't move
        if (o->substate == CRAWLER_SUBSTATE_CURLED) {
            return;
        }
    }

    switch (o->action) {
    case DIRECTION_N: o->tomove.y = -1; break;
    case DIRECTION_S: o->tomove.y = +1; break;
    case DIRECTION_W: o->tomove.x = -1; break;
    case DIRECTION_E: o->tomove.x = +1; break;
    }

    // crawl history -> average rotation angle
    crawler->crawl_cur                         = o->action;
    crawler->crawl_history[crawler->n_crawl++] = o->action;
    crawler->n_crawl %= CRAWLER_ACTION_HISTORY;
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

    switch (o->state) {
    case CRAWLER_STATE_FALLING:
    case CRAWLER_STATE_BOUNCING:
        break;
    case CRAWLER_STATE_CRAWLING:
        break;
    }
}

void crawler_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    sprite_simple_s *spr     = &o->sprites[0];
    obj_crawler_s   *crawler = (obj_crawler_s *)o->mem;

    f32 ang        = 0;
    int is_flipped = 0;

    if (o->state == CRAWLER_STATE_CRAWLING) {
        // calculate rotation angle based on crawl history
        // have to average unit vector to get a correct result
        int ax = 0;
        int ay = 0;
        for (int i = 0; i < CRAWLER_ACTION_HISTORY; i++) {
            switch (crawler->crawl_history[i]) {
            case DIRECTION_E: ax++; break;
            case DIRECTION_N: ay++; break;
            case DIRECTION_W: ax--; break;
            case DIRECTION_S: ay--; break;
            }
        }

        switch (crawler->crawl_cur) {
        case DIRECTION_E: {
            rec_i32 r1 = {o->pos.x - 1, o->pos.y - 1, o->w + 2, 1};
            is_flipped = !game_traversable(g, r1);
        } break;
        case DIRECTION_N: {
            rec_i32 r1 = {o->pos.x - 1, o->pos.y - 1, 1, o->h + 2};
            is_flipped = !game_traversable(g, r1);
        } break;
        case DIRECTION_W: {
            rec_i32 r1 = {o->pos.x - 1, o->pos.y - 1, o->w + 2, 1};
            is_flipped = game_traversable(g, r1);
        } break;
        case DIRECTION_S: {
            rec_i32 r1 = {o->pos.x - 1, o->pos.y - 1, 1, o->h + 2};
            is_flipped = game_traversable(g, r1);
        } break;
        }

        ang = atan2_f(is_flipped ? -(f32)ay : +(f32)ay, (f32)ax);
    } else {
        if (o->state == CRAWLER_STATE_BOUNCING) {
            crawler->bounce_angle += crawler->bounce_rotation;
            ang = crawler->bounce_angle;
        }
    }

    gfx_ctx_s ctx     = gfx_ctx_display();
    f32       scly    = is_flipped ? -1.f : 1.f;
    v2_i32    pcenter = obj_pos_center(o);
    v2_i32    pos     = v2_add(pcenter, cam);
    pos.x -= 32;
    pos.y -= 48 - 10;
    v2_i32 origin = {32, 48 - 10};

    spr->trec.r.x = 64 * ((g->tick / 20) % 2);
    if (o->substate == CRAWLER_SUBSTATE_CURLED) {
        spr->trec.r.x = 64 * 2;
    }
    gfx_spr_affine(ctx, spr->trec, pos, origin, ang, 1.f, scly);
}

void crawler_on_weapon_hit(obj_s *o, hitbox_s hb)
{
    obj_crawler_s *crawler = (obj_crawler_s *)o->mem;

    bool32 can_be_hurt = !(o->substate == CRAWLER_SUBSTATE_CURLED &&
                           CRAWLER_TICKS_HURT_CURLING <= o->timer);

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
        if (game_traversable(g, obj_rec_top(o))) {
            rec_i32 rs = {r.x - 1, r.y - 1, r.w + 2, r.h + 1};
            if (!game_traversable(g, rs))
                return DIRECTION_N;
        }
    }
    if (dir_prev != DIRECTION_N) {
        if (game_traversable(g, obj_rec_bottom(o))) {
            rec_i32 rs = {r.x - 1, r.y, r.w + 2, r.h + 1};
            if (!game_traversable(g, rs))
                return DIRECTION_S;
        }
    }
    if (dir_prev != DIRECTION_W) {
        if (game_traversable(g, obj_rec_right(o))) {
            rec_i32 rs = {r.x, r.y - 1, r.w + 1, r.h + 2};
            if (!game_traversable(g, rs))
                return DIRECTION_E;
        }
    }
    if (dir_prev != DIRECTION_E) {
        if (game_traversable(g, obj_rec_left(o))) {
            rec_i32 rs = {r.x - 1, r.y - 1, r.w + 1, r.h + 2};
            if (!game_traversable(g, rs))
                return DIRECTION_W;
        }
    }

    return DIRECTION_NONE;
}