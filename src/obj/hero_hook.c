// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero_hook.h"
#include "game.h"

void   hook_on_animate(game_s *g, obj_s *o);
bool32 hook_move(game_s *g, obj_s *o, v2_i32 dt, obj_s **ohook);
void   hook_verlet_sim(game_s *g, obj_s *o, rope_s *r);
bool32 hook_can_attach(game_s *g, rec_i32 r, obj_s **ohook);

obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8)
{
    obj_s *o      = obj_create(g);
    o->ID         = OBJ_ID_HOOK;
    o->on_animate = hook_on_animate;
    obj_tag(g, o, OBJ_TAG_HOOK);
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_SPRITE;
    o->n_sprites         = 1;
    sprite_simple_s *spr = &o->sprites[0];
    spr->trec            = asset_texrec(TEXID_HOOK, 0, 0, 32, 32);
    spr->offs.x          = -16 + 4;
    spr->offs.y          = -16 + 4;

    o->w            = 8;
    o->h            = 8;
    o->pos.x        = p.x - o->w / 2;
    o->pos.y        = p.y - o->h / 2;
    o->drag_q8.x    = 256;
    o->drag_q8.y    = 256;
    o->gravity_q8.y = 70;
    o->vel_q8       = v_q8;
    o->vel_cap_q8.x = 2500;
    o->vel_cap_q8.y = 2500;

    rope_init(r);
    rope_set_len_max_q4(r, hero_max_rope_len_q4(g));
    r->tail->p  = p;
    r->head->p  = p;
    o->rope     = r;
    o->ropenode = o->rope->tail;

    herohook_s *h = (herohook_s *)o->mem;

    for (int i = 0; i < HEROHOOK_N_HIST; i++) {
        h->anghist[i] = (v2_f32){(f32)v_q8.x, (f32)v_q8.y};
    }
    return o;
}

void hook_verlet_sim(game_s *g, obj_s *o, rope_s *r)
{
    typedef struct {
        i32    i;
        v2_i32 p;
    } verlet_pos_s;

    hero_s *hd = &g->hero_mem;

    // calculated current length in Q8
    u32          ropelen_q8 = 1 + (rope_length_q4(g, r) << 4); // +1 to avoid div 0
    i32          n_vpos     = 0;
    verlet_pos_s vpos[64]   = {0};
    verlet_pos_s vp_beg     = {0, v2_shl(r->tail->p, 8)};
    vpos[n_vpos++]          = vp_beg;

    u32 dista = 0;
    for (ropenode_s *r1 = r->tail, *r2 = r1->prev; r2; r1 = r2, r2 = r2->prev) {
        dista += v2_len(v2_shl(v2_sub(r1->p, r2->p), 8));
        i32 i = (dista * ROPE_VERLET_N) / ropelen_q8;
        if (1 <= i && i < ROPE_VERLET_N - 1) {
            verlet_pos_s vp = {i, v2_shl(r2->p, 8)};
            vpos[n_vpos++]  = vp;
        }
    }

    verlet_pos_s vp_end = {ROPE_VERLET_N - 1, v2_shl(r->head->p, 8)};
    vpos[n_vpos++]      = vp_end;

    u32 ropelen_max_q8 = r->len_max_q4 << 4;
    f32 len_ratio      = min_f(1.f, (f32)ropelen_q8 / (f32)ropelen_max_q8);
    i32 ll_q8          = (i32)((f32)ropelen_max_q8 * len_ratio) / ROPE_VERLET_N;

    for (i32 n = 1; n < ROPE_VERLET_N - 1; n++) {
        hook_pt_s *pt  = &hd->hookpt[n];
        v2_i32     tmp = pt->p;
        pt->p.x += (pt->p.x - pt->pp.x);
        pt->p.y += (pt->p.y - pt->pp.y) + ROPE_VERLET_GRAV;
        pt->pp = tmp;
    }

    for (i32 k = 0; k < ROPE_VERLET_IT; k++) {
        for (int n = 1; n < ROPE_VERLET_N; n++) {
            hook_pt_s *p1 = &hd->hookpt[n - 1];
            hook_pt_s *p2 = &hd->hookpt[n];

            v2i dt = v2_sub(p1->p, p2->p);
            v2f df = {(f32)dt.x, (f32)dt.y};
            f32 dl = v2f_len(df);
            i32 dd = (i32)(dl + .5f) - ll_q8;

            if (dd <= 1) continue;
            dt    = v2_setlenl(dt, dl, dd >> 1);
            p1->p = v2_sub(p1->p, dt);
            p2->p = v2_add(p2->p, dt);
        }

        for (int n = n_vpos - 1; 0 <= n; n--) {
            hd->hookpt[vpos[n].i].p = vpos[n].p;
        }
    }

    if (0.95f <= len_ratio) { // straighten rope
        for (int n = 1; n < ROPE_VERLET_N - 1; n++) {
            bool32 contained = 0;
            for (int i = 0; i < n_vpos; i++) {
                if (vpos[i].i == n) {
                    contained = 1; // is fixed to corner already
                    break;
                }
            }
            if (contained) continue;

            // figure out previous and next corner of verlet particle
            verlet_pos_s prev_vp = {-1};
            verlet_pos_s next_vp = {ROPE_VERLET_N};

            for (int i = 0; i < n_vpos; i++) {
                verlet_pos_s vp = vpos[i];
                if (prev_vp.i < vp.i && vp.i < n) {
                    prev_vp = vpos[i];
                }
                if (vp.i < next_vp.i && n < vp.i) {
                    next_vp = vpos[i];
                }
            }

            if (!(0 <= prev_vp.i && next_vp.i < ROPE_VERLET_N)) continue;

            // lerp position of particle towards straight line between corners
            v2_i32 ptarget  = v2_lerp(prev_vp.p, next_vp.p,
                                      n - prev_vp.i,
                                      next_vp.i - prev_vp.i);
            hd->hookpt[n].p = v2_lerp(hd->hookpt[n].p, ptarget, 1, 4);
        }
        return;
    }
}

