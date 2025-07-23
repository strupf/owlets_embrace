// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

#define BOSS_GOLEM_INTRO_TICKS      100
#define BOSS_GOLEM_PLATFORM_Y_FLOOR 450

typedef struct {
    i16 terrain;
    i16 x;
    i8  w;
    i8  h;
    i16 dy;
    i16 t1;
    i16 t2;
    i16 t3;
} boss_golem_platform_param_s;

boss_golem_platform_param_s g_bgp_1[16] = {
    {TILE_TYPE_BRIGHT_STONE, 27 + 0, 3, 2, 60, 20, 150, 170},
    {TILE_TYPE_BRIGHT_STONE, 27 + 4, 2, 1, 80, 20, 150, 170},
    {TILE_TYPE_BRIGHT_STONE, 27 + 8, 3, 1, 100, 20, 150, 170}};

typedef struct {
    i32 y;
    i32 dy;
    i32 t1; // end of rise
    i32 t2; // start of fall
    i32 t3; // end of fall
} boss_golem_platform_s;

void boss_golem_intro_update(g_s *g, boss_golem_s *b);
void boss_golem_slam_cracking(g_s *g, boss_golem_s *b);
void boss_golem_slam_cracking_update(g_s *g, boss_golem_s *b);
void boss_golem_slam_update(g_s *g, boss_golem_s *b);
void boss_golem_slam_end(g_s *g, boss_golem_s *b);

void boss_golem_init(g_s *g, boss_golem_s *b)
{
    if (save_event_exists(g, SAVE_EV_BOSS_GOLEM)) return;

    b->tx = 26;
    b->ty = 25;
    b->tw = 12;
    b->th = 4;
    assert(b->tw * b->th <= BOSS_GOLEM_NUM_TILES);

    obj_s *o = obj_create(g);
    b->golem = o;

    o->ID    = OBJID_BOSS_GOLEM;
    o->pos.x = 400;
    o->pos.y = 360;
    o->w     = 64;
    o->h     = 64;
    o->flags = OBJ_FLAG_HERO_JUMPABLE |
               OBJ_FLAG_HERO_STOMPABLE;
}

void boss_golem_defeated(g_s *g, boss_golem_s *b)
{
    save_event_register(g, SAVE_EV_BOSS_GOLEM);
    for (i32 n = 0; n < b->n_platforms; n++) {
        obj_delete(g, b->platforms[n]);
    }
}

void boss_golem_intro_update(g_s *g, boss_golem_s *b)
{
}

void boss_golem_slam_cracking(g_s *g, boss_golem_s *b)
{
    b->phase       = BOSS_GOLEM_P_SLAM_CRACKING;
    b->phase_tick  = 0;
    b->n_platforms = 3;

    for (i32 n = 0; n < b->n_platforms; n++) {
        boss_golem_platform_param_s param = g_bgp_1[n];

        obj_s *o                 = obj_create(g);
        o->ID                    = OBJID_BOSS_GOLEM_PLATFORM;
        boss_golem_platform_s *p = (boss_golem_platform_s *)o->mem;

        b->platforms[n] = o;
        o->pos.y        = BOSS_GOLEM_PLATFORM_Y_FLOOR;
        o->pos.x        = param.x << 4;
        o->w            = param.w << 4;
        o->h            = param.h << 4;
        p->dy           = param.dy;
        p->t1           = param.t1;
        p->t2           = param.t2;
        p->t3           = param.t3;
    }

    i32 x1 = max_i32(b->tx, 0);
    i32 y1 = max_i32(b->ty, 0);
    i32 x2 = min_i32(b->tx + b->tw, g->tiles_x) - 1;
    i32 y2 = min_i32(b->ty + b->th, g->tiles_y) - 1;
    i32 n  = 0;
    for (i32 y = y1; y <= y2; y++) {
        for (i32 x = x1; x <= x2; x++) {
            i32 i       = x + y * g->tiles_x;
            b->tiles[n] = g->tiles[i];
            n++;
        }
    }
}

