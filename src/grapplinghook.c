// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "grapplinghook.h"
#include "game.h"

#define GRAPPLING_HOOK_GRAV 110

void grapplinghook_do_hook(g_s *g, grapplinghook_s *h)
{
    i32    clen_q4   = rope_len_q4(g, &h->rope);
    obj_s *ohero     = obj_from_obj_handle(h->o1);
    i32    herostate = hero_get_actual_state(g, ohero);
    if (herostate == HERO_ST_AIR) {
        clen_q4 = (clen_q4 * 245) >> 8;
    }
    h->rope.len_max_q4 = clamp_i32(clen_q4,
                                   HERO_ROPE_LEN_MIN,
                                   HERO_ROPE_LEN_MAX);
    snd_instance_stop_fade(h->throw_snd_iID, 25, 0);
}

void grapplinghook_create(g_s *g, grapplinghook_s *h, obj_s *ohero,
                          v2_i32 p, v2_i32 v)
{
    mclr(h, sizeof(grapplinghook_s));
    rope_s *r = &h->rope;
    rope_init(r);
    ohero->rope     = r;
    ohero->ropenode = r->head;
    h->rn           = r->tail;
    h->o1           = obj_handle_from_obj(ohero);
    h->v_q8         = v2_i16_from_i32(v);
    h->state        = GRAPPLINGHOOK_FLYING;
    h->p            = p;
    r->head->p      = p;
    r->tail->p      = p;
    r->len_max_q4   = HERO_ROPE_LEN_MAX;

    v2_i32 p_q8 = v2_i32_shl(p, 8);

    // "hint" the direction to the verlet sim
    for (i32 n = 0; n < ROPE_VERLET_N; n++) {
        rope_pt_s *pt = &r->ropept[n];

        i32 k    = max_i32(0, ROPE_VERLET_N - 1 - n);
        pt->p    = p_q8;
        pt->pp.x = p_q8.x - (v.x * k) / ROPE_VERLET_N;
        pt->pp.y = p_q8.y - (v.y * k) / ROPE_VERLET_N;
    }
}

void flyblob_on_unhooked(g_s *g, obj_s *o);

void grapplinghook_unhook_obj(g_s *g, obj_s *o)
{
    o->rope     = 0;
    o->ropenode = 0;

    if (o->on_hook) {
        o->on_hook(g, o, 0);
    }

    switch (o->ID) {
    case OBJID_HERO: hero_action_ungrapple(g, o); break;
    default: break;
    }
}

void grapplinghook_destroy(g_s *g, grapplinghook_s *h)
{
    if (!h->state) return;
    h->state = GRAPPLINGHOOK_INACTICE;

    obj_s *o1 = obj_from_obj_handle(h->o1);
    if (o1) {
        grapplinghook_unhook_obj(g, o1);
    }
    obj_s *o2 = obj_from_obj_handle(h->o2);
    if (o2) {
        grapplinghook_unhook_obj(g, o2);
    }
}

bool32 grapplinghook_step(g_s *g, grapplinghook_s *h, i32 sx, i32 sy);

static i32 sdamp   = 230;
static i32 sspring = 250;

void grapplinghook_update(g_s *g, grapplinghook_s *h)
{
#if PLTF_DEV_ENV
    i32 sda = sdamp;
    i32 spr = sspring;

    if (pltf_sdl_jkey(SDL_SCANCODE_J))
        sdamp -= 10;
    if (pltf_sdl_jkey(SDL_SCANCODE_U))
        sdamp += 10;
    if (pltf_sdl_jkey(SDL_SCANCODE_H))
        sspring -= 10;
    if (pltf_sdl_jkey(SDL_SCANCODE_K))
        sspring += 10;

    if (sda != sdamp || spr != sspring) {
        pltf_log("%i %i\n", sdamp, sspring);
    }
#endif

    if (h->state == GRAPPLINGHOOK_INACTICE) return;

    if (map_blocked_pt(g, h->p.x, h->p.y)) {
        grapplinghook_destroy(g, h);
        return;
    }

    switch (h->state) {
    case GRAPPLINGHOOK_FLYING: {
        h->p_q8 = v2_i16_add(h->p_q8, h->v_q8);
        h->v_q8.y += GRAPPLING_HOOK_GRAV;

        i32 dx = h->p_q8.x >> 8;
        i32 dy = h->p_q8.y >> 8;
        h->p_q8.x &= 0xFF;
        h->p_q8.y &= 0xFF;

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
                if (!grapplinghook_step(g, h, sx, 0))
                    break;
                e += py;
                x += sx;
            }
            if (e2 <= px) {
                if (!grapplinghook_step(g, h, 0, sy))
                    break;
                e += px;
                y += sy;
            }
        }
        break;
    }
    case GRAPPLINGHOOK_HOOKED_SOLID: {
        rec_i32 rsurround = {h->p.x - 1, h->p.y - 1, 3, 3};
        obj_s  *o2        = obj_from_obj_handle(h->o2);
        if (!overlap_rec(rsurround, obj_aabb(o2))) {
            if (map_blocked(g, rsurround)) {
                h->state = GRAPPLINGHOOK_HOOKED_TERRAIN;
            } else {
                grapplinghook_destroy(g, h);
            }
        }

        break;
    }
    case GRAPPLINGHOOK_HOOKED_TERRAIN: {
        rec_i32 rsurround = {h->p.x - 1, h->p.y - 1, 3, 3};
        if (!map_blocked(g, rsurround)) {
            grapplinghook_destroy(g, h);
        }
        break;
    }
    case GRAPPLINGHOOK_HOOKED_OBJ: {
        break;
    }
    }

    grapplinghook_rope_intact(g, h);
}

