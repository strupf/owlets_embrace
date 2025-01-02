// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero_hook.h"
#include "game.h"

enum {
    HOOK_ATTACH_NONE,
    HOOK_ATTACH_SOLID,
    HOOK_ATTACH_OBJ,
    HOOK_ATTACH_COLLISION,
};

enum {
    HOOK_STATE_FREE,
    HOOK_STATE_ATTACHED,
};

i32 hook_move(g_s *g, obj_s *o, v2_i32 dt, obj_s **ohook);
i32 hook_can_attach(g_s *g, obj_s *o, rec_i32 r, obj_s **ohook);

obj_s *hook_create(g_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJ_ID_HOOK;
    o->on_animate = hook_on_animate;
    o->on_update  = hook_update;
    obj_tag(g, o, OBJ_TAG_HOOK);
    o->flags          = OBJ_FLAG_SPRITE;
    o->n_sprites      = 1;
    obj_sprite_s *spr = &o->sprites[0];
    spr->trec         = asset_texrec(TEXID_HOOK, 0, 0, 32, 32);
    spr->offs.x       = -16 + 4;
    spr->offs.y       = -16 + 4;
    o->w              = 4;
    o->h              = 4;
    o->pos.x          = p.x - o->w / 2;
    o->pos.y          = p.y - o->h / 2;
    o->v_q8           = v2_i16_from_i32(v_q8);

    rope_init(r);
    r->len_max_q4 = HERO_ROPE_LEN_LONG;
    r->tail->p    = p;
    r->head->p    = p;
    o->rope       = r;
    o->ropenode   = r->tail;

    herohook_s *h = (herohook_s *)o->mem;

    for (int i = 0; i < HEROHOOK_N_HIST; i++) {
        h->anghist[i] = (v2_f32){(f32)v_q8.x, (f32)v_q8.y};
    }
    return o;
}

void hook_on_animate(g_s *g, obj_s *o)
{
    rope_s *r = o->rope;
    assert(r && r->head && r->tail);

    herohook_s *h  = (herohook_s *)o->mem;
    ropenode_s *rn = ropenode_neighbour(r, o->ropenode);
    hero_s     *hd = &g->hero_mem;

    if (o->state == HOOK_STATE_FREE) {
        v2_i32 rndt            = v2_sub(o->ropenode->p, rn->p);
        v2_f32 v               = v2_f32_from_i32(rndt);
        h->anghist[h->n_ang++] = v;
        h->n_ang %= HEROHOOK_N_HIST;

        for (i32 i = 0; i < HEROHOOK_N_HIST; i++) {
            v = v2f_add(v, h->anghist[i]);
        }

        f32 ang  = (atan2f(v.y, v.x) * 16.f) / PI2_FLOAT;
        i32 imgy = (i32)(ang + 16.f + 4.f) & 15;

        o->sprites[0].trec.r.y = imgy * 32;
    }
}

void hookplant_on_hook(obj_s *o);

i32 hook_can_attach(g_s *g, obj_s *o, rec_i32 r, obj_s **ohook)
{
    assert(o && o->ropenode);
    v2_i32 ph = o->ropenode->p;
    for (obj_each(g, it)) {
        rec_i32 rit = obj_aabb(it);
        if (it->mass) {
            if (overlap_rec(r, rit)) {
                if (ohook) {
                    *ohook = it;
                }
                return HOOK_ATTACH_SOLID;
            }

        } else if (it->flags & OBJ_FLAG_HOOKABLE) {
            if (overlap_rec_pnt(rit, ph)) {
                if (ohook) {
                    *ohook = it;
                }
                return HOOK_ATTACH_OBJ;
            }
        }
    }

    if (tile_map_hookable(g, r)) {
        return HOOK_ATTACH_SOLID;
    }
    return 0;
}

i32 hook_move(g_s *g, obj_s *o, v2_i32 dt, obj_s **ohook)
{
    for (i32 m = abs_i32(dt.x), sx = sgn_i32(dt.x); 0 < m; m--) {
        rec_i32 r = obj_aabb(o);
        v2_i32  v = {sx, 0};
        r.x += sx;

        if (!map_traversable(g, r)) {
            o->bumpflags |= 0 < sx ? OBJ_BUMP_X_POS : OBJ_BUMP_X_NEG;
            return hook_can_attach(g, o, r, ohook);
        }
        obj_step(g, o, sx, 0, 0, 0);
        rec_i32 hookr = {r.x - 1, r.y - 1, r.w + 2, r.h + 2};
        i32     res   = hook_can_attach(g, o, hookr, ohook);
        if (res) {
            return res;
        }
    }

    for (i32 m = abs_i32(dt.y), sy = sgn_i32(dt.y); 0 < m; m--) {
        rec_i32 r = obj_aabb(o);
        v2_i32  v = {0, sy};
        r.y += sy;

        if (!map_traversable(g, r)) {
            o->bumpflags |= 0 < sy ? OBJ_BUMP_Y_POS : OBJ_BUMP_Y_NEG;
            return hook_can_attach(g, o, r, ohook);
        }

        obj_step(g, o, 0, sy, 0, 0);
        rec_i32 hookr = {r.x - 1, r.y - 1, r.w + 2, r.h + 2};
        i32     res   = hook_can_attach(g, o, hookr, ohook);
        if (res) {
            return res;
        }
    }

    rec_i32 hookrend = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};
    return hook_can_attach(g, o, hookrend, ohook);
}

