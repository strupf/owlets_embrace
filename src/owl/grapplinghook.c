// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "owl/grapplinghook.h"
#include "game.h"

#define GRAPPLING_HOOK_GRAV Q_12(0.31)

bool32 grapplinghook_try_attach(g_s *g, grapplinghook_s *h, obj_s *o);

bool32 grapplinghook_is_hooking_solid(grapplinghook_s *h, obj_s *osolid)
{
    if (!osolid) return 0;
    obj_s *o = obj_from_handle(h->o2);
    if (!o) return 0;
    rec_i32 r = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};
    if (overlap_rec(r, obj_aabb(osolid))) {
        return 1;
    }
    return 0;
}

bool32 grapplinghook_is_hooking_terrain(g_s *g, grapplinghook_s *h)
{
    obj_s *o = obj_from_handle(h->o2);
    if (!o) return 0;
    rec_i32 r = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};
    if (tile_map_hookable(g, r)) {
        return 1;
    }
    return 0;
}

void grapplinghook_on_attach(g_s *g, grapplinghook_s *h, obj_s *o)
{
    i32    clen_q4 = wire_len_qx(g, &h->wire, 4);
    obj_s *ohero   = obj_from_handle(h->o1);
    i32    st      = owl_state_check(g, ohero);

    if (st == OWL_ST_AIR) {
        clen_q4 = (clen_q4 * 248) >> 8;
    }
    h->len_max_q4 = clamp_i32(clen_q4, HERO_ROPE_LEN_MIN, HERO_ROPE_LEN_MAX);
    snd_instance_stop_fade(h->throw_snd_iID, 25, 0);
    h->attached_tick = 1;
    if (o) {
        o->on_pushed_by_solid = 0;
        o->v_q12.x            = 0;
        o->v_q12.y            = 0;
    }
}

void grapplinghook_on_pushed_end_node(g_s *g, obj_s *o, obj_s *osolid, i32 sx, i32 sy)
{
    grapplinghook_s *h = (grapplinghook_s *)o->heap;
    if (grapplinghook_try_attach(g, h, o)) {
        o->on_pushed_by_solid = 0;
    }
}

void grapplinghook_on_hooked(g_s *g, obj_s *o, i32 hooked)
{
    grapplinghook_s *h = (grapplinghook_s *)o->heap;
    grapplinghook_destroy(g, h);
    obj_delete(g, o);
}

void grapplinghook_create(g_s *g, grapplinghook_s *h, obj_s *ohero, v2_i32 p, v2_i32 v)
{
    mclr(h, sizeof(grapplinghook_s));
    wire_s        *r  = &h->wire;
    rope_verlet_s *rv = &h->rope_verlet;
    wire_init(r);

    obj_s *o              = obj_create(g);
    o->ID                 = OBJID_HOOK;
    o->w                  = 3;
    o->h                  = 3;
    o->pos.x              = p.x - 1;
    o->pos.y              = p.y - 1;
    o->v_q12              = v;
    o->wire               = r;
    o->wirenode           = r->tail;
    o->flags              = OBJ_FLAG_ACTOR;
    o->moverflags         = OBJ_MOVER_TERRAIN_COLLISIONS;
    o->ropeobj.m_q12      = 4096;
    o->on_pushed_by_solid = grapplinghook_on_pushed_end_node;
    o->on_hook            = grapplinghook_on_hooked;
    o->heap               = h;

    ohero->wire            = r;
    ohero->wirenode        = r->head;
    h->o1                  = handle_from_obj(ohero);
    h->o2                  = handle_from_obj(o);
    h->state               = GRAPPLINGHOOK_FLYING;
    r->head->p             = p;
    r->tail->p             = p;
    h->len_max_q4          = 4000;
    g->ropes[g->n_ropes++] = r;

    v2_i32 p_q8 = v2_i32_shl(p, 8);
    rv->r       = r;
    rv->n       = 24;

    // "hint" the direction to the verlet sim
    for (i32 n = 0; n < rv->n; n++) {
        rope_pt_s *pt = &rv->p[n];

        i32 k    = max_i32(0, rv->n - 1 - n);
        pt->p    = p_q8;
        pt->pp.x = p_q8.x - ((v.x >> 4) * k) / rv->n;
        pt->pp.y = p_q8.y - ((v.y >> 4) * k) / rv->n;
    }
}

