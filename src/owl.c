// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "owl.h"
#include "game.h"

const owl_jumpvar_s g_owl_jumpvar[NUM_OWL_JUMP] = {
    {Q_VOBJ(3.13), 30, Q_VOBJ(0.39), Q_VOBJ(0.12)}, // out of water
    {Q_VOBJ(3.91), 30, Q_VOBJ(0.25), Q_VOBJ(0.12)}, // ground spear
    {Q_VOBJ(4.20), 30, Q_VOBJ(0.29), Q_VOBJ(0.13)}, // ground
    {Q_VOBJ(5.86), 40, Q_VOBJ(0.20), Q_VOBJ(0.08)}, // ground boosted
    {Q_VOBJ(2.34), 50, Q_VOBJ(0.55), Q_VOBJ(0.08)}, // fly
    {Q_VOBJ(2.50), 30, Q_VOBJ(0.30), Q_VOBJ(0.05)}  // wall
};

obj_s *owl_create(g_s *g)
{
    obj_s *o      = obj_create(g);
    owl_s *h      = (owl_s *)&g->owl;
    o->ID         = OBJID_OWL;
    o->heap       = h;
    o->w          = OWL_W;
    o->h          = OWL_H;
    o->on_animate = owl_on_animate;
    o->facing     = 1;
    o->n_sprites  = 1;
    obj_tag(g, o, OBJ_TAG_OWL);
    o->render_priority = RENDER_PRIO_HERO;
    o->flags =
        OBJ_FLAG_ACTOR |
        OBJ_FLAG_CLAMP_ROOM_X |
        OBJ_FLAG_LIGHT;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_AVOID_HEADBUMP |
                    OBJ_MOVER_SLIDE_Y_NEG;

    return o;
}

