// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// Crawls along the surface of solid blocks and moving platforms

#include "game.h"

enum {
    CRAWLER_ST_FALLING,
    CRAWLER_ST_CRAWLING,
    CRAWLER_ST_DIE,
    CRAWLER_ST_HURT,
};

enum {
    CRAWLER_SUBSTATE_NORMAL,
};

void          crawler_on_hurt(g_s *g, obj_s *o);
void          crawler_on_update(g_s *g, obj_s *o);
void          crawler_on_animate(g_s *g, obj_s *o);
static void   crawler_image_on_wall(g_s *g, obj_s *o, i32 *imgy, i32 *flip);
static bool32 crawler_can_crawl_into_dir(g_s *g, obj_s *o, i32 dir);
static bool32 crawler_find_crawl_direction(g_s *g, obj_s *o, i32 *out_dir_feet, i32 *out_dir_move);
static bool32 crawler_try_start_crawling(g_s *g, obj_s *o);

typedef struct crawler_s {
    i32 dir_feet_prev;
    i32 dir_move_prev;
    i32 dir_feet; // dir feet and dir forward are orthogonal
    i32 dir_move;
    u32 seed;
} crawler_s;

void crawler_load(g_s *g, map_obj_s *mo)
{
    obj_s     *o = obj_create(g);
    crawler_s *c = (crawler_s *)o->mem;
    o->editorUID = mo->UID;
    o->ID        = OBJID_CRAWLER;
    o->flags     = OBJ_FLAG_KILL_OFFSCREEN |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_ACTOR;

    o->moverflags         = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->on_update          = crawler_on_update;
    o->on_animate         = crawler_on_animate;
    o->w                  = 20;
    o->h                  = 20;
    o->health_max         = 2;
    o->health             = o->health_max;
    o->enemy              = enemy_default();
    o->enemy.on_hurt      = crawler_on_hurt;
    o->enemy.hurt_on_jump = 1;
    c->seed               = 213;

    u8     dirs[4]     = {DIR_N, DIR_W, DIR_S, DIR_E};
    bool32 face_left   = map_obj_bool(mo, "face_left");
    bool32 start_crawl = 0;
    o->facing          = face_left ? -1 : +1;

    for (i32 n = 0; n < 4; n++) {
        v2_i32 v = dir_v2(dirs[n]);
        obj_place_to_map_obj(o, mo, v.x, v.y);
        if (crawler_try_start_crawling(g, o)) {
            start_crawl = 1;
            break;
        }
    }

    if (start_crawl) {
        assert(c->dir_move);
        o->state = CRAWLER_ST_CRAWLING;
    } else {
    }
}