void grapplinghook_unhook_obj(g_s *g, obj_s *o)
{
    o->wire     = 0;
    o->wirenode = 0;

    if (o->on_hook) {
        o->on_hook(g, o, 0);
    }
}

void grapplinghook_destroy(g_s *g, grapplinghook_s *h)
{
    if (!h->state) return;

    h->state           = 0;
    h->destroy_tick_q8 = 0;
    obj_s *o1          = obj_from_handle(h->o1);
    if (o1) {
        grapplinghook_unhook_obj(g, o1);
    }
    obj_s *o2 = obj_from_handle(h->o2);
    if (o2) {
        grapplinghook_unhook_obj(g, o2);
    }
    g->n_ropes = 0;
}

bool32 grapplinghook_try_attach(g_s *g, grapplinghook_s *h, obj_s *o)
{
    if (grapplinghook_is_hooking_terrain(g, h)) {
        grapplinghook_on_attach(g, h, o);
        h->state = GRAPPLINGHOOK_HOOKED_TERRAIN;
        return 1;
    }

    for (obj_each(g, i)) {
        if ((i->flags & OBJ_FLAG_SOLID) && grapplinghook_is_hooking_solid(h, i)) {
            grapplinghook_on_attach(g, h, o);
            h->state        = GRAPPLINGHOOK_HOOKED_SOLID;
            h->solid        = handle_from_obj(i);
            o->linked_solid = h->solid;
            if (i->on_hook) {
                i->on_hook(g, i, 1);
            }
            return 1;
        }
    }
    return 0;
}

bool32 grapplinghook_step_obj(g_s *g, grapplinghook_s *h, obj_s *o, i32 sx, i32 sy)
{
    if (grapplinghook_try_attach(g, h, o)) return 0;
    o->pos.x += sx;
    o->pos.y += sy;
    wirenode_move(g, o->wire, o->wirenode, sx, sy);
    return 1;
}

void grapplinghook_update(g_s *g, grapplinghook_s *h)
{
    if (!h->state) return;
    if (h->destroy_tick_q8) {
        h->destroy_tick_q8 = 0;
    }

    obj_s *o = obj_from_handle(h->o2);
    if (!o) {
        grapplinghook_destroy(g, h);
        return;
    }

    rec_i32 ro = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};
    switch (h->state) {
    case GRAPPLINGHOOK_FLYING: {
        o->v_q12.y += GRAPPLING_HOOK_GRAV;
        o->subpos_q12 = v2_i32_add(o->subpos_q12, o->v_q12);

        i32 dx = o->subpos_q12.x >> 12;
        i32 dy = o->subpos_q12.y >> 12;
        o->subpos_q12.x &= 0xFFF;
        o->subpos_q12.y &= 0xFFF;

        i32 px = +abs_i32(dx);
        i32 py = -abs_i32(dy);
        i32 sx = +sgn_i32(dx);
        i32 sy = +sgn_i32(dy);
        i32 e  = px + py;
        i32 x  = 0;
        i32 y  = 0;

        // bresenham for a more linear movement
        while (x != dx || y != dy) {
            i32 e2 = e << 1;
            if (e2 >= py) {
                if (!grapplinghook_step_obj(g, h, o, sx, 0)) break;
                e += py;
                x += sx;
            }
            if (e2 <= px) {
                if (!grapplinghook_step_obj(g, h, o, 0, sy)) break;
                e += px;
                y += sy;
            }
        }

        break;
    }
    case GRAPPLINGHOOK_HOOKED_TERRAIN: {
        if (!grapplinghook_is_hooking_terrain(g, h)) {
            grapplinghook_destroy(g, h);
            break;
        }
        break;
    }
    case GRAPPLINGHOOK_HOOKED_SOLID: {
        if (!grapplinghook_is_hooking_solid(h, obj_from_handle(h->solid))) {
            grapplinghook_destroy(g, h);
            break;
        }
        break;
    }
    }
}

