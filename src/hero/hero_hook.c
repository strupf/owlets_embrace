// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "hero_hook.h"
#include "game.h"

enum {
    HOOK_ATTACH_NONE,
    HOOK_ATTACH_SOLID,
    HOOK_ATTACH_OBJ,
};

enum {
    HOOK_STATE_FREE,
    HOOK_STATE_ATTACHED,
};

void hook_on_animate(game_s *g, obj_s *o);
i32  hook_move(game_s *g, obj_s *o, v2_i32 dt, obj_s **ohook);
i32  hook_can_attach(game_s *g, obj_s *o, rec_i32 r, obj_s **ohook);

obj_s *hook_create(game_s *g, rope_s *r, v2_i32 p, v2_i32 v_q8)
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
    o->drag_q8.x      = 256;
    o->drag_q8.y      = 256;
    o->grav_q8.y      = 60;
    o->v_q8           = v2_i16_from_i32(v_q8, 0);
    o->v_cap_y_q8_pos = 2500;

    rope_init(r);
    r->len_max_q4 = hero_max_rope_len_q4(g);
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

void hook_on_animate(game_s *g, obj_s *o)
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

i32 hook_can_attach(game_s *g, obj_s *o, rec_i32 r, obj_s **ohook)
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

    return tile_map_hookable(g, r);
}

i32 hook_move(game_s *g, obj_s *o, v2_i32 dt, obj_s **ohook)
{
    for (i32 m = abs_i32(dt.x), sx = sgn_i32(dt.x); 0 < m; m--) {
        rec_i32 r = obj_aabb(o);
        v2_i32  v = {sx, 0};
        r.x += sx;

        if (!map_traversable(g, r)) {
            o->bumpflags |= 0 < sx ? OBJ_BUMPED_X_POS : OBJ_BUMPED_X_NEG;
            return hook_can_attach(g, o, r, ohook);
        }

        obj_step_x(g, o, sx, 0, 0);
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
            o->bumpflags |= 0 < sy ? OBJ_BUMPED_Y_POS : OBJ_BUMPED_Y_NEG;
            return hook_can_attach(g, o, r, ohook);
        }

        obj_step_y(g, o, sy, 0, 0);
        rec_i32 hookr = {r.x - 1, r.y - 1, r.w + 2, r.h + 2};
        i32     res   = hook_can_attach(g, o, hookr, ohook);
        if (res) {
            return res;
        }
    }

    rec_i32 hookrend = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};
    return hook_can_attach(g, o, hookrend, ohook);
}

bool32 hook_update_nonhooked(game_s *g, obj_s *hook)
{
    obj_s  *h = obj_get_tagged(g, OBJ_TAG_HERO);
    rope_s *r = hook->rope;

    obj_apply_movement(hook);

    obj_s *tohook = NULL;
    i32    attach = hook_move(g, hook, v2_i32_from_i16(hook->tomove), &tohook);
    if (attach != HOOK_ATTACH_NONE) {
        i32 mlen_q4   = hero_max_rope_len_q4(g);
        i32 clen_q4   = rope_len_q4(g, r);
        i32 herostate = hero_determine_state(g, h, (hero_s *)h->mem);
        if (herostate == HERO_STATE_AIR) {
            clen_q4 = (clen_q4 * 240) >> 8;
        }
        i32 newlen_q4 = clamp_i32(clen_q4, HERO_ROPE_LEN_MIN_JUST_HOOKED, mlen_q4);
        r->len_max_q4 = newlen_q4;
    }

    switch (attach) {
    case HOOK_ATTACH_NONE:
        if (hook->bumpflags & OBJ_BUMPED_X) {
            if (abs_i32(hook->v_q8.x) > 700) {
                snd_play(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
            }
            hook->v_q8.x = -hook->v_q8.x / 3;
        }
        if (hook->bumpflags & OBJ_BUMPED_Y) {
            if (abs_i32(hook->v_q8.y) > 700) {
                snd_play(SNDID_DOOR_TOGGLE, 1.f, 0.7f);
            }
            hook->v_q8.y = -hook->v_q8.y / 3;
        }
        if (obj_grounded(g, hook)) {
            hook->v_q8.x = (hook->v_q8.x * 240) >> 8;
        }
        break;
    case HOOK_ATTACH_SOLID: {
        hook->tomove.x = 0;
        hook->tomove.y = 0;
        hook->v_q8.x   = 0;
        hook->v_q8.y   = 0;
        hook->state    = HOOK_STATE_ATTACHED;
        snd_play(SNDID_HOOK_ATTACH, 0.5f, 1.f);

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
        ropenode_move(g, r, hook->ropenode, dt);
        tohook->rope     = r;
        tohook->ropenode = hook->ropenode;
        g->hero_mem.hook = obj_handle_from_obj(tohook);
        obj_delete(g, hook);
        obj_on_hooked(g, tohook);
        return 1;
    }
    }
    return 0;
}

void hook_update_hooked_solid(game_s *g, obj_s *hook)
{
    obj_s  *h = obj_get_tagged(g, OBJ_TAG_HERO);
    rope_s *r = hook->rope;

    // check if still attached
    rec_i32 hookrec = {hook->pos.x - 1, hook->pos.y - 1, hook->w + 2, hook->h + 2};

    obj_s *tohook = NULL;
    if (hook_can_attach(g, hook, hookrec, &tohook) == HOOK_ATTACH_SOLID) {
        obj_s *solid;
        if (obj_try_from_obj_handle(hook->linked_solid, &solid) &&
            !overlap_rec(hookrec, obj_aabb(solid))) {
            hook->state          = HOOK_STATE_FREE;
            hook->linked_solid.o = NULL;
        }
    } else {
        hook->state          = HOOK_STATE_FREE;
        hook->linked_solid.o = NULL;
    }
}

void hook_update(game_s *g, obj_s *hook)
{
    switch (hook->state) {
    case HOOK_STATE_FREE:
        if (hook_update_nonhooked(g, hook)) return;
        break;
    case HOOK_STATE_ATTACHED: {
        hook_update_hooked_solid(g, hook);
        break;
    }
    }

    hook->bumpflags = 0;
    obj_s  *h       = obj_get_tagged(g, OBJ_TAG_HERO);
    rec_i32 r_room  = {0, 0, g->pixel_x, g->pixel_y};
    if (!overlap_rec_pnt(r_room, obj_pos_center(hook))) {
        hero_action_ungrapple(g, h);
    }
}

void hook_destroy(game_s *g, obj_s *ohero, obj_s *ohook)
{
    if (ohook->ID == OBJ_ID_HOOK) {
        obj_delete(g, ohook);
    }

    g->rope.active  = 0;
    ohero->ropenode = NULL;
    ohero->rope     = NULL;
    ohook->rope     = NULL;
    ohook->ropenode = NULL;
}

bool32 hook_is_attached(obj_s *o)
{
    assert(o->ID == OBJ_ID_HOOK);
    return o->state == HOOK_STATE_ATTACHED;
}

i32 hook_is_stretched(game_s *g, obj_s *o)
{
    rope_s *r = o->rope;
    if (!r) return 0;
    i32 l = rope_len_q4(g, r);
    if (l <= r->len_max_q4) return 0;
    return ((l << 8) / r->len_max_q4 - 256);
}