// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map.h"
#include "game.h"

#define MAP_SCL_MIN 1024
#define MAP_SCL_MAX 16384

void map_init(g_s *g)
{
    map_s *m = &g->map;
}

void map_update(g_s *g)
{
    map_s *m = &g->map;
    m->crank_acc_q16 += inp_crank_dt_q16();
    i32 d = m->crank_acc_q16 >> 4;
    m->crank_acc_q16 &= (1 << 4) - 1;
    m->scl = clamp_i32(m->scl + d, MAP_SCL_MIN, MAP_SCL_MAX);
}

void map_draw(g_s *g)
{
    gfx_ctx_s ctx   = gfx_ctx_display();
    map_s    *m     = &g->map;
    obj_s    *ohero = obj_get_hero(g);
    rec_i32   rcl   = {0, 0, 100, 50};
    gfx_rec_fill(ctx, rcl, PRIM_MODE_WHITE);
    m->w_pixel  = 100;
    m->h_pixel  = 50;
    m->scl      = MAP_SCL_MIN;
    ctx.clip_x2 = rcl.w - 1;
    ctx.clip_y2 = rcl.h - 1;

    if (ohero) {
        m->p.x = ohero->pos.x + g->map_room_cur->x * 16;
        m->p.y = ohero->pos.y + g->map_room_cur->y * 16;
    }

    for (i32 n = 0; n < g->n_map_rooms; n++) {
        map_room_s *r = &g->map_rooms[n];

        v2_i32 corners_world[2]  = {{r->x, r->y},
                                    {r->x + r->w, r->y + r->h}};
        v2_i32 corners_screen[2] = {
            map_coords_screen_from_world_q8(m, v2_i32_shl(corners_world[0], 4)),
            map_coords_screen_from_world_q8(m, v2_i32_shl(corners_world[1], 4))};
        rec_i32 rf = {corners_screen[0].x,
                      corners_screen[0].y,
                      corners_screen[1].x - corners_screen[0].x,
                      corners_screen[1].y - corners_screen[0].y};
        gfx_rec_fill(ctx, rf, PRIM_MODE_BLACK);
    }
}

v2_i32 map_coords_world_q8_from_screen(map_s *m, v2_i32 pxpos)
{
    v2_i32 p = {
        (((pxpos.x - (m->w_pixel >> 1)) << 16) / m->scl + m->p.x),
        (((pxpos.y - (m->h_pixel >> 1)) << 16) / m->scl + m->p.y)};
    return p;
}

v2_i32 map_coords_screen_from_world_q8(map_s *m, v2_i32 wp_q8)
{
    v2_i32 p = {
        (((wp_q8.x - m->p.x) * m->scl) >> 16) + (m->w_pixel >> 1),
        (((wp_q8.y - m->p.y) * m->scl) >> 16) + (m->h_pixel >> 1)};
    return p;
}