void grapplinghook_animate(g_s *g, grapplinghook_s *h)
{
    if (!h->state) return;

    wire_s *r = &h->wire;
    rope_verlet_sim(g, &h->rope_verlet);

    wirenode_s *rn = wirenode_neighbour_of_end_node(r, r->tail);

    if (h->state == GRAPPLINGHOOK_FLYING) {
        v2_i32 rndt            = v2_i32_sub(r->tail->p, rn->p);
        h->anghist[h->n_ang++] = rndt;
        h->n_ang &= 3;
    }
}

void grapplinghook_draw(g_s *g, grapplinghook_s *h, v2_i32 cam)
{
    if (h->state == 0 && !h->destroy_tick_q8)
        return;

    gfx_ctx_s        ctx  = gfx_ctx_display();
    wire_s          *wire = &h->wire;
    grapplinghook_s *gh   = &g->ghook;
    v2_i32           ropepts[ROPE_VERLET_N];
    i32              n_ropepts = 0;

    for (i32 k = 1; k < h->rope_verlet.n; k++) {
        v2_i32 p             = v2_i32_shr(h->rope_verlet.p[k].p, 8);
        ropepts[n_ropepts++] = v2_i32_add(p, cam);
    }

    i32 n_ropepts_beg = 1;
    if (h->destroy_tick_q8) {
        n_ropepts_beg = lerp_i32(n_ropepts - 1, 1, h->destroy_tick_q8, 256);
        i32 patID     = ease_out_quad(0, GFX_PATTERN_MAX, h->destroy_tick_q8, 256);
        ctx.pat       = gfx_pattern_bayer_4x4(patID);
    }

    gfx_ctx_s ctxhookpt = ctx;
    ctxhookpt.pat       = gfx_pattern_4x4(B4(0111),
                                          B4(1010),
                                          B4(1101),
                                          B4(1010));

    for (i32 k = n_ropepts_beg; k < n_ropepts; k++) {
        v2_i32 p1 = ropepts[k - 1];
        v2_i32 p2 = ropepts[k];
        gfx_lin_thick(ctx, p1, p2, GFX_COL_WHITE, 10);
    }
    for (i32 k = n_ropepts_beg; k < n_ropepts; k++) {
        v2_i32 p1 = ropepts[k - 1];
        v2_i32 p2 = ropepts[k];
        gfx_lin_thick(ctx, p1, p2, GFX_COL_BLACK, 8);
    }
    for (i32 k = n_ropepts_beg; k < n_ropepts; k++) {
        v2_i32 p1 = ropepts[k - 1];
        v2_i32 p2 = ropepts[k];
        gfx_lin_thick(ctxhookpt, p1, p2, GFX_COL_WHITE, 4);
    }

    if (h->state && h->state != GRAPPLINGHOOK_HOOKED_OBJ) {
        v2_i32 vhook = {0};
        for (i32 i = 0; i < 4; i++) {
            vhook = v2_i32_add(vhook, gh->anghist[i]);
        }

        i32         ang    = shr_balanced_i32(atan2_i32(vhook.y, vhook.x) * 16, 18);
        i32         imgy   = (ang + 16 + 4) & 15;
        texrec_s    trhook = {asset_tex(TEXID_HOOK), 0, imgy * 32, 32, 32};
        wirenode_s *wn     = h->wire.tail;
        v2_i32      hpos   = {wn->p.x - 16, wn->p.y - 16};
        gfx_spr_tile_32x32(ctx, trhook, v2_i32_add(hpos, cam));
    }
}

v2_i32 grapplinghook_v_damping(wire_s *r, wirenode_s *rn, v2_i32 subpos_q8, v2_i32 v_q8)
{
    if (!r || !rn) return v_q8;

    wirenode_s *rprev = rn->next ? rn->next : rn->prev;
    assert(rprev);

    v2_i32 ropedt = v2_i32_sub(rn->p, rprev->p);
    v2_i32 dt_q4  = v2_i32_add(v2_i32_shl(ropedt, 4), v2_i32_shr(subpos_q8, 8));

    // damping force
    if (v2_i32_dot(ropedt, v_q8) > 0) {
        v2_i32 vrad  = project_pnt_line(v_q8, CINIT(v2_i32){0}, dt_q4);
        v2_i32 fdamp = v2_i32_mulq(vrad, 230, 8);
        return v2_i32_sub(v_q8, fdamp);
    }
    return v_q8;
}