void owl_on_update(g_s *g, obj_s *o, inp_s inp)
{
    owl_s  *h      = (owl_s *)o->heap;
    i32     dp_x   = inps_x(inp);
    i32     dp_y   = inps_y(inp);
    bool32  dp_any = dp_x | dp_y;
    i32     st     = owl_state_check(g, o);
    rope_s *r      = o->rope;

    if (!h->ground_pushing) {
        h->ground_pushing_anim = 0;
        h->ground_push_tick    = 0;
    }
    h->ground_pushing_prev = h->ground_pushing;
    h->ground_pushing      = 0;

    if (!h->crouch && o->h != OWL_H) {
        owl_try_force_normal_height(g, o);
    }

    bool32 facing_locked = h->crouch_crawl ||
                           h->ground_skid_ticks ||
                           h->climb ||
                           h->ground_stomp_landing_ticks ||
                           h->air_stomp ||
                           h->attack_tick;
    if (st == OWL_ST_AIR && r && 240 <= rope_stretch_q8(g, r)) {
        facing_locked = 1;
    }

    if (!facing_locked && dp_x == -o->facing) {
        o->facing = dp_x;
    }

    if (st == OWL_ST_GROUND || st == OWL_ST_AIR) {
        if (inps_btn_jp(inp, INP_DU) && owl_climb_try_snap_to_ladder(g, o)) {
            st = OWL_ST_NULL;
        }
    }

    if (inps_btn(inp, INP_B)) {
        switch (h->stance) {
        case OWL_STANCE_ATTACK: {
            if (inps_btn_jp(inp, INP_B) &&
                (st == OWL_ST_GROUND || st == OWL_ST_AIR)) {
                h->hitID           = game_owl_hitID_next(g);
                h->attack_tick     = 1;
                h->attack_flipflop = 1 - h->attack_flipflop;
                snd_play(SNDID_WINGATTACK, 0.75f, rngr_f32(0.9f, 1.1f));
            }
            break;
        }
        case OWL_STANCE_GRAPPLE: {
            if (inps_btn_jp(inp, INP_B) && dp_any &&
                (st == OWL_ST_GROUND || st == OWL_ST_AIR)) { //     0 = upwards
                // 32768 = downwards
                i32 ang = 0;
                i32 dx  = dp_x;
                i32 dy  = dp_y;

                if (dx && dy) { // diagonal up/down
                    ang = -dx * (0 < dy ? 20000 : 8000);
                } else if (dx == 0 && dy) { // up/down
                    ang = 0 < dy ? 32768 : 0;
                } else if (dx && dy == 0) { // sideways
                    ang = -dx * 12000;
                }

                v2_i32 vlaunch         = grapplinghook_vlaunch_from_angle(ang, 5000);
                g->ghook.throw_snd_iID = snd_play(SNDID_HOOK_THROW, 0.5f, rngr_f32(0.95f, 1.05f));
                v2_i32 center          = obj_pos_center(o);

                if (0 < o->v_q12.y) {
                    o->v_q12.y >>= 1;
                }
                // h->hook_throw_anim_tick = HERO_THROW_HOOK_ANIM_TICKS;
                grapplinghook_create(g, &g->ghook, o, center, vlaunch);
                // hero_action_throw_grapple(g, o, ang, 5000);
            }
            break;
        }
        }

        // swapping
        if (st != OWL_ST_WATER) {
            if (h->stance_swap_tick) {
                h->stance_swap_tick++;
                if (owl_swap_ticks() <= h->stance_swap_tick) {
                    h->stance           = 1 - h->stance;
                    h->stance_swap_tick = 0;
                    h->attack_tick      = 0;
                    obj_s *ocomp        = obj_get_tagged(g, OBJ_TAG_COMPANION);
                    ocomp->pos.x        = o->pos.x + o->w / 2 - ocomp->w / 2 - o->facing * 16;
                    ocomp->pos.y        = o->pos.y + o->h / 2 - ocomp->h / 2 - 8;
                    companion_on_enter_mode(g, ocomp, h->stance);
                }
            } else if (inps_btn_jp(inp, INP_B) && !dp_any) {
                h->stance_swap_tick = 1;
            }
        } else {
            h->stance_swap_tick = 0;
        }
    } else {
        h->stance_swap_tick = 0;

        switch (h->stance) {
        case OWL_STANCE_ATTACK: {

            break;
        }
        case OWL_STANCE_GRAPPLE: {
            owl_ungrapple(g, o);
            break;
        }
        }
    }

    if (st == OWL_ST_GROUND || st == OWL_ST_AIR) {
        if (h->attack_tick) {
            assert(h->stance == OWL_STANCE_ATTACK);
            h->attack_tick++;

            if (h->attack_tick <= 12) {
                // hitbox_s hb = hero_hitbox_wingattack(o);
                //  if (hero_attackbox(g, hb)) {

                // o->v_q12.x = -o->facing * Q_VOBJ(2.0);
                //}
            }
            i32 ticks_total = ani_len(ANIID_OWL_ATTACK);
            if (ticks_total <= h->attack_tick) {
                h->attack_tick       = 0;
                h->attack_last_frame = 0;
            }
        }
    } else {
        owl_cancel_attack(g, o);
    }

    switch (st) {
    case OWL_ST_NULL: {
        break;
    }
    case OWL_ST_GROUND: {
        owl_ground(g, o, inp);
        break;
    }
    case OWL_ST_AIR: {
        owl_air(g, o, inp);
        break;
    }
    case OWL_ST_CLIMB: {
        owl_climb(g, o, inp);
        break;
    }
    case OWL_ST_WATER: {
        owl_water(g, o, inp);
        break;
    }
    }

#if 1
    // TODO: put somewhere else?
    grapplinghook_s *gh = &g->ghook;
    grapplinghook_update(g, gh);
    if (r && grapplinghook_rope_intact(g, gh)) {
        rope_update(g, r);
        grapplinghook_animate(g, gh);

        if (gh->state) {
            if (gh->state == GRAPPLINGHOOK_FLYING) {
                v2_i32 hv_q12 = v2_i32_shl(v2_i32_from_i16(gh->v_q8), 4);
                v2_i32 v_hook = rope_recalc_v(g, r, gh->rn,
                                              v2_i32_from_i16(gh->p_q8),
                                              hv_q12);
                gh->v_q8      = v2_i16_from_i32(v2_i32_shr(v_hook, 4));
            } else if (!h->air_stomp) {
                bool32 calc_v = 0;
                calc_v |= st == OWL_ST_GROUND && 271 <= rope_stretch_q8(g, r);
                calc_v |= st != OWL_ST_GROUND;
                if (calc_v) {
                    v2_i32 v_hero = rope_recalc_v(g, r, o->ropenode,
                                                  o->subpos_q12,
                                                  o->v_q12);
                    o->v_q12      = v_hero;
                }
            }
        }
    }
#endif
}

