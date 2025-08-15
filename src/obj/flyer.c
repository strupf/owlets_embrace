// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    FLYER_PATH_MODE_PINGPONG,
    FLYER_PATH_MODE_CIRCULAR
};

typedef struct {
    i8     n_path;
    i8     n_from;
    i8     n_to;
    i8     pathdir;
    u8     path_mode;
    i32    len_q4_total;
    i32    len_q4_left;   // used for slowing down at ends for pingpong
    i32    dist_q4_cache; // distance between from and to node
    i32    pos_q4;
    i32    v_q4;
    v2_i32 path[8];
} flyer_s;

static_assert(sizeof(flyer_s) <= OBJ_MEM_BYTES, "flyer");

void flyer_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void flyer_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    flyer_s  *f   = (flyer_s *)o->mem;

    v2_i32 posp = v2_i32_add(o->pos, cam);
    render_tile_terrain_block(ctx, posp, 2, 2, TILE_TYPE_BRIGHT_STONE);
}

void flyer_on_update(g_s *g, obj_s *o)
{
    flyer_s      *f   = (flyer_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];
    o->timer++;

    v2_i32 p_src = f->path[f->n_from];
    v2_i32 p_dst = f->path[f->n_to];
    i32    v_q4  = f->v_q4;

#define FLYER_D_ACC_Q4 300 // acceleration window at end points
    if (f->path_mode == FLYER_PATH_MODE_PINGPONG) {
        i32 l_left   = f->len_q4_left;
        i32 l_so_far = f->len_q4_total - f->len_q4_left;

        i32 l_ease = FLYER_D_ACC_Q4;
        if (l_left < FLYER_D_ACC_Q4) {
            l_ease = l_left;
        } else if (l_so_far < FLYER_D_ACC_Q4) {
            l_ease = l_so_far;
        }
        v_q4 = ease_out_quad(1, v_q4, l_ease, FLYER_D_ACC_Q4);
        f->len_q4_left -= v_q4;
    }

    f->pos_q4 += v_q4;
    bool32 crossed_node = (f->dist_q4_cache <= f->pos_q4);
    if (crossed_node) {
        f->pos_q4 -= f->dist_q4_cache;
        f->n_from = f->n_to;
        f->n_to += f->pathdir;

        if (0 < f->pathdir) {
            if (f->n_to == f->n_path) {
                f->pos_q4      = 0;
                f->len_q4_left = f->len_q4_total;

                if (f->path_mode == FLYER_PATH_MODE_CIRCULAR) {
                    f->n_to = 0;
                } else {
                    f->n_to    = f->n_path - 2;
                    f->pathdir = -f->pathdir;
                }
            }
        } else {
            if (f->n_to == -1) {
                f->pos_q4      = 0;
                f->len_q4_left = f->len_q4_total;

                if (f->path_mode == FLYER_PATH_MODE_CIRCULAR) {
                    f->n_to = f->n_path - 1;
                } else {
                    f->n_to    = 1;
                    f->pathdir = -f->pathdir;
                }
            }
        }

        p_src            = f->path[f->n_from];
        p_dst            = f->path[f->n_to];
        f->dist_q4_cache = v2_i32_distance(p_src, p_dst) << 4;
    }

    v2_i32 p  = v2_i32_lerp(p_src, p_dst, f->pos_q4, f->dist_q4_cache);
    v2_i32 dt = v2_i32_sub(p, o->pos);
    obj_move(g, o, dt.x, dt.y);
    o->ropeobj.v_q12.x = dt.x << 12;
    o->ropeobj.v_q12.y = dt.y << 12;
    o->ropeobj.m_q12   = Q_12(1.0);
    if (0 < dt.x) spr->flip = SPR_FLIP_X;
    if (0 > dt.x) spr->flip = 0;
}

void flyer_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *spr = &o->sprites[0];
    o->animation++;

#define BUG_V 8000
    i32 wingID    = (o->animation >> 1) & 1;
    i32 sin1      = sin_q16(o->animation * BUG_V);
    i32 frameID   = (((o->animation * BUG_V + 10000) * 6) / 0x40000) % 6;
    frameID       = clamp_i32(frameID, 0, 5);
    spr->offs.y   = -40 + ((4 * sin1) >> 16);
    spr->offs.x   = (o->w - 96) / 2;
    texrec_s tbug = asset_texrec(TEXID_FLYING_BUG, frameID * 96, 1 * 96, 96, 96);
    spr->trec     = tbug;
}

void flyer_load(g_s *g, map_obj_s *mo)
{
    obj_s   *o = obj_create(g);
    flyer_s *f = (flyer_s *)o->mem;
    o->ID      = OBJID_FLYER;
    o->flags =
        OBJ_FLAG_PLATFORM |
        OBJ_FLAG_HOOKABLE |
        OBJ_FLAG_KILL_OFFSCREEN;
    o->w               = 36;
    o->h               = 36;
    o->pos.x           = mo->x + (mo->w - o->w) / 2;
    o->pos.y           = mo->y + (mo->h - o->h) / 2;
    o->facing          = 1;
    o->n_sprites       = 1;
    f->pathdir         = 1;
    f->n_to            = 1;
    o->render_priority = RENDER_PRIO_OWL - 1;

    i32     num  = 0;
    // pltf_log("eq %i\n", str_eq_nc("Path", "Path"));
    v2_i16 *path = map_obj_arr(mo, "Path", &num);
    f->v_q4      = map_obj_i32(mo, "Vel");
    f->path_mode = map_obj_bool(mo, "Circular");
    f->n_path    = num + 1;
    f->path[0]   = obj_pos_center(o);
    for (i32 n = 0; n < num; n++) {
        f->path[1 + n].x = path[n].x * 16 + 8;
        f->path[1 + n].y = path[n].y * 16 + 8;
    }

    for (i32 n = 1; n < f->n_path; n++) {
        v2_i32 a = f->path[n - 1];
        v2_i32 b = f->path[n];
        f->len_q4_total += v2_i32_distance(a, b) << 4;
    }
    f->len_q4_left   = f->len_q4_total;
    v2_i32 p_src     = f->path[f->n_from];
    v2_i32 p_dst     = f->path[f->n_to];
    f->dist_q4_cache = v2_i32_distance(p_src, p_dst) << 4;

    // o->on_animate = flyer_on_animate;
    o->on_update = flyer_on_update;
    o->on_draw   = flyer_on_draw;
}