v2_i32 grapplinghook_vlaunch_from_angle(i32 a_q16, i32 vel)
{
    v2_i32 v = {(-vel * sin_q15(a_q16 << 1)) / 32768,
                (-vel * cos_q15(a_q16 << 1)) / 32768};
    return v;
}

// project a vector along the rope onto v
v2_i32 rope_v_to_neighbour(wire_s *r, wirenode_s *rn)
{
    return v2_i32_sub(wirenode_neighbour_of_end_node(r, rn)->p, rn->p);
}

v2_i32 rope_v_pulling_at_obj(wire_s *r, wirenode_s *rn, obj_s *o)
{
    wirenode_s *rn1 = rn;
    wirenode_s *rn2 = wirenode_neighbour_of_end_node(r, rn1);

    if (o->flags & OBJ_FLAG_SOLID) {
        v2_i32 pts[4];
        points_from_rec(obj_aabb(o), pts);

        while (rn2) {
            bool32 is_on_aabb = 0;
            for (i32 n = 0; n < 4; n++) {
                if (v2_i32_eq(rn2->p, pts[n])) {
                    is_on_aabb = 1;
                    break;
                }
            }

            if (is_on_aabb) {
                wirenode_s *rnp = rn2;
                rn2             = rn2->next == rn1 ? rn2->prev : rn2->next;
                rn1             = rnp;
            } else {
                break;
            }
        }
    }
    assert(rn1 && rn2);
    return v2_i32_sub(rn2->p, rn1->p);
}

// calculating pulling force in rope direction at node rn
// based on an attached object with mass, velocity and acceleration
i32 rope_pulling_force_of_ropeobj(wire_s *r, wirenode_s *rn, obj_s *o, ropeobj_param_s param);

#define GH_Q4_SMOOTH 10
void grapplinghook_calc_f_internal(g_s *g, grapplinghook_s *gh)
{
    if (!gh->state) return;

    gh->f_cache_dt     = (gh->f_cache_dt * GH_Q4_SMOOTH) >> 4; // smooth force calculation a bit
    gh->f_cache_o1     = (gh->f_cache_o1 * GH_Q4_SMOOTH) >> 4;
    gh->f_cache_o2     = (gh->f_cache_o2 * GH_Q4_SMOOTH) >> 4;
    gh->param1.a_q12.x = 0;
    gh->param1.a_q12.y = 0;
    gh->param1.v_q12.x = 0;
    gh->param1.v_q12.y = 0;
    gh->param1.m_q12   = 0;
    gh->param2.a_q12.x = 0;
    gh->param2.a_q12.y = 0;
    gh->param2.v_q12.x = 0;
    gh->param2.v_q12.y = 0;
    gh->param2.m_q12   = 0;

    wire_s *r = &gh->wire;

    i32 lc = wire_len_qx(g, r, 4);
    i32 lm = gh->len_max_q4;
    if (lc < lm) return;

    i32 f_dt = (lc - lm) << 2; // spring force
    gh->f_cache_dt += (f_dt * (16 - GH_Q4_SMOOTH)) >> 4;

    obj_s *o1 = obj_from_handle(gh->o1);
    if (o1) {
        gh->param1 = o1->ropeobj;
        i32 f_o1   = rope_pulling_force_of_ropeobj(r, o1->wirenode, o1, gh->param1);
        gh->f_cache_o1 += (f_o1 * (16 - GH_Q4_SMOOTH)) >> 4;
    }

    obj_s *o2 = obj_from_handle(gh->o2);
    obj_s *os = 0;
    // wirenode_s *wn2 = gh->wire.tail;
    if (o2 && o2->ID == OBJID_HOOK && (os = obj_from_handle(o2->linked_solid))) {
        o2 = os;
    }
    if (o2) {
        gh->param2 = o2->ropeobj;
        i32 f_o2   = rope_pulling_force_of_ropeobj(r, gh->wire.tail, o2, gh->param2);
        gh->f_cache_o2 += (f_o2 * (16 - GH_Q4_SMOOTH)) >> 4;
    } else if (gh->state == GRAPPLINGHOOK_HOOKED_TERRAIN) {
        gh->param2.m_q12 = ROPEOBJ_M_INF;
    }
}