void owl_on_update_post(g_s *g, obj_s *o, inp_s inp)
{
    owl_s  *h    = (owl_s *)o->heap;
    rec_i32 owlr = obj_aabb(o);
    v2_i32  owlc = {o->pos.x + (OWL_W >> 1), o->pos.y + o->h - 12};

    bool32 stomped_on_any = 0;
    bool32 jumped_on_any  = h->n_jumpstomped;
    for (i32 n = 0; n < h->n_jumpstomped; n++) {
        obj_jumpstomped_s js = h->jumpstomped[n];
        obj_s            *i  = obj_from_obj_handle(js.h);

        stomped_on_any |= js.stomped;
        if (!i) continue;
    }
    h->n_jumpstomped      = 0;
    h->interactable       = obj_handle_from_obj(0);
    u32    d_interactable = POW2(OWL_INTERACTABLE_DST);
    i32    heal           = 0;
    i32    dmg            = 0;
    bool32 has_knockback  = 0;
    v2_i32 knockback      = {0};

    // damage from objects
    for (obj_each(g, i)) {
        if (i == o) continue;
        rec_i32 ri = obj_aabb(i);

        if (i->flags & OBJ_FLAG_HURT_ON_TOUCH) {
            if (overlap_rec(owlr, ri)) {
                dmg = max_i32(dmg, 1);
            }
        }

        if (i->flags & OBJ_FLAG_INTERACTABLE) {
            v2_i32 ic = {o->pos.x + (o->w >> 1) + o->offs_interact.x,
                         o->pos.y + (o->h >> 1) + o->offs_interact.y};
            u32    d  = v2_i32_distancesq(owlc, ic);
            if (d < d_interactable) {
                d_interactable  = d;
                h->interactable = obj_handle_from_obj(i);
            }
        }
    }

    // damage from tiles
    tile_map_bounds_s bd = tile_map_bounds_rec(g, owlr);
    for (i32 y = bd.y1; y <= bd.y2; y++) {
        for (i32 x = bd.x1; x <= bd.x2; x++) {
            tile_s t = g->tiles[x + y * g->tiles_x];
            if (t.type == TILE_TYPE_THORNS) {
                dmg = max_i32(dmg, 1);
            }
        }
    }

    if (has_knockback) {
        o->v_q12.x = knockback.x;
        o->v_q12.y = knockback.y;
    }

    i32 health_dt = heal - dmg;
    o->health     = clamp_i32((i32)o->health + health_dt, 0, o->health_max);

    if (dmg) {
        snd_play(SNDID_HURT, 1.f, 1.f);
    }

    if (o->health == 0) {
    } else {
    }
}

obj_s *owl_if_present_and_alive(g_s *g)
{
    obj_s *o = obj_get_owl(g);
    return (o && o->health ? o : 0);
}

i32 owl_state_check(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;

    // WATER?
    if (owl_in_water(g, o)) {
        owl_stamina_modify(o, h->stamina_max);
        if (!h->swim) {
            h->swim = OWL_SWIM_SURFACE;
            owl_cancel_climb(g, o);
            owl_cancel_air(g, o);
            owl_cancel_ground(g, o);
            owl_ungrapple(g, o);
        }
        return OWL_ST_WATER;
    }
    if (h->swim) {
        h->swim = 0;
        owl_cancel_swim(g, o);
    }

    // GROUNDED?
    if (obj_grounded(g, o) && 0 <= o->v_q12.y) {
        owl_on_touch_ground(g, o);

        if (!h->ground) {
            bool32 was_stomping = h->air_stomp;
            owl_cancel_climb(g, o);
            owl_cancel_air(g, o);
            if (was_stomping) {
                h->ground_stomp_landing_ticks = OWL_STOMP_LANDING_TICKS;
            } else {
                h->ground_impact_ticks = 5;
            }
            h->ground = 1;
        }
        return OWL_ST_GROUND;
    }
    if (h->ground) {
        h->ground = 0;
        owl_cancel_ground(g, o);
    }

    // STILL CLIMBING?
    if ((h->climb == OWL_CLIMB_LADDER && owl_climb_still_on_wall(g, o, 0, 0)) ||
        (h->climb == OWL_CLIMB_WALL && owl_climb_still_on_wall(g, o, 0, 0))) {
        return OWL_ST_CLIMB;
    }
    if (h->climb) {
        h->climb = 0;
        owl_cancel_climb(g, o);
    }

    return OWL_ST_AIR;
}

bool32 owl_in_water(g_s *g, obj_s *o)
{
    return (water_depth_rec(g, obj_aabb(o)) >= (OWL_WATER_THRESHOLD));
}

bool32 owl_submerged(g_s *g, obj_s *o)
{
    return (water_depth_rec(g, obj_aabb(o)) >= OWL_H);
}

void owl_cancel_climb(g_s *g, obj_s *o)
{
    owl_s *h             = (owl_s *)o->heap;
    h->climb             = 0;
    h->climb_ladderx     = 0;
    h->climb_anim        = 0;
    h->climb_camx_smooth = 0;
    h->climb_from_x      = 0;
}

void owl_cancel_swim(g_s *g, obj_s *o)
{
    owl_s *h         = (owl_s *)o->heap;
    h->swim          = 0;
    h->swim_anim     = 0;
    h->swim_sideways = 0;
}

void owl_cancel_air(g_s *g, obj_s *o)
{
    owl_s *h             = (owl_s *)o->heap;
    h->jump_ticks        = 0;
    h->jump_index        = 0;
    h->jump_snd_iID      = 0;
    h->jump_ground_ticks = 0;
    h->air_gliding       = 0;
    h->air_stomp         = 0;
}