void hook_on_animate(game_s *g, obj_s *o)
{
    assert(o->rope && o->rope->head && o->rope->tail);

    rope_s     *r  = o->rope;
    herohook_s *h  = (herohook_s *)o->mem;
    ropenode_s *rn = ropenode_neighbour(r, o->ropenode);
    hero_s     *hd = &g->hero_mem;
    rope_update(g, r);
    hook_verlet_sim(g, o, r);

    o->n_sprites = 0;

    if (o->substate) {
        o->n_sprites = 1;
    }

    if (o->state) return;

    v2i rndt               = v2_sub(o->ropenode->p, rn->p);
    v2f v                  = {(f32)rndt.x, (f32)rndt.y};
    h->anghist[h->n_ang++] = v;
    h->n_ang %= HEROHOOK_N_HIST;

    for (int i = 0; i < HEROHOOK_N_HIST; i++) {
        v = v2f_add(v, h->anghist[i]);
    }

    f32 ang  = (atan2f(v.y, v.x) * 16.f) / PI2_FLOAT;
    int imgy = (int)(ang + 16.f + 4.f) & 15;

    o->sprites[0].trec.r.y = imgy * 32;
}

bool32 hook_can_attach(game_s *g, rec_i32 r, obj_s **ohook)
{
    for (obj_each(g, it)) {
        // if (!(it->flags & OBJ_FLAG_HOOKABLE)) continue;
        if (!overlap_rec(r, obj_aabb(it))) continue;
        if (!(it->flags & OBJ_FLAG_SOLID)) continue;
        if (ohook) {
            *ohook = it;
        }
        return 1;
    }

    return tile_map_hookable(g, r);
}

bool32 hook_move(game_s *g, obj_s *o, v2_i32 dt, obj_s **ohook)
{
    for (int m = abs_i(dt.x), sx = sgn_i(dt.x); 0 < m; m--) {
        rec_i32 r = obj_aabb(o);
        v2_i32  v = {sx, 0};
        r.x += sx;

        if (!game_traversable(g, r)) {
            o->bumpflags |= 0 < sx ? OBJ_BUMPED_X_POS : OBJ_BUMPED_X_NEG;
            return hook_can_attach(g, r, ohook);
        }

        actor_move_by_internal(g, o, v);
        rec_i32 hookr = {r.x - 1, r.y - 1, r.w + 2, r.h + 2};
        if (hook_can_attach(g, hookr, ohook)) {
            return 1;
        }
    }

    for (int m = abs_i(dt.y), sy = sgn_i(dt.y); 0 < m; m--) {
        rec_i32 r = obj_aabb(o);
        v2_i32  v = {0, sy};
        r.y += sy;

        if (!game_traversable(g, r)) {
            o->bumpflags |= 0 < sy ? OBJ_BUMPED_Y_POS : OBJ_BUMPED_Y_NEG;
            return hook_can_attach(g, r, ohook);
        }

        actor_move_by_internal(g, o, v);
        rec_i32 hookr = {r.x - 1, r.y - 1, r.w + 2, r.h + 2};
        if (hook_can_attach(g, hookr, ohook)) {
            return 1;
        }
    }

    rec_i32 hookrend = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};
    return hook_can_attach(g, hookrend, ohook);
}