void boss_golem_slam_cracking_update(g_s *g, boss_golem_s *b)
{
}

void boss_golem_slam_update(g_s *g, boss_golem_s *b)
{
    cam_screenshake(&g->cam, 50, 1);

    if (rngr_i32(0, 10) == 0) {
        obj_s *o = fallingstone_spawn(g);
        o->pos.x = rngr_i32(10, 400);
        o->pos.y = 100;
    }

    i32 t = b->phase_tick;
    for (i32 n = 0; n < b->n_platforms; n++) {
        obj_s                 *o = b->platforms[n];
        boss_golem_platform_s *p = (boss_golem_platform_s *)o->mem;

        i32 py = 0;
        if (0) {
        } else if (t < p->t1) {
            py = ease_out_quad(0, p->dy, t, p->t1);
        } else if (t < p->t2) {
            py = p->dy;
        } else if (t < p->t3) {
            py = ease_in_quad(p->dy, 0, t - p->t2, p->t3 - p->t2);
        }
        i32 tmove = -py - p->y;
        p->y += tmove;
        obj_move(g, o, 0, tmove);
    }
}

void boss_golem_slam_end(g_s *g, boss_golem_s *b)
{
    for (i32 n = 0; n < b->n_platforms; n++) {
        obj_delete(g, b->platforms[n]);
    }
    b->n_platforms = 0;

    i32 x1 = max_i32(b->tx, 0);
    i32 y1 = max_i32(b->ty, 0);
    i32 x2 = min_i32(b->tx + b->tw, g->tiles_x) - 1;
    i32 y2 = min_i32(b->ty + b->th, g->tiles_y) - 1;

    i32 n = 0;
    for (i32 y = y1; y <= y2; y++) {
        for (i32 x = x1; x <= x2; x++) {
            i32 i       = x + y * g->tiles_x;
            g->tiles[i] = b->tiles[n];
            n++;
        }
    }
    game_on_solid_appear(g);
}

void boss_golem_update(g_s *g, boss_golem_s *b)
{
    b->phase_tick++;
    switch (b->phase) {
    case BOSS_GOLEM_P_INTRO: {
        boss_golem_intro_update(g, b);
        if (b->phase_tick == BOSS_GOLEM_INTRO_TICKS) {
            b->phase      = BOSS_GOLEM_P_IDLE;
            b->phase_tick = 0;
        }
        break;
    }
    case BOSS_GOLEM_P_IDLE: {
        if (b->phase_tick == 100) {
            boss_golem_slam_cracking(g, b);
            b->phase      = BOSS_GOLEM_P_SLAM;
            b->phase_tick = 0;
        }
        break;
    }
    case BOSS_GOLEM_P_SLAM_CRACKING: {
        boss_golem_slam_cracking_update(g, b);

        if (b->phase_tick == b->phase_tick_end || 1) {
            b->phase      = BOSS_GOLEM_P_SLAM;
            b->phase_tick = 0;
        }
        break;
    }
    case BOSS_GOLEM_P_SLAM: {
        b->phase_tick_end = 180;
        boss_golem_slam_update(g, b);

        if (b->phase_tick == b->phase_tick_end) {
            boss_golem_slam_end(g, b);
            b->phase      = BOSS_GOLEM_P_IDLE;
            b->phase_tick = 0;
        }
        break;
    }
    }
}

void boss_golem_draw(g_s *g, boss_golem_s *b, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    switch (b->phase) {
    case BOSS_GOLEM_P_SLAM: {
        for (i32 n = 0; n < b->n_platforms; n++) {
            obj_s *o = b->platforms[n];
            v2_i32 p = v2_i32_add(o->pos, cam);
            p.x &= ~1;
            p.y &= ~1;
            render_tile_terrain_block(ctx, p, o->w >> 4, o->h >> 4,
                                      TILE_TYPE_BRIGHT_STONE);
        }
        break;
    }
    }
}