i32 grapplinghook_f_at_obj_proj_v(grapplinghook_s *gh, obj_s *o, v2_i32 dproj, v2_i32 *f_out)
{
    // f = m * a
    // a = f / m
    obj_s      *o1 = obj_from_handle(gh->o1);
    obj_s      *o2 = obj_from_handle(gh->o2);
    wirenode_s *rn = 0;
    i32         f  = gh->f_cache_dt + gh->f_cache_o1 + gh->f_cache_o2;
    if (0) {
    } else if (o == o1) {
        rn = gh->wire.head;
    } else if (o == o2 ||
               (o2 && o2->ID == OBJID_HOOK && obj_from_handle(o2->linked_solid))) {
        rn = gh->wire.tail;
    }

    if (!f || !rn) {
        if (f_out) {
            f_out->x = 0;
            f_out->y = 0;
        }
        return 0;
    }

    v2_i32 dt = rope_v_pulling_at_obj(&gh->wire, rn, o);
    assert(dt.x | dt.y);
    v2_i32 f_dt = v2_i32_setlen(dt, f);

    if (dproj.x | dproj.y) {
        v2_i32 f_proj = project_pnt_dir(f_dt, dproj);
        f             = v2_i32_len(f_proj);

        if (f_out) {
            *f_out = f_proj;
        }
    } else if (f_out) {
        *f_out = f_dt;
    }

    i32 dp = v2_i32_dot(dt, dproj);
    return (dp <= 0 ? +f : -f);
}

i32 grapplinghook_f_at_obj_proj(grapplinghook_s *gh, obj_s *o, v2_i32 dproj)
{
    return grapplinghook_f_at_obj_proj_v(gh, o, dproj, 0);
}

i32 rope_pulling_force_of_ropeobj(wire_s *r, wirenode_s *rn, obj_s *o, ropeobj_param_s param)
{
    if (param.m_q12 == 0) return 0;

    // f = m * a
    // a = f / m

    v2_i32 v_q12 = param.v_q12;
    v2_i32 a_q12 = param.a_q12;
    if (!(v_q12.x | v_q12.y | a_q12.x | a_q12.y)) return 0;

    i32    a_q12_res = 0;
    v2_i32 dt        = rope_v_pulling_at_obj(r, rn, o);
    v2_i32 dt_tang   = {+dt.y, -dt.x};

    // centrifugal effect of velocity (pulling force)
    // f = m * (v^2) / r
    // a = (v^2) / r
    v2_i32 v_q12_tang        = project_pnt_dir(v_q12, dt_tang);
    i32    a_q12_centrifugal = (i32)(v2_i32_lensq(v2_i32_shr(v_q12_tang, 4)) / ((u32)v2_i32_len(dt) << 8));
    a_q12_res += a_q12_centrifugal;

    // acceleration along rope direction
    if (v2_i32_dot(a_q12, dt) < 0) {
        a_q12_res += project_pnt_dir_len(a_q12, dt);
    }

    i32 f_q12 = (param.m_q12 * a_q12_res) >> 12;
    return f_q12;
}

bool32 grapplinghook_rope_intact(g_s *g, grapplinghook_s *h)
{
    wire_s *w = &h->wire;

    if (!wire_is_intact(g, w)) {
        return 0;
    }
    return 1;
}

bool32 pnt_along_array(v2_i32 *pt, i32 n_pt, i32 dst, v2_i32 *p_out)
{
    i32 dst_total = 0;
    for (i32 n = 1; n < n_pt; n++) {
        v2_i32 p0            = pt[n - 1];
        v2_i32 p1            = pt[n];
        i32    d             = v2_i32_distance(p0, p1);
        i32    dst_total_new = dst_total + d;

        if (dst_total <= dst && dst <= dst_total_new) {
            v2_i32 p = v2_i32_lerp(p0, p1, dst - dst_total, d);
            *p_out   = p;
            return 1;
        }

        dst_total = dst_total_new;
    }
    return 0;
}

typedef struct {
    i32    i;
    v2_i32 p;
} verlet_pos_s;