// Walk in a straight path along the wall. If it bumps against
// a wall or just stepped into empty space it'll try to change directions
// to continue crawling.
void crawler_on_update(g_s *g, obj_s *o)
{
    crawler_s *c = (crawler_s *)o->mem;

    switch (o->state) {
    case CRAWLER_ST_HURT: {
        o->timer++;
        if (ENEMY_HIT_FREEZE_TICKS <= o->timer) {
            o->timer = 0;
            o->state = CRAWLER_ST_CRAWLING;

            if (crawler_try_start_crawling(g, o)) {
                o->state = CRAWLER_ST_CRAWLING;
            } else {
                o->state     = CRAWLER_ST_FALLING;
                o->bumpflags = 0;
            }
            o->timer   = 0;
            o->v_q12.x = 0;
            o->v_q12.y = 0;
        }
        break;
    }
    case CRAWLER_ST_FALLING: {
        o->timer++;

        if (o->bumpflags & OBJ_BUMP_X) {
            o->v_q12.x = -o->v_q12.x;
        }
        if (o->bumpflags & OBJ_BUMP_Y) {
            obj_vy_q8_mul(o, -160);
        }
        o->bumpflags = 0;
        o->v_q12.y += Q_VOBJ(0.35);
        o->v_q12.y = min_i32(o->v_q12.y, Q_VOBJ(6.0));

        obj_vx_q8_mul(o, 250);
        obj_move_by_v_q12(g, o);

        if (obj_grounded(g, o) && o->v_q12.y < Q_VOBJ(1.5) && crawler_try_start_crawling(g, o)) {
            o->subtimer++;
            o->v_q12.x = 0;
            o->v_q12.y = 0;

            if (20 <= o->subtimer) {
                o->state    = CRAWLER_ST_CRAWLING;
                o->timer    = 0;
                o->subtimer = 0;
            }
        } else {
            o->subtimer = 0;
        }
        break;
    }
    case CRAWLER_ST_CRAWLING: {
        o->subtimer++;
        o->timer++;

        i32    dir_move = 0;
        i32    dir_feet = 0;
        bool32 can_move = crawler_find_crawl_direction(g, o, &dir_feet, &dir_move);
        if (can_move) {
            if ((o->timer & 1) == 0) {
                c->dir_feet_prev = c->dir_feet;
                c->dir_move_prev = c->dir_move;
                c->dir_feet      = dir_feet;
                c->dir_move      = dir_move;
                v2_i32 tomove    = dir_v2(dir_move);

                // clear and set collisions for slopes
                o->moverflags &= ~OBJ_MOVER_TERRAIN_COLLISIONS;
                obj_move(g, o, tomove.x, tomove.y);
                o->moverflags |= OBJ_MOVER_TERRAIN_COLLISIONS;
            }
        } else {
            o->state     = CRAWLER_ST_FALLING;
            o->bumpflags = 0;
            o->timer     = 0;
            o->v_q12.x   = 0;
            o->v_q12.y   = 0;
        }

        break;
    }
    }
}

void crawler_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    crawler_s    *c   = (crawler_s *)o->mem;
    o->n_sprites      = 1;
    spr->flip         = 0;
    i32 imgx          = 0;
    i32 imgy          = 0;
    i32 flip          = 0;
    spr->offs.x       = o->w / 2 - 32;
    spr->offs.y       = o->h / 2 - 48 + 10;

    switch (o->state) {
    case CRAWLER_ST_DIE:
    case CRAWLER_ST_HURT: {
        crawler_image_on_wall(g, o, &imgy, &flip);
        spr->flip = flip;
        spr->offs.x += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        spr->offs.y += rngr_sym_i32(ENEMY_HIT_FREEZE_SHAKE_AMOUNT);
        imgx = 8;
        break;
    }
    case CRAWLER_ST_CRAWLING: {
        crawler_image_on_wall(g, o, &imgy, &flip);
        spr->flip = flip;
        imgx      = (o->subtimer >> 2) % 8;
        break;
    }
    case CRAWLER_ST_FALLING: {
        imgx = 9;
        break;
    }
    }

    spr->trec = asset_texrec(TEXID_CRAWLER, imgx * 64, imgy * 64, 64, 64);
}

void crawler_on_hurt(g_s *g, obj_s *o)
{
    if (o->health) {
        o->state = CRAWLER_ST_HURT;
    } else {
        o->state = CRAWLER_ST_DIE;
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        o->on_update = enemy_on_update_die;
        g->enemies_killed++;
        g->enemy_killed[ENEMYID_CRAWLER]++;
    }
    o->timer   = 0;
    o->v_q12.x = 0;
    o->v_q12.y = 0;
}

static bool32 crawler_find_crawl_direction(g_s *g, obj_s *o, i32 *out_dir_feet, i32 *out_dir_move)
{
    crawler_s *c      = (crawler_s *)o->mem;
    bool32     rot_cw = dir_nswe_90_deg(c->dir_move, 1) == c->dir_feet;
    i32        dm_tmp = c->dir_move;
    i32        dm_rev = 0;

    for (i32 k = 0; k < 8; k++) {
        if (crawler_can_crawl_into_dir(g, o, dm_tmp)) {
            if (dm_tmp == dir_nswe_opposite(c->dir_move)) { // could go back again, but actually moving forward is the goal
                dm_rev = dm_tmp;
            } else {
                *out_dir_move = dm_tmp;
                *out_dir_feet = dir_nswe_90_deg(dm_tmp, rot_cw);
                return 1;
            }
        }
        dm_tmp = dir_nswe_nearest(dm_tmp, rot_cw);
    }

    if (dm_rev) { // last resort: go back
        *out_dir_move = dm_rev;
        *out_dir_feet = dir_nswe_90_deg(dm_rev, rot_cw);
        return 1;
    }
    return 0;
}