i32 hook_update_nonhooked(g_s *g, obj_s *hook)
{
    obj_s  *h = obj_get_tagged(g, OBJ_TAG_HERO);
    rope_s *r = hook->rope;

    hook->v_q8.y += 110;
    hook->subpos_q8 = v2_i16_add(hook->subpos_q8, hook->v_q8);
    v2_i32 dtpos    = v2_i32_from_i16(v2_i16_shr(hook->subpos_q8, 8));
    hook->subpos_q8.x &= 255;
    hook->subpos_q8.y &= 255;

    obj_s *tohook = 0;
    i32    attach = hook_move(g, hook, dtpos, &tohook);
    if (attach != HOOK_ATTACH_NONE) {
        i32 mlen_q4   = HERO_ROPE_LEN_LONG;
        i32 clen_q4   = rope_len_q4(g, r);
        i32 herostate = hero_get_actual_state(g, h);
        if (herostate == HERO_ST_AIR) {
            clen_q4 = (clen_q4 * 240) >> 8;
        }
        i32 newlen_q4 = clamp_i32(clen_q4, HERO_ROPE_LEN_MIN_JUST_HOOKED, mlen_q4);
        r->len_max_q4 = newlen_q4;
    }

    switch (attach) {
    case HOOK_ATTACH_COLLISION: break;
    case HOOK_ATTACH_NONE:
        if (hook->bumpflags & OBJ_BUMP_X) {
            if (abs_i32(hook->v_q8.x) > 700) {
                snd_play(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
            }
            obj_vx_q8_mul(hook, -85);
        }
        if (hook->bumpflags & OBJ_BUMP_Y) {
            if (abs_i32(hook->v_q8.y) > 700) {
                snd_play(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
            }
            obj_vy_q8_mul(hook, -85);
        }
        if (obj_grounded(g, hook)) {
            obj_vx_q8_mul(hook, 240);
        }
        break;
    case HOOK_ATTACH_SOLID: {
        hook->v_q8.x = 0;
        hook->v_q8.y = 0;
        hook->state  = HOOK_STATE_ATTACHED;
        snd_play(SNDID_HOOK_ATTACH, 0.2f, 1.f);

        rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
        for (obj_each(g, solid)) {
            if (!overlap_rec(hookrec, obj_aabb(solid))) continue;
            if (solid->ID == OBJ_ID_CRUMBLEBLOCK) {
                crumbleblock_on_hooked(solid);
            }
            if (solid->mass <= 0) continue;

            hook->linked_solid = obj_handle_from_obj(solid);
        }

        ropenode_s *rnn = ropenode_neighbour(hook->rope, hook->ropenode);
        v2_i32      pos = obj_pos_center(hook);
        pos             = v2_add(pos, v2_setlen(v2_sub(rnn->p, pos), 4));
        pos.x -= 32;
        pos.y -= 32;
        rec_i32 rdecal = {0, 64 * 9, 64, 64};

        i32 dflip = rngr_i32(0, 3);
        spritedecal_create(g, 0x20000, NULL, pos, TEXID_EXPLOSIONS, rdecal, 18, 6, dflip);
        break;
    }
    case HOOK_ATTACH_OBJ: {
        v2_i32 ctr = obj_pos_center(tohook);
        v2_i32 dt  = v2_sub(ctr, hook->ropenode->p);
        ropenode_move(g, r, hook->ropenode, dt.x, dt.y);
        tohook->rope     = r;
        tohook->ropenode = hook->ropenode;
        g->hero_mem.hook = obj_handle_from_obj(tohook);
        obj_delete(g, hook);
        obj_on_hooked(g, tohook);
        break;
    }
    }
    return attach;
}

void hook_update_hooked_solid(g_s *g, obj_s *hook)
{
    obj_s  *h = obj_get_tagged(g, OBJ_TAG_HERO);
    rope_s *r = hook->rope;

    // check if still attached
    rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};

    obj_s *tohook = 0;
    if (hook_can_attach(g, hook, hookrec, &tohook) == HOOK_ATTACH_SOLID) {
        obj_s *solid;
        if (obj_try_from_obj_handle(hook->linked_solid, &solid) &&
            !overlap_rec(hookrec, obj_aabb(solid))) {
            hook->state          = HOOK_STATE_FREE;
            hook->linked_solid.o = 0;
        }
    } else {
        hook->state          = HOOK_STATE_FREE;
        hook->linked_solid.o = 0;
    }
}

void hook_update(g_s *g, obj_s *hook)
{
    obj_s *h = obj_get_tagged(g, OBJ_TAG_HERO);

    switch (hook->state) {
    case HOOK_STATE_FREE:
        i32 r = hook_update_nonhooked(g, hook);
        if (r == HOOK_ATTACH_COLLISION) {
            hook_destroy(g, h, hook);
            return;
        }
        if (r) return;
        break;
    case HOOK_STATE_ATTACHED: {
        hook_update_hooked_solid(g, hook);
        break;
    }
    }

    hook->bumpflags = 0;

    rec_i32 r_room = {0, 0, g->pixel_x, g->pixel_y};
    if (!overlap_rec_pnt(r_room, obj_pos_center(hook))) {
        hook_destroy(g, h, hook);
    }
}

void hook_destroy(g_s *g, obj_s *ohero, obj_s *ohook)
{
    if (ohook->ID == OBJ_ID_HOOK) {
        obj_delete(g, ohook);
    }

    g->rope.active  = 0;
    ohero->ropenode = 0;
    ohero->rope     = 0;
    ohook->rope     = 0;
    ohook->ropenode = 0;
}

bool32 hook_is_attached(obj_s *o)
{
    assert(o->ID == OBJ_ID_HOOK);
    return o->state == HOOK_STATE_ATTACHED;
}

i32 hero_hook_pulling_force(g_s *g, obj_s *ohero)
{
    rope_s *r = ohero->rope;
    if (!r) return 0;
    i32 rl  = rope_len_q4(g, r);
    i32 rlm = r->len_max_q4;
    if (rl < rlm) return 0;

    ropenode_s *rn1 = ohero->ropenode;
    ropenode_s *rn2 = ropenode_neighbour(r, rn1);
    v2_i32      v12 = v2_sub(rn2->p, rn1->p);

    v2_i32 v_hero        = v2_i32_from_i16(ohero->v_q8);
    v2_i32 tang          = {v12.y, -v12.x};
    v2_i32 v_proj        = project_pnt_line(v_hero, (v2_i32){0}, tang);
    i32    rad           = v2_len(v12);
    i32    f_grav        = (-HERO_GRAVITY * v12.y) / rad; // component of gravity
    i32    f_centripetal = v2_lensq(v_proj) / (rad << 8);
    i32    f_stretch     = (2 * (rl - rlm));
    return max_i32(f_grav + f_centripetal + f_stretch, 0);
}

bool32 pnt_along_array(v2_i32 *pt, i32 n_pt, i32 dst, v2_i32 *p_out)
{
    i32 dst_total = 0;
    for (i32 n = 1; n < n_pt; n++) {
        v2_i32 p0            = pt[n - 1];
        v2_i32 p1            = pt[n];
        i32    d             = v2_distance(p0, p1);
        i32    dst_total_new = dst_total + d;

        if (dst_total <= dst && dst <= dst_total_new) {
            v2_i32 p = v2_lerp(p0, p1, dst - dst_total, d);
            *p_out   = p;
            return 1;
        }

        dst_total = dst_total_new;
    }
    return 0;
}

void hero_hook_preview_throw(g_s *g, obj_s *ohero, v2_i32 cam)
{
    spm_push();
    i32     ang   = -inp_crank_q16();
    v2_i32  p     = v2_shl(obj_pos_center(ohero), 8);
    v2_i32  v     = hook_launch_vel_from_angle(ang, 2500);
    v2_i32 *pts   = spm_alloct(v2_i32, 256);
    i32     n_pts = 0;

    pts[n_pts++] = p;
    i32 length   = 0;

    for (i32 n = 0; n < 128; n++) {
        p.x += v.x;
        p.y += v.y;
        v.y += 60;
        length += v2_distance(pts[n_pts - 1], p);
        pts[n_pts++] = p;
    }

    gfx_ctx_s ctx = gfx_ctx_display();

#define HOOK_PREVIEW_DST 3500

    i32    d = (pltf_cur_tick() * 500) % HOOK_PREVIEW_DST;
    v2_i32 pa;
    if (pnt_along_array(pts, n_pts, d, &pa)) {
        d += HOOK_PREVIEW_DST;
        v2_i32 pb;
        while (pnt_along_array(pts, n_pts, d, &pb)) {
            v2_i32 p0 = v2_add(cam, v2_shr(pa, 8));
            gfx_cir_fill(ctx, p0, 5, GFX_COL_BLACK);
            pa = pb;
            d += HOOK_PREVIEW_DST;
        }
    }
    spm_pop();
}

v2_i32 hook_launch_vel_from_angle(i32 a_q16, i32 vel)
{
    v2_i32 v = {(-vel * sin_q16(a_q16 << 2)) / 65536,
                (-vel * cos_q16(a_q16 << 2)) / 65536};
    return v;
}