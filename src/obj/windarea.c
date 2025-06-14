// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    WINDAREA_UP,
    WINDAREA_DO,
    WINDAREA_LE,
    WINDAREA_RI
};

#define WINDAREA_SIDEWAYS_ACC       Q_VOBJ(1.00)
#define WINDAREA_SIDEWAYS_THRESHOLD Q_VOBJ(5.00)

typedef struct windarea_pt_s {
    ALIGNAS(8)
    i16 x_q4; // position in "wind" direction
    i16 y_q4;
    i16 vx_q4;
    i16 vy_q4;
} windarea_pt_s;

#define WINDAREA_NUM_PT 256

typedef struct {
    i32 str;
    i32 trigger_enable;
    i32 trigger_disable;

    i32            particles_per_tick_q4;
    i32            particle_tick_q4;
    i32            pt_extend;
    i32            pt_width;
    windarea_pt_s *pt;
    i32            n_pt;
} windarea_s;

void windarea_on_update(g_s *g, obj_s *o);
void windarea_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void windarea_on_update_upwards(g_s *g, obj_s *o, obj_s *oh);
void windarea_on_update_downwards(g_s *g, obj_s *o, obj_s *oh);
void windarea_on_update_sideways(g_s *g, obj_s *o, obj_s *oh);
void windarea_on_trigger(g_s *g, obj_s *o, i32 trigger);

void windarea_load(g_s *g, map_obj_s *mo)
{
    obj_s      *o      = obj_create(g);
    windarea_s *w      = (windarea_s *)o->mem;
    o->ID              = OBJID_WINDAREA;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->on_update       = windarea_on_update;
    o->on_draw         = windarea_on_draw;
    w->str             = 30 + map_obj_i32(mo, "Strength");
    w->trigger_enable  = map_obj_i32(mo, "Trigger_enable");
    w->trigger_disable = map_obj_i32(mo, "Trigger_disable");
    if (w->trigger_enable || w->trigger_disable) {
        o->on_trigger = windarea_on_trigger;
    }
    o->state           = map_obj_bool(mo, "Enabled");
    o->render_priority = RENDER_PRIO_BEHIND_TERRAIN_LAYER;

    if (0) {
    } else if (str_eq_nc(mo->name, "Windarea_U")) {
        o->subID     = WINDAREA_UP;
        w->pt_extend = o->h << 4;
        w->pt_width  = o->w << 4;
    } else if (str_eq_nc(mo->name, "Windarea_D")) {
        o->subID     = WINDAREA_DO;
        w->pt_extend = o->h << 4;
        w->pt_width  = o->w << 4;
    } else if (str_eq_nc(mo->name, "Windarea_L")) {
        o->subID     = WINDAREA_LE;
        w->pt_extend = o->w << 4;
        w->pt_width  = o->h << 4;
    } else if (str_eq_nc(mo->name, "Windarea_R")) {
        o->subID     = WINDAREA_RI;
        w->pt_extend = o->w << 4;
        w->pt_width  = o->h << 4;
    }
    w->particles_per_tick_q4 = w->pt_width >> 7;
    w->pt                    = game_alloctn(g, windarea_pt_s, WINDAREA_NUM_PT);
}

void windarea_on_update(g_s *g, obj_s *o)
{
    o->animation++;
    o->timer++;

    if (!o->state) return;

    windarea_s *w = (windarea_s *)o->mem;

    for (i32 n = w->n_pt - 1; 0 <= n; n--) {
        windarea_pt_s *pt = &w->pt[n];
        if (pt->y_q4 + 16 < 0) {
            pt->y_q4  = -14;
            pt->vy_q4 = +2;
        }
        if (pt->y_q4 - 16 >= w->pt_width) {
            pt->y_q4  = w->pt_width + 14;
            pt->vy_q4 = -2;
        }
        pt->vy_q4 += rngr_sym_i32(1);
        pt->vx_q4 += rngr_sym_i32(1);
        pt->x_q4 += pt->vx_q4;
        pt->y_q4 += pt->vy_q4;

        if (w->pt_extend <= pt->x_q4) {
            *pt = w->pt[--w->n_pt];
        }
    }

    w->particle_tick_q4 += w->particles_per_tick_q4;
    i32 n_spawn = min_i32(w->particle_tick_q4 >> 4, WINDAREA_NUM_PT - w->n_pt);
    w->particle_tick_q4 &= 15;

    for (i32 n = 0; n < n_spawn; n++) {
        windarea_pt_s *pt = &w->pt[w->n_pt++];
        mclr(pt, sizeof(windarea_pt_s));
        pt->y_q4  = rngr_i32(-16, w->pt_width + 16);
        pt->vx_q4 = 100;
    }

    obj_s *oh = obj_get_hero(g);
    if (!oh || !overlap_rec_pnt(obj_aabb(o), obj_pos_center(oh))) return;

    switch (o->subID) {
    case WINDAREA_UP:
        windarea_on_update_upwards(g, o, oh);
        break;
    case WINDAREA_DO:
        windarea_on_update_downwards(g, o, oh);
        break;
    case WINDAREA_RI:
    case WINDAREA_LE:
        windarea_on_update_sideways(g, o, oh);
        break;
    }
}