void hook_update(game_s *g, obj_s *h, obj_s *hook)
{
    rope_s *r = hook->rope;

    if (!rope_intact(g, r)) {
        hero_unhook(g, h);
        return;
    }

    obj_s *tohook = NULL;
    if (hook->state) {
        // check if still attached
        rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};

        if (hook_can_attach(g, hookrec, &tohook)) {
            obj_s *solid;
            if (obj_try_from_obj_handle(hook->linked_solid, &solid) &&
                !overlap_rec(hookrec, obj_aabb(solid))) {
                hook->linked_solid.o = NULL;
            }
        } else {
            hook->state          = 0;
            hook->linked_solid.o = NULL;
        }
    } else {
        v2_i32 hookp = hook->pos;
        obj_apply_movement(hook);
        bool32  attach  = hook_move(g, hook, hook->tomove, &tohook);
        rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};
        hook->tomove.x  = 0;
        hook->tomove.y  = 0;

        if (attach) {
            i32 mlen_q4 = hero_max_rope_len_q4(g);
            i32 clen_q4 = rope_length_q4(g, r);

            i32 herostate = hero_determine_state(g, h, (hero_s *)h->mem);
            if (herostate == HERO_STATE_AIR) {
                clen_q4 = (clen_q4 * 240) >> 8;
            }
            i32 newlen_q4 = clamp_i(clen_q4, HERO_ROPE_LEN_MIN_JUST_HOOKED, mlen_q4);
            rope_set_len_max_q4(r, newlen_q4);
            snd_play_ext(SNDID_HOOK_ATTACH, 1.f, 1.f);
            hook->state    = 1;
            hook->vel_q8.x = 0;
            hook->vel_q8.y = 0;

            for (obj_each(g, solid)) {
                if (!overlap_rec(hookrec, obj_aabb(solid))) continue;
                if (solid->ID == OBJ_ID_CRUMBLEBLOCK) {
                    crumbleblock_on_hooked(solid);
                }
                if (!(solid->flags & OBJ_FLAG_SOLID)) continue;

                i32 kk             = overlap_rec(hookrec, obj_aabb(solid));
                hook->linked_solid = obj_handle_from_obj(solid);
            }
        } else {
            if (hook->bumpflags & OBJ_BUMPED_X) {
                if (abs_i(hook->vel_q8.x) > 700) {
                    snd_play_ext(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
                }
                hook->vel_q8.x = -hook->vel_q8.x / 3;
            }
            if (hook->bumpflags & OBJ_BUMPED_Y) {
                if (abs_i(hook->vel_q8.y) > 700) {
                    snd_play_ext(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
                }
                hook->vel_q8.y = -hook->vel_q8.y / 3;
            }
            if (obj_grounded(g, hook)) {
                hook->vel_q8.x = (hook->vel_q8.x * 240) >> 8;
            }
        }
    }
    hook->bumpflags = 0;

    rec_i32 r_room = {0, 0, g->pixel_x, g->pixel_y};
    if (!overlap_rec_pnt(r_room, obj_pos_center(hook))) {
        hero_unhook(g, h);
        return;
    }

    rope_update(g, &g->hero_mem.rope);
    if (hook->state) {
        h->vel_q8 = obj_constrain_to_rope(g, h);
    } else {
#if 1
        hook->vel_q8 = obj_constrain_to_rope(g, hook);
#else
        v2_i32 vhero = obj_constrain_to_rope(g, h);
        v2_i32 vhook = obj_constrain_to_rope(g, hook);

        h->vel_q8    = v2_lerp(h->vel_q8, vhero, 1, 2);
        hook->vel_q8 = v2_lerp(hook->vel_q8, vhook, 1, 2);
#endif
    }
}

void hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook)
{
    obj_delete(g, ohook);
    ohero->ropenode         = NULL;
    ohero->rope             = NULL;
    ohook->rope             = NULL;
    ohook->ropenode         = NULL;
    g->hero_mem.rope_active = 0;
}