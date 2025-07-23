// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    FLYER_PATH_MODE_PINGPONG,
    FLYER_PATH_MODE_CIRCULAR
};

typedef struct {
    v2_i32 p_q8;
} tendril_seg_s;

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

    tendril_seg_s ts[12];
} flyer_s;

static_assert(sizeof(flyer_s) <= OBJ_MEM_BYTES, "flyer");

void flyer_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void flyer_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();

    i32            num = 12;
    flyer_s       *f   = (flyer_s *)o->mem;
    tendril_seg_s *s0  = &f->ts[0];
    s0->p_q8.x         = (f->path[0].x + 128) << 8;
    s0->p_q8.y         = f->path[0].y << 8;
    tendril_seg_s *s1  = &f->ts[num - 1];
    s1->p_q8.x         = (o->pos.x + 16) << 8;
    s1->p_q8.y         = (o->pos.y + 16) << 8;
    i32 l              = 16 << 8;

    gfx_ctx_s ctxp = ctx;
    ctxp.pat       = gfx_pattern_bayer_4x4(14);

    for (i32 k = 1; k < num - 1; k++) {
        tendril_seg_s *sa = &f->ts[k];
        sa->p_q8.y += 512;
    }

    for (i32 k = num - 1; 1 <= k; k--) {
        tendril_seg_s *sa = &f->ts[k - 1];
        tendril_seg_s *sb = &f->ts[k];

        v2_i32 p0  = sa->p_q8;
        v2_i32 p1  = sb->p_q8;
        v2_i32 dt  = v2_i32_sub(p1, p0);
        i32    len = v2_i32_len_appr(dt);
        // if (len <= l) continue;

        i32    new_l = l + ((len - l) >> 1);
        v2_i32 vadd  = v2_i32_setlenl(dt, len, new_l);

        if (1 < k) {
            sa->p_q8 = v2_i32_sub(p1, vadd);
        }
        if (k < num - 1) {
            sb->p_q8 = v2_i32_add(p0, vadd);
        }
    }

    for (i32 k = 1; k < num; k++) {
        tendril_seg_s *sa = &f->ts[k - 1];
        tendril_seg_s *sb = &f->ts[k];

        v2_i32 pa  = v2_i32_add(v2_i32_shr(sa->p_q8, 8), cam);
        v2_i32 pb  = v2_i32_add(v2_i32_shr(sb->p_q8, 8), cam);
        v2_i32 pdt = v2_i32_sub(pa, pb);

        i32 ptid     = lerp_i32(32, 65, k - 1, num);
        ptid         = (ptid + 1) & ~1;
        ctxp.pat     = gfx_pattern_bayer_8x8(ptid);
        ctxp.pat     = gfx_pattern_shift(ctxp.pat, 0, -cam.x);
        texrec_s trr = asset_texrec(TEXID_ROOTS, 0, 0, 64, 64);
        trr.y        = atan2_index_pow2((f32)pdt.y, (f32)pdt.x, 64, 0) * 64;
        trr.x        = clamp_i32(lerp_i32(0, 4, k, num), 0, 3) * 64;
        v2_i32 pmid  = v2_i32_lerp(pa, pb, 1, 2);
        v2_i32 ps    = {pmid.x - 32, pmid.y - 32};
        gfx_spr(ctxp, trr, ps, 0, SPR_MODE_BLACK_ONLY_WHITE_PT_OPAQUE);
    }

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
    o->ropeobj.v_q8.x = dt.x << 8;
    o->ropeobj.v_q8.y = dt.y << 8;
    o->ropeobj.m_q8   = 256;
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
    o->render_priority = RENDER_PRIO_HERO - 1;

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