bool32 grapplinghook_rope_intact(g_s *g, grapplinghook_s *h)
{
    bool32  intact = 1;
    rope_s *r      = &h->rope;
    if (h->state == GRAPPLINGHOOK_FLYING ||
        h->state == GRAPPLINGHOOK_HOOKED_SOLID ||
        h->state == GRAPPLINGHOOK_HOOKED_TERRAIN) {
        if (map_blocked_pt(g, h->p.x, h->p.y)) {
            intact = 0;
        }
    }

    if (intact && !rope_is_intact(g, r)) {
        intact = 0;
    }

    if (!intact) {
        grapplinghook_destroy(g, h);
    }
    return intact;
}

void grapplinghook_animate(g_s *g, grapplinghook_s *h)
{
    if (!h->state) return;

    rope_s *r = &h->rope;
    rope_verletsim(g, &h->rope);
    ropenode_s *rn = ropenode_neighbour(r, h->rn);

    if (h->state == GRAPPLINGHOOK_FLYING) {
        v2_i32 rndt            = v2_i32_sub(h->rn->p, rn->p);
        v2_f32 v               = v2_f32_from_i32(rndt);
        h->anghist[h->n_ang++] = v;
        h->n_ang %= HEROHOOK_N_HIST;
    }
}

void grapplinghook_draw(g_s *g, grapplinghook_s *h, v2_i32 cam)
{
    gfx_ctx_s        ctx  = gfx_ctx_display();
    rope_s          *rope = &h->rope;
    grapplinghook_s *gh   = &g->ghook;
    v2_i32           ropepts[ROPE_VERLET_N];
    u32              n_ropepts = 0;

    for (u32 k = 1; k < ROPE_VERLET_N; k++) {
        v2_i32 p             = v2_i32_shr(rope->ropept[k].p, 8);
        ropepts[n_ropepts++] = v2_i32_add(p, cam);
    }

    spm_push();
    tex_s tex_hook = tex_create(PLTF_DISPLAY_W, PLTF_DISPLAY_H, 1, spm_allocator(), 0);
    tex_clr(tex_hook, GFX_COL_CLEAR);
    gfx_ctx_s ctxhook   = gfx_ctx_default(tex_hook);
    gfx_ctx_s ctxhookpt = ctx;
    ctxhookpt.pat       = gfx_pattern_4x4(B4(0111),
                                          B4(1010),
                                          B4(1101),
                                          B4(1010));

    for (u32 k = 1; k < n_ropepts; k++) {
        v2_i32 p1 = ropepts[k - 1];
        v2_i32 p2 = ropepts[k];
        gfx_lin_thick(ctx, p1, p2, GFX_COL_WHITE, 10);
    }
    for (u32 k = 1; k < n_ropepts; k++) {
        v2_i32 p1 = ropepts[k - 1];
        v2_i32 p2 = ropepts[k];
        gfx_lin_thick(ctx, p1, p2, GFX_COL_BLACK, 8);
    }
    for (u32 k = 1; k < n_ropepts; k++) {
        v2_i32 p1 = ropepts[k - 1];
        v2_i32 p2 = ropepts[k];
        gfx_lin_thick(ctxhookpt, p1, p2, GFX_COL_WHITE, 4);
    }

    // tex_merge_to_opaque_outlined_white(ctx.dst, tex_hook);
    spm_pop();
    if (h->state != GRAPPLINGHOOK_HOOKED_OBJ) {
        v2_f32 vhook = {0};
        for (i32 i = 0; i < HEROHOOK_N_HIST; i++) {
            vhook = v2f_add(vhook, gh->anghist[i]);
        }

        f32      ang    = (atan2f(vhook.y, vhook.x) * 16.f) / PI2_FLOAT;
        i32      imgy   = (i32)(ang + 16.f + 4.f) & 15;
        texrec_s trhook = {asset_tex(TEXID_HOOK), 0, imgy * 32, 32, 32};
        v2_i32   hpos   = {gh->p.x - 16, gh->p.y - 16};
        gfx_spr_tile_32x32(ctx, trhook, v2_i32_add(hpos, cam));
    }
}