static bool32 crawler_try_start_crawling(g_s *g, obj_s *o)
{
    crawler_s *crawler = (crawler_s *)o->mem;
    if (!crawler->dir_move) {
        crawler->dir_move = o->facing < 0 ? DIR_X_NEG : DIR_X_POS;
        crawler->dir_feet = DIR_Y_POS;
    }

    i32 dm = 0;
    i32 df = 0;
    if (crawler_find_crawl_direction(g, o, &df, &dm)) {
        crawler->dir_move = dm;
        crawler->dir_feet = df;
        return 1;
    } else {
        return 0;
    }
}

static bool32 crawler_can_crawl_into_dir(g_s *g, obj_s *o, i32 dir)
{
    v2_i32  cp = dir_v2(dir);
    rec_i32 rr = translate_rec(obj_aabb(o), cp.x, cp.y);
    if (map_blocked(g, rr)) return 0; // direction blocked

    rec_i32 r1 = {rr.x - 1, rr.y, rr.w + 2, rr.h}; // check if there is a solid surface on the side
    rec_i32 r2 = {rr.x, rr.y - 1, rr.w, rr.h + 2};
    return !(!map_blocked(g, r1) && !map_blocked(g, r2));
}

// calculated both prerotated sprite row and flip flags
static void crawler_image_on_wall(g_s *g, obj_s *o, i32 *imgy, i32 *flip)
{
    crawler_s *c = (crawler_s *)o->mem;

    i32 d1 = c->dir_move;
    i32 d2 = c->dir_feet;
    i32 d3 = c->dir_move_prev;
    i32 d4 = c->dir_feet_prev;
    b32 cw = dir_nswe_90_deg(c->dir_move, 1) == c->dir_feet;

    if (d1 && d3) { // use inbetween frame if changing direction orthogonally
        b32 same_dir = (cw == (b32)(dir_nswe_90_deg(d3, 1) == d4));
        b32 orth_cw  = dir_nswe_90_deg(d1, 1) == d3;
        b32 orth_ccw = dir_nswe_90_deg(d1, 0) == d3;

        v2_i32 v1 = dir_v2(d1);
        if ((orth_cw || orth_ccw) && same_dir && v1.x * v1.y == 0) {
            d1 = dir_nswe_nearest(d1, orth_cw);
        }
    }

    switch (d1) {
    case DIR_S:
        *imgy = 2;
        if (!cw) {
            *flip = SPR_FLIP_X;
        }
        break;
    case DIR_N:
        *imgy = 6;
        if (!cw) {
            *flip = SPR_FLIP_X;
        }
        break;
    case DIR_E:
        if (cw) {
            *imgy = 0;
        } else {
            *imgy = 4;
            *flip = SPR_FLIP_X;
        }
        break;
    case DIR_W:
        if (cw) {
            *imgy = 4;
        } else {
            *imgy = 0;
            *flip = SPR_FLIP_X;
        }
        break;
    case DIR_SE:
        if (cw) {
            *imgy = 1;
        } else {
            *imgy = 3;
            *flip = SPR_FLIP_X;
        }
        break;
    case DIR_SW:
        if (cw) {
            *imgy = 3;
        } else {
            *imgy = 1;
            *flip = SPR_FLIP_X;
        }
        break;
    case DIR_NE:
        if (cw) {
            *imgy = 7;
        } else {
            *imgy = 5;
            *flip = SPR_FLIP_X;
        }
        break;
    case DIR_NW:
        if (cw) {
            *imgy = 5;
        } else {
            *imgy = 7;
            *flip = SPR_FLIP_X;
        }
        break;
    }
}