void owl_cancel_push_pull(g_s *g, obj_s *o)
{
    owl_s *h               = (owl_s *)o->heap;
    h->ground_pushing      = 0;
    h->ground_pushing_anim = 0;
    h->ground_pushing_prev = 0;
    h->ground_push_tick    = 0;
}

void owl_cancel_ground(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    owl_cancel_push_pull(g, o);
    h->ground                        = 0;
    h->ground_impact_ticks           = 0;
    h->ground_skid_ticks             = 0;
    h->ground_idle_ticks             = 0;
    h->ground_walking                = 0;
    h->ground_idle_ticks             = 0;
    h->ground_sprint_doubletap_ticks = 0;
    h->ground_stomp_landing_ticks    = 0;
    h->interactable                  = obj_handle_from_obj(0);
    if (o->h != OWL_H) {
        owl_try_force_normal_height(g, o);
    }
}

void owl_cancel_attack(g_s *g, obj_s *o)
{
    owl_s *h       = (owl_s *)o->heap;
    h->attack_tick = 0;
}

void owl_ungrapple(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    if (!o->rope) return;

    grapplinghook_s *gh       = &g->ghook;
    bool32           attached = gh->state && gh->state != GRAPPLINGHOOK_FLYING;
    grapplinghook_destroy(g, gh);

    if (obj_grounded(g, o) || !attached) return;

    i32 mulx = 265;
    i32 muly = 270;

    if (abs_i32(o->v_q12.x) < OWL_VX_WALK) {
        mulx = 285;
    }
    if (abs_i32(o->v_q12.y) < Q_VOBJ(2.34)) {
        muly = 280;
    }

    obj_v_q8_mul(o, mulx, muly);
    if (o->v_q12.y <= 0) {
#define HERO_UNHOOK_VLERP_MOM Q_VOBJ(4.0)
#if 0
        h->unhooked_mom = lerp_i32(0, HERO_TICKS_UNHOOKED_MOM,
                                   min_i32(-o->v_q12.y, HERO_UNHOOK_VLERP_MOM), HERO_UNHOOK_VLERP_MOM);
#endif
#define HERO_UNHOOK_VLERP_BOOST Q_VOBJ(5.0)
        o->v_q12.y -= lerp_i32(Q_VOBJ(0.2), Q_VOBJ(1.2),
                               min_i32(-o->v_q12.y, HERO_UNHOOK_VLERP_BOOST), HERO_UNHOOK_VLERP_BOOST);
    }
    o->v_q12.x += Q_VOBJ(0.78) * sgn_i32(o->v_q12.x);
}

bool32 owl_try_force_normal_height(g_s *g, obj_s *o)
{
    if (o->h == OWL_H) return 1;
    assert(o->h == OWL_H_CROUCH);

    owl_s  *h = (owl_s *)o->heap;
    rec_i32 r = {o->pos.x, o->pos.y - (OWL_H - OWL_H_CROUCH), OWL_W, OWL_H};

    if (!map_blocked(g, r)) {
        h->crouch               = 0;
        h->crouch_crawl         = 0;
        h->crouch_standup_ticks = 0;
        h->crouch_crawl_anim    = 0;
        o->h                    = OWL_H;
        o->pos.y                = r.y;
        return 1;
    }
    return 0;
}

i32 owl_swap_ticks()
{
    switch (SETTINGS.swap_ticks) {
    case SETTINGS_SWAP_TICKS_SHORT: return 20;
    case SETTINGS_SWAP_TICKS_NORMAL: return 30;
    case SETTINGS_SWAP_TICKS_LONG: return 40;
    }
    return 0;
}

i32 owl_stamina_modify(obj_s *o, i32 dt)
{
    owl_s *h = (owl_s *)o->heap;

    i32 stamina_new  = clamp_i32(h->stamina + dt, 0, h->stamina_max);
    i32 d            = stamina_new - (i32)h->stamina;
    h->stamina       = stamina_new;
    h->stamina_added = clamp_i32((i32)h->stamina_added + d, 0, h->stamina);
    if (0 < d) {
        h->stamina_added_delay_ticks = 10;
    }
    return h->stamina;
}

i32 owl_jumpstomped_register(obj_s *ohero, obj_s *o, bool32 stomped)
{
    owl_s *h = (owl_s *)ohero->heap;
    for (i32 n = 0; n < h->n_jumpstomped; n++) {
        if (obj_from_obj_handle(h->jumpstomped[n].h) == o) {
            return 1;
        }
    }
    if (h->n_jumpstomped == ARRLEN(h->jumpstomped)) return 0;

    obj_jumpstomped_s js = {obj_handle_from_obj(o), stomped};

    h->jumpstomped[h->n_jumpstomped++] = js;
    return 2;
}