void windarea_on_update_upwards(g_s *g, obj_s *o, obj_s *oh)
{
    if (Q_VOBJ(0.25) < oh->v_q12.y) {
        obj_vy_q8_mul(oh, 240);
    }

    i32 scl = min_i32(oh->pos.y - o->pos.y + oh->h, 32);

    if (-Q_VOBJ(6.0) < oh->v_q12.y) {
        oh->v_q12.y -= lerp_i32(0, Q_VOBJ(0.6), scl, 32);
        oh->v_q12.y = max_i32(oh->v_q12.y, -Q_VOBJ(6.0));
    }

    hero_s *h  = (hero_s *)oh->heap;
    h->gliding = min_i32(h->gliding + 2, 16);
    hero_stamina_modify(oh, 32);
}

void windarea_on_update_downwards(g_s *g, obj_s *o, obj_s *oh)
{
    if (oh->v_q12.y < Q_VOBJ(6.0)) {
        oh->v_q12.y += Q_VOBJ(0.1);
        oh->v_q12.y = min_i32(oh->v_q12.y, Q_VOBJ(6.0));
    }

    hero_s *h  = (hero_s *)oh->heap;
    h->gliding = min_i32(h->gliding + 2, 16);
    hero_stamina_modify(oh, 32);
}

void windarea_on_update_sideways(g_s *g, obj_s *o, obj_s *oh)
{
    windarea_s *w        = (windarea_s *)o->mem;
    bool32      grounded = obj_grounded(g, oh);
    i32         xdir     = o->subID == WINDAREA_LE ? -1 : +1;

    if (grounded) {
        obj_move_by_q12(g, oh, xdir * Q_VOBJ((w->str)), 0);
    } else {
        hero_s *h  = (hero_s *)oh->heap;
        h->gliding = min_i32(h->gliding + 2, 16);
        hero_stamina_modify(oh, 32);
        if (-Q_VOBJ(6.0) < oh->v_q12.y) {
            oh->v_q12.y = max_i32(oh->v_q12.y - Q_VOBJ(0.1), -Q_VOBJ(6.0));
        }

        switch (o->subID) {
        case WINDAREA_LE: {
            oh->v_q12.x -= WINDAREA_SIDEWAYS_ACC;
            oh->v_q12.x = max_i32(oh->v_q12.x, -WINDAREA_SIDEWAYS_THRESHOLD);
            break;
        }
        case WINDAREA_RI: {
            oh->v_q12.x += WINDAREA_SIDEWAYS_ACC;
            oh->v_q12.x = min_i32(oh->v_q12.x, +WINDAREA_SIDEWAYS_THRESHOLD);
            break;
        }
        }
    }
}

void windarea_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    rec_i32 rroom = {-8, -8, g->pixel_x + 16, g->pixel_y + 16};
    if (!overlap_rec(obj_aabb(o), rroom)) return;

    gfx_ctx_s   ctx = gfx_ctx_display();
    windarea_s *w   = (windarea_s *)o->mem;
    v2_i32      p   = v2_i32_add(o->pos, cam);
    texrec_s    tr  = asset_texrec(TEXID_WIND, 0, 0, 32, 32);

    for (i32 pass = 0; pass < 2; pass++) {
        for (i32 n = 0; n < w->n_pt; n++) {
            windarea_pt_s *pt = &w->pt[n];
            v2_i32         wp = {p.x + (pt->x_q4 >> 4),
                                 p.y + (pt->y_q4 >> 4)};

            i32 d = -sgn_i32((pt->y_q4 >> 4) - ((pt->y_q4 - pt->vy_q4) >> 4));

            switch (pass) {
            case 0:
                gfx_rec_fill(ctx, (rec_i32){wp.x - 1, wp.y - 1, 8 + 2, 1 + 2}, PRIM_MODE_WHITE);
                gfx_rec_fill(ctx, (rec_i32){wp.x - 16 + 1, wp.y + d - 1, 16 - 2, 1 + 2}, PRIM_MODE_WHITE);
                break;
            case 1:
                gfx_rec_fill(ctx, (rec_i32){wp.x, wp.y, 8, 1}, PRIM_MODE_BLACK);
                gfx_rec_fill(ctx, (rec_i32){wp.x - 16, wp.y + d, 16, 1}, PRIM_MODE_BLACK);
                break;
            }
        }
    }
}

void windarea_on_trigger(g_s *g, obj_s *o, i32 trigger)
{
    windarea_s *w = (windarea_s *)o->mem;
    if (0) {
    } else if (trigger == w->trigger_enable) {
        if (o->state == 0) {
            o->state = 1;
            o->timer = 0;
        }
    } else if (trigger == w->trigger_disable) {
        if (o->state == 1) {
            o->state = 0;
            o->timer = 0;
        }
    }
}