void rope_verlet_sim(g_s *g, rope_verlet_s *rv)
{
    wire_s *r = rv->r;

    // calculated current length in Q8
    ALIGNAS(32) verlet_pos_s vpos[64];

    u32          ropelen_q4 = 1 + wire_len_qx(g, r, 4); // +1 to avoid div 0
    i32          n_vpos     = 0;
    verlet_pos_s vp_beg     = {0, v2_i32_shl(r->tail->p, 8)};
    vpos[n_vpos++]          = vp_beg;

    u32 dista = 0;
    for (wirenode_s *r1 = r->tail, *r2 = r1->prev; r2; r1 = r2, r2 = r2->prev) {
        dista += v2_i32_len_appr(v2_i32_shl(v2_i32_sub(r1->p, r2->p), 4));
        i32 i = (dista * rv->n) / ropelen_q4;
        if (1 <= i && i < rv->n - 1) {
            verlet_pos_s vp = {i, v2_i32_shl(r2->p, 8)};
            vpos[n_vpos++]  = vp;
        }
    }

    verlet_pos_s vp_end = {rv->n - 1, v2_i32_shl(r->head->p, 8)};
    vpos[n_vpos++]      = vp_end;

    i32 len_ratio_q8 = min_i32(256, (ropelen_q4 << 8) / g->ghook.len_max_q4);
    i32 ll_q8        = (g->ghook.len_max_q4 * len_ratio_q8) / (rv->n << 4);

    for (i32 n = 1; n < rv->n - 1; n++) {
        rope_pt_s *pt  = &rv->p[n];
        v2_i32     tmp = pt->p;
        pt->p.x += (pt->p.x - pt->pp.x);
        pt->p.y += (pt->p.y - pt->pp.y) + 60; // gravity
        pt->pp = tmp;
    }

    for (i32 k = 0; k < 3; k++) {
        for (i32 n = 1; n < rv->n; n++) {
            rope_pt_s *p1 = &rv->p[n - 1];
            rope_pt_s *p2 = &rv->p[n];

            v2_i32 dt = v2_i32_sub(p1->p, p2->p);
            i32    dl = v2_i32_len_appr(dt);
            i32    dd = dl - ll_q8;

            if (dd <= 1) continue;
            dt    = v2_i32_setlenl_small(dt, dl, dd >> 1);
            p1->p = v2_i32_sub(p1->p, dt);
            p2->p = v2_i32_add(p2->p, dt);
        }

        for (i32 n = n_vpos - 1; 0 <= n; n--) {
            rv->p[vpos[n].i].p = vpos[n].p;
        }
    }

    if (len_ratio_q8 < 245) return;

    // straighten rope
    for (i32 n = 1; n < rv->n - 1; n++) {
        bool32 contained = 0;

        for (i32 i = 0; i < n_vpos; i++) {
            if (vpos[i].i == n) {
                contained = 1; // is fixed to corner already
                break;
            }
        }
        if (contained) continue;

        // figure out previous and next corner of verlet particle
        verlet_pos_s prev_vp = {0};
        verlet_pos_s next_vp = {0};
        prev_vp.i            = -1;
        next_vp.i            = rv->n;

        for (i32 i = 0; i < n_vpos; i++) {
            verlet_pos_s vp = vpos[i];

            if (prev_vp.i < vp.i && vp.i < n) {
                prev_vp = vpos[i];
            }
            if (vp.i < next_vp.i && n < vp.i) {
                next_vp = vpos[i];
            }
        }

        if (!(0 <= prev_vp.i && next_vp.i < rv->n)) continue;

        // lerp position of particle towards straight line between corners
        v2_i32 ptarget = v2_i32_lerp(prev_vp.p, next_vp.p, n - prev_vp.i, next_vp.i - prev_vp.i);
        rv->p[n].p     = v2_i32_lerp(rv->p[n].p, ptarget, 1, 8);
    }
}

i32 grapplinghook_stretched_dt_len_abs(g_s *g, grapplinghook_s *h)
{
    if (h->state == 0 || h->state == GRAPPLINGHOOK_FLYING) return 0;

    i32 l_dt = (i32)wire_len_qx(g, &h->wire, 4) - (i32)h->len_max_q4;
    return max_i32(l_dt, 0);
}