bool32 grapplinghook_try_grab_terrain(g_s *g, grapplinghook_s *h,
                                      i32 sx, i32 sy)
{
    i32 x = h->p.x + sx;
    i32 y = h->p.y + sy;

    if (!(0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y))
        return 0;

    tile_s t = g->tiles[(x >> 4) + (y >> 4) * g->tiles_x];
    if (!tile_solid_pt(t.collision, x & 15, y & 15))
        return 0;

    switch (t.type & 63) {
    case TILE_TYPE_NONE:
        return 0;
    case TILE_TYPE_DARK_OBSIDIAN:
        // repelled
        grapplinghook_destroy(g, h);
        return 1;
    default:
        // hooked
        snd_play(SNDID_KLONG, 0.25f, rngr_f32(0.9f, 1.1f));
        h->state = GRAPPLINGHOOK_HOOKED_TERRAIN;
        grapplinghook_do_hook(g, h);
        particle_emit_ID(g, PARTICLE_EMIT_ID_HOOK_TERRAIN, h->p);
        return 1;
    }
    return 0;
}

bool32 grapplinghook_try_grab_obj(g_s *g, grapplinghook_s *h,
                                  obj_s *o, i32 sx, i32 sy)
{
    bool32 cangrab_solid = (h->state == GRAPPLINGHOOK_FLYING ||
                            h->state == GRAPPLINGHOOK_HOOKED_TERRAIN);
    bool32 cangrab_other = (h->state == GRAPPLINGHOOK_FLYING);

    i32     x = h->p.x + sx;
    i32     y = h->p.y + sy;
    v2_i32  p = {x, y};
    rec_i32 r = obj_aabb(o);

    if (!overlap_rec_pnt(r, p))
        return 0;

    i32 res = 0;
    if (cangrab_solid && (o->flags & OBJ_FLAG_SOLID)) {
        h->o2       = obj_handle_from_obj(o);
        o->ropenode = h->rn;
        o->rope     = &h->rope;
        h->state    = GRAPPLINGHOOK_HOOKED_SOLID;
        res         = GRAPPLINGHOOK_HOOKED_SOLID;
        grapplinghook_do_hook(g, h);
        particle_emit_ID(g, PARTICLE_EMIT_ID_HOOK_TERRAIN, h->p);
    } else if (cangrab_other) {
        if (o->flags & OBJ_FLAG_HOOKABLE) {
            h->state  = GRAPPLINGHOOK_HOOKED_OBJ;
            v2_i32 pc = obj_pos_center(o);
            v2_i32 dt = v2_i32_sub(pc, h->rn->p);
            ropenode_move(g, &h->rope, h->rn, dt.x, dt.y);
            h->p        = pc;
            h->o2       = obj_handle_from_obj(o);
            o->rope     = &h->rope;
            o->ropenode = h->rn;
            res         = GRAPPLINGHOOK_HOOKED_OBJ;
            grapplinghook_do_hook(g, h);

            if (o->on_hook) {
                o->on_hook(g, o, 1);
            }
        } else if (o->flags & OBJ_FLAG_DESTROY_HOOK) {
            grapplinghook_destroy(g, h);
            res = -1;
        }
    }

    return res;
}

bool32 grapplinghook_step(g_s *g, grapplinghook_s *h, i32 sx, i32 sy)
{
    if (grapplinghook_try_grab_terrain(g, h, sx, sy))
        return 0;

    for (obj_each(g, o)) {
        i32 r = grapplinghook_try_grab_obj(g, h, o, sx, sy);
        if (r) return 0;
    }

    ropenode_move(g, &h->rope, h->rn, sx, sy);
    h->p.x += sx;
    h->p.y += sy;
    if (h->p.x < 0 || h->p.x >= g->pixel_x ||
        h->p.y < 0 || h->p.y >= g->pixel_y) {
        grapplinghook_destroy(g, h);
        return 0;
    }
    return 1;
}

v2_i32 rope_recalc_v(g_s *g, rope_s *r, ropenode_s *rn,
                     v2_i32 subpos_q8, v2_i32 v_q8)
{
    if (!r || !rn) return v_q8;

    i32 len_q4     = rope_len_q4(g, r);
    i32 len_max_q4 = r->len_max_q4;
    i32 dt_len     = len_q4 - len_max_q4;
    if (dt_len <= 0) return v_q8; // rope is not stretched

    ropenode_s *rprev = rn->next ? rn->next : rn->prev;
    assert(rprev);

    v2_i32 ropedt = v2_i32_sub(rn->p, rprev->p);
    v2_i32 dt_q4  = v2_i32_add(v2_i32_shl(ropedt, 4), v2_i32_shr(subpos_q8, 4));

    // damping force
    v2_i32 fdamp = {0};
    if (v2_i32_dot(ropedt, v_q8) > 0) {
        v2_i32 vrad = project_pnt_line(v_q8, CINIT(v2_i32){0}, dt_q4);
        fdamp       = v2_i32_mulq(vrad, sdamp, 8);
    }

    // spring force
    i32    fspring_scalar = (dt_len * sspring) >> 8;
    v2_i32 fspring        = v2_i32_setlen(dt_q4, fspring_scalar);
    v2_i32 frope          = v2_i32_add(fdamp, fspring);
    v2_i32 vel_new        = v2_i32_sub(v_q8, frope);
    return vel_new;
}

v2_i32 grapplinghook_vlaunch_from_angle(i32 a_q16, i32 vel)
{
    v2_i32 v = {(-vel * sin_q15(a_q16 << 1)) / 32768,
                (-vel * cos_q15(a_q16 << 1)) / 32768};
    return v;
}

i32 grapplinghook_pulling_force_hero(g_s *g)
{
    grapplinghook_s *gh = &g->ghook;
    rope_s          *r  = &gh->rope;
    if (gh->state == GRAPPLINGHOOK_INACTICE ||
        gh->state == GRAPPLINGHOOK_FLYING) return 0;

    i32 rl  = rope_len_q4(g, r);
    i32 rlm = r->len_max_q4;
    if (rl <= rlm) return 0;

    obj_s      *o         = obj_get_hero(g);
    ropenode_s *rn1       = o->ropenode;
    ropenode_s *rn2       = ropenode_neighbour(r, rn1);
    v2_i32      v12       = v2_i32_sub(rn2->p, rn1->p);
    v2_i32      v_hero    = v2_i32_from_i16(o->v_q8);
    v2_i32      v_tang    = {v12.y, -v12.x};
    v2_i32      v_proj    = project_pnt_dir(v_hero, v_tang);
    i32         rad       = v2_i32_len(v12);
    i32         f_grav    = (-HERO_GRAVITY * v12.y) / rad; // component of gravity
    i32         f_centrip = v2_i32_lensq(v_proj) / (rad << 8);
    i32         f_stretch = (2 * (rl - rlm));
    return max_i32(f_grav + f_centrip + f_stretch, 0);
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

void hero_hook_preview_throw(g_s *g, obj_s *ohero, v2_i32 cam)
{
    i32    str = hero_aim_throw_strength(ohero);
    i32    ang = g->hero.hook_aim;
    v2_i32 p   = v2_i32_shl(obj_pos_center(ohero), 8);
    v2_i32 v   = grapplinghook_vlaunch_from_angle(ang, str);
    v2_i32 pts[128];
    i32    n_pts = 0;
    pts[n_pts++] = p;
    i32 length   = 0;

    for (i32 n = 0; n < 64; n++) {
        p.x += v.x;
        p.y += v.y;
        v.y += GRAPPLING_HOOK_GRAV;
        length += v2_i32_distance(pts[n_pts - 1], p);
        pts[n_pts++] = p;
    }

    gfx_ctx_s ctx = gfx_ctx_display();

#define HOOK_PREVIEW_DST 3500

    i32    d = (pltf_cur_tick() * 700) % HOOK_PREVIEW_DST;
    v2_i32 pa;
    if (pnt_along_array(pts, n_pts, d, &pa)) {
        d += HOOK_PREVIEW_DST;
        v2_i32 pb;
        while (pnt_along_array(pts, n_pts, d, &pb)) {
            v2_i32 p0 = v2_i32_add(cam, v2_i32_shr(pa, 8));
            gfx_cir_fill(ctx, p0, 6, GFX_COL_BLACK);
            pa = pb;
            d += HOOK_PREVIEW_DST;
        }
    }
}
