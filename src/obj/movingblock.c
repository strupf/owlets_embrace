// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    v2_i32 p_q8;
} tendril_seg2_s;

enum {
    MOVINGBLOCK_PATH_CIRCLE, // literally a circle
    MOVINGBLOCK_PATH_CIRCULAR,
    MOVINGBLOCK_PATH_PINGPONG,
};

typedef struct movingblock_s {
    u8  trigger_on;
    u8  trigger_off;
    u8  n_seg;
    u8  n_path;
    u8  path_type;
    u8  path_from;
    u8  path_to;
    i16 path_vmax_q8;
    i16 path_v_q8;
    i32 path_pos_q8;

    v2_i32         path_q8[16];
    v2_i32         anchor_root;
    tendril_seg2_s seg[16];
} movingblock_s;

static_assert(sizeof(movingblock_s) <= OBJ_MEM_BYTES, "Size moving block");

void movingblock_on_update(g_s *g, obj_s *o);
void movingblock_on_animate(g_s *g, obj_s *o);
void movingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void movingblock_on_trigger(g_s *g, obj_s *o, i32 trigger);

void movingblock_load(g_s *g, map_obj_s *mo)
{
    sizeof(movingblock_s);
    obj_s         *o   = obj_create(g);
    movingblock_s *m   = (movingblock_s *)o->mem;
    o->ID              = OBJID_MOVINGBLOCK;
    o->w               = mo->w;
    o->h               = mo->h;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->flags           = OBJ_FLAG_SOLID;
    o->render_priority = RENDER_PRIO_HERO + 1;
    o->on_update       = movingblock_on_update;
    o->on_draw         = movingblock_on_draw;
    o->on_trigger      = movingblock_on_trigger;
    o->on_animate      = movingblock_on_animate;

    i32     n_pt;
    v2_i16 *pt = map_obj_arr(mo, "path", &n_pt);
    if (pt) {
        m->n_path       = n_pt + 1;
        m->path_q8[0].x = (o->pos.x + o->w / 2) << 8;
        m->path_q8[0].y = (o->pos.y + o->h / 2) << 8;
        for (i32 n = 0; n < n_pt; n++) {
            m->path_q8[n + 1].x = (pt[n].x << 8) + 128;
            m->path_q8[n + 1].y = (pt[n].y << 8) + 128;
        }
        m->path_to = 1;
    } else {
    }

    m->path_vmax_q8  = map_obj_i32(mo, "v_q8");
    m->trigger_on    = map_obj_i32(mo, "trigger_on");
    m->trigger_off   = map_obj_i32(mo, "trigger_off");
    o->state         = map_obj_bool(mo, "active");
    v2_i16 anchor    = map_obj_pt(mo, "anchor_root");
    m->anchor_root.x = anchor.x;
    m->anchor_root.y = anchor.y;
}

void movingblock_on_update(g_s *g, obj_s *o)
{
    movingblock_s *m = (movingblock_s *)o->mem;

    switch (m->path_type) {
    case MOVINGBLOCK_PATH_CIRCLE: {

        break;
    }
    case MOVINGBLOCK_PATH_CIRCULAR:
    case MOVINGBLOCK_PATH_PINGPONG: {
        m->path_pos_q8 += m->path_v_q8;

        while (1) {
            v2_i32 p1      = m->path_q8[m->path_from];
            v2_i32 p2      = m->path_q8[m->path_to];
            i32    l       = v2_i32_distance(p1, p2);
            bool32 crossed = 0;

            if (m->path_pos_q8 < 0) {
                crossed    = 1;
                i32 p_to   = m->path_to;
                m->path_to = m->path_from;

                switch (m->path_type) {
                case MOVINGBLOCK_PATH_CIRCULAR: {
                    if (m->path_from == 0) {
                        m->path_from = m->n_path - 1;
                    } else {
                        m->path_from--;
                    }
                    break;
                }
                case MOVINGBLOCK_PATH_PINGPONG: {
                    if (m->path_from == m->n_path - 1 ||
                        m->path_from == 0) {
                        m->path_from = p_to;
                    } else {
                        m->path_from += p_to - m->path_from;
                    }
                    break;
                }
                }

                m->path_pos_q8 += v2_i32_distance(m->path_q8[m->path_from],
                                                  m->path_q8[m->path_to]);
            } else if (l <= m->path_pos_q8) {
                crossed = 1;
                m->path_pos_q8 -= l;

                switch (m->path_type) {
                case MOVINGBLOCK_PATH_CIRCULAR: {
                    if (m->path_from == m->n_path - 1) {
                        m->path_from = 0;
                    } else {
                        m->path_from++;
                    }
                    break;
                }
                case MOVINGBLOCK_PATH_PINGPONG: {
                    if (m->path_from == 0) {
                        m->path_from = m->n_path - 1;
                    } else {
                        m->path_from--;
                    }
                    break;
                }
                }
            }

            if (!crossed) {
                break;
            }
        }
        break;
    }
    }
}

void movingblock_on_animate(g_s *g, obj_s *o)
{
    movingblock_s *m = (movingblock_s *)o->mem;

    i32 l = 16 << 8;
    for (i32 k = (i32)m->n_seg - 1; 1 <= k; k--) {
        tendril_seg2_s *sa = &m->seg[k - 1];
        tendril_seg2_s *sb = &m->seg[k];

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
        if (k < m->n_seg - 1) {
            sb->p_q8 = v2_i32_add(p0, vadd);
        }
    }
}

void movingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s      ctx = gfx_ctx_display();
    movingblock_s *m   = (movingblock_s *)o->mem;
    v2_i32         p   = v2_i32_add(o->pos, cam);
}

void movingblock_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    movingblock_s *m = (movingblock_s *)o->mem;

    switch (o->state) {
    case 0: {
        if (trigger != m->trigger_on) break;

        o->state = 1;
        break;
    }
    case 1:
    default: {
        if (trigger != m->trigger_off) break;

        o->state = 0;
        break;
    }
    }
}
