// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

// Crawls along the surface of solid blocks and moving platforms

#include "game.h"

enum {
    CRAWLER_MOVED_NONE,
    CRAWLER_MOVED_N,
    CRAWLER_MOVED_S,
    CRAWLER_MOVED_W,
    CRAWLER_MOVED_E,
};

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
    o->flags |= OBJ_FLAG_ACTOR;
    o->flags |= OBJ_FLAG_MOVER;
    o->gravity_q8.y = 30;
    o->drag_q8.y    = 255;
    o->drag_q8.x    = 255;
    o->w            = 15;
    o->h            = 15;

    return o;
}

// Walk in a straight path along the wall. If it bumps against
// a wall or just stepped into empty space it'll try to change directions
// to continue crawling.
void crawler_on_update(game_s *g, obj_s *o)
{
    const rec_i32 r  = obj_aabb(o);
    const state_prev = o->substate;

    bool32 newdir = 0;
    switch (o->substate) {
    case CRAWLER_MOVED_NONE: newdir = 1; break;
    case CRAWLER_MOVED_N: {
        rec_i32 rside = {r.x - 1, r.y, r.w + 2, r.h};
        if (!(game_traversable(g, obj_rec_top(o)) &&
              !game_traversable(g, rside))) {
            newdir = 1;
        }
    } break;
    case CRAWLER_MOVED_S: {
        rec_i32 rside = {r.x - 1, r.y, r.w + 2, r.h};
        if (!(game_traversable(g, obj_rec_bottom(o)) &&
              !game_traversable(g, rside))) {
            newdir = 1;
        }
    } break;
    case CRAWLER_MOVED_E: {
        rec_i32 rside = {r.x, r.y - 1, r.w, r.h + 2};
        if (!(game_traversable(g, obj_rec_right(o)) &&
              !game_traversable(g, rside))) {
            newdir = 1;
        }
    } break;
    case CRAWLER_MOVED_W: {
        rec_i32 rside = {r.x, r.y - 1, r.w, r.h + 2};
        if (!(game_traversable(g, obj_rec_left(o)) &&
              !game_traversable(g, rside))) {
            newdir = 1;
        }
    } break;
    }

    if (newdir) {
        // look for a new crawl direction
        o->substate = crawler_find_new_dir(g, o, o->substate); // find new direction
        if (o->substate == CRAWLER_MOVED_NONE) {
            o->substate = crawler_find_new_dir(g, o, CRAWLER_MOVED_NONE); // no new direction, now allow going back
        }
    }

    if (o->substate == CRAWLER_MOVED_NONE) {
        // free fall
        o->flags |= OBJ_FLAG_MOVER; // enable gravity and physics
        o->vel_q8.x = 0;
        switch (state_prev) {
        case CRAWLER_MOVED_N: o->vel_q8.y = -1000; break;
        case CRAWLER_MOVED_S: o->vel_q8.y = +1000; break;
        case CRAWLER_MOVED_W: o->vel_q8.x = -1000; break;
        case CRAWLER_MOVED_E: o->vel_q8.x = +1000; break;
        }
    } else {
        // glued to wall
        o->flags &= ~OBJ_FLAG_MOVER; // disable physics movement
        o->timer++;
        if ((o->timer & 1) == 0 || o->substate != state_prev) {
            switch (o->substate) {
            case CRAWLER_MOVED_N: o->tomove.y = -1; break;
            case CRAWLER_MOVED_S: o->tomove.y = +1; break;
            case CRAWLER_MOVED_W: o->tomove.x = -1; break;
            case CRAWLER_MOVED_E: o->tomove.x = +1; break;
            }
        }
    }
}

void crawler_on_animate(game_s *g, obj_s *o)
{
}

static int crawler_find_new_dir(game_s *g, obj_s *o, int dir_prev)
{
    const rec_i32 r = obj_aabb(o);

    if (dir_prev != CRAWLER_MOVED_N) {
        if (game_traversable(g, obj_rec_bottom(o))) {
            rec_i32 rs = {r.x - 1, r.y, r.w + 2, r.h + 1};
            if (!game_traversable(g, rs)) {
                return CRAWLER_MOVED_S;
            }
        }
    }
    if (dir_prev != CRAWLER_MOVED_S) {
        if (game_traversable(g, obj_rec_top(o))) {
            rec_i32 rs = {r.x - 1, r.y - 1, r.w + 2, r.h + 1};
            if (!game_traversable(g, rs)) {
                return CRAWLER_MOVED_N;
            }
        }
    }
    if (dir_prev != CRAWLER_MOVED_E) {
        if (game_traversable(g, obj_rec_left(o))) {
            rec_i32 rs = {r.x - 1, r.y - 1, r.w + 1, r.h + 2};
            if (!game_traversable(g, rs)) {
                return CRAWLER_MOVED_W;
            }
        }
    }
    if (dir_prev != CRAWLER_MOVED_W) {
        if (game_traversable(g, obj_rec_right(o))) {
            rec_i32 rs = {r.x, r.y - 1, r.w + 1, r.h + 2};
            if (!game_traversable(g, rs)) {
                return CRAWLER_MOVED_E;
            }
        }
    }
    return CRAWLER_MOVED_NONE;
}