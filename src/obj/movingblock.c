// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "pathmover.h"

enum {
    PATHMOVER_ST_NULL,
    PATHMOVER_ST_MOVING,
    PATHMOVER_ST_APPROACHING_STOP,
    PATHMOVER_ST_STOPPED_TIMER,
    PATHMOVER_ST_STOPPED_TRIGGER,
};

typedef struct movingblock_s {
    i16          path_vmax_q4;
    i16          path_v_q4;
    u16          trigger_on;
    u16          trigger_off;
    u16          trigger_continue;
    u8           ticks_since_stop;
    u8           pos_smoothing_div;
    obj_handle_s tendril;

    ALIGNAS(8)
    pathmover_s pathmover;
    i16         stop_at_ticks[24];
    i8          stopping_at;
    b8          stopping;
    v2_i32      p_q8;
    v2_i32      anchor_root;
} movingblock_s;

static_assert(sizeof(movingblock_s) <= OBJ_MEM_BYTES, "Size moving block");

void movingblock_on_update(g_s *g, obj_s *o);
void movingblock_on_animate(g_s *g, obj_s *o);
void movingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void movingblock_on_trigger(g_s *g, obj_s *o, i32 trigger);

void movingblock_load(g_s *g, map_obj_s *mo)
{
    obj_s         *o     = obj_create(g);
    movingblock_s *m     = (movingblock_s *)o->mem;
    o->editorUID         = mo->UID;
    o->ID                = OBJID_MOVINGBLOCK;
    o->w                 = mo->w;
    o->h                 = mo->h;
    o->pos.x             = mo->x;
    o->pos.y             = mo->y;
    o->flags             = OBJ_FLAG_SOLID;
    o->render_priority   = RENDER_PRIO_OWL + 1;
    o->on_update         = movingblock_on_update;
    o->on_draw           = movingblock_on_draw;
    o->on_trigger        = movingblock_on_trigger;
    o->on_animate        = movingblock_on_animate;
    m->p_q8              = v2_i32_shl(o->pos, 8);
    m->pos_smoothing_div = 12;

    i32     ptype = 0;
    i32     n_pt  = 0;
    v2_i16 *pt    = 0;

    if (0) {
    } else if ((pt = map_obj_arr(mo, "path_looping", &n_pt)) && n_pt) {
        ptype = PATHMOVER_PATH_CIRCULAR;
    } else if ((pt = map_obj_arr(mo, "path", &n_pt)) && n_pt) {
        ptype = PATHMOVER_PATH_PINGPONG;
    }
    assert(n_pt);

    i32  num_stop_at    = 0;
    i32 *stop_at        = map_obj_arr(mo, "stop_at_ticks", &num_stop_at);
    m->stop_at_ticks[0] = map_obj_i32(mo, "stop_at_ticks_start");
    for (i32 n = 0; n < num_stop_at; n++) {
        m->stop_at_ticks[n + 1] = stop_at[n];
    }

    v2_i16 anchor        = map_obj_pt(mo, "anchor_root");
    i32    tendril_l_max = 64;
    m->anchor_root.x     = (i32)anchor.x << 4;
    m->anchor_root.y     = (i32)anchor.y << 4;
    assert(pt);

    if (pt) {
        v2_i32 ppath[24] = {0};

        ppath[0].x = o->pos.x;
        ppath[0].y = o->pos.y;

        for (i32 n = 0; n < n_pt; n++) {
            ppath[n + 1].x = (i32)pt[n].x << 4;
            ppath[n + 1].y = (i32)pt[n].y << 4;
        }

        for (i32 n = 0; n < n_pt + 1; n++) {
            v2_i32 ptendril = ppath[n];
            ptendril.x += o->w >> 1;
            ptendril.y += o->h >> 1;
            i32 lt        = v2_i32_distance_appr(ptendril, m->anchor_root);
            tendril_l_max = max_i32(tendril_l_max, lt);
            ppath[n]      = v2_i32_shl(ppath[n], 8);
        }
        pathmover_setup(&m->pathmover, ppath, n_pt + 1, ptype);
    }

    obj_s *o_tendril = tendrilconnection_create(g);
    tendrilconnection_setup(g, o_tendril, m->anchor_root, obj_pos_center(o), tendril_l_max);
    m->tendril = handle_from_obj(o_tendril);

    m->path_vmax_q4     = map_obj_i32(mo, "v_q8");
    m->trigger_on       = map_obj_i32(mo, "trigger_on");
    m->trigger_off      = map_obj_i32(mo, "trigger_off");
    m->trigger_continue = map_obj_i32(mo, "trigger_continue");
    o->state            = map_obj_bool(mo, "active");
}

void movingblock_on_update(g_s *g, obj_s *o)
{
    movingblock_s *m = (movingblock_s *)o->mem;
    m->path_v_q4     = 1024;

    if (!m->stopping) {
        if (o->timer) {

            if (0 < o->timer) {
                o->timer--;
            }

            if (m->ticks_since_stop < 255) {
                m->ticks_since_stop++;
            }
        } else {
            m->ticks_since_stop = 0;
            i32 reached_node    = -1;
            i32 d_move_q4       = m->path_v_q4;

            // i32 len_to = pathmover_distance_to(&m->pathmover, 2, 0);
            // d_move_q4  = lerp_i32(10, d_move_q4, min_i32(len_to, 4096), 4096);

            while (d_move_q4) {
                d_move_q4 = pathmover_move_on_path_to_next(&m->pathmover, d_move_q4, &reached_node);

                if (0 <= reached_node && m->stop_at_ticks[reached_node]) {
                    m->stopping         = 1;
                    m->stopping_at      = reached_node;
                    m->ticks_since_stop = 0;
                    break;
                }
            }
        }
    }

    v2_i32 trg_pos_q8 = pathmover_curr_pos(&m->pathmover);
    v2_i32 trg_pos    = v2_i32_shr(trg_pos_q8, 8);

    if (m->stopping && v2_i32_distance_appr(m->p_q8, trg_pos_q8) < 512) {
        m->p_q8 = trg_pos_q8;
        if (m->stopping) {
            m->stopping = 0;
            o->timer    = m->stop_at_ticks[m->stopping_at];
        }
    } else {
        m->p_q8 = v2_i32_lerp(m->p_q8, trg_pos_q8, 1, 16);
    }

    v2_i32 dt = v2_i32_sub(v2_i32_shr(m->p_q8, 8), o->pos);
    dt.x &= ~1;
    dt.y &= ~1;

    obj_move(g, o, dt.x, dt.y);

    obj_s *o_tendril = obj_from_handle(m->tendril);
    if (o_tendril) {
        tendrilconnection_constrain_ends(o_tendril, m->anchor_root, obj_pos_center(o));
    }
}

void movingblock_on_animate(g_s *g, obj_s *o)
{
    movingblock_s *m = (movingblock_s *)o->mem;
}

void movingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s      ctx = gfx_ctx_display();
    movingblock_s *m   = (movingblock_s *)o->mem;
    v2_i32         p   = o->pos;
    p                  = v2_i32_add(p, cam);
    // pltf_log("%i\n", p.x);
    p.x &= ~1;
    p.y &= ~1;
    render_tile_terrain_block(ctx, p, o->w >> 4, o->h >> 4, TILE_TYPE_BRIGHT_STONE);
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