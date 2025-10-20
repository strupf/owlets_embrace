// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "owl/owl.h"
#include "app.h"
#include "game.h"

obj_s *owl_create(g_s *g)
{
    obj_s *o              = obj_create(g);
    owl_s *h              = (owl_s *)&g->owl;
    o->ID                 = OBJID_OWL;
    o->heap               = h;
    o->w                  = OWL_W;
    o->h                  = OWL_H;
    o->on_animate         = owl_on_animate;
    o->facing             = 1;
    o->n_sprites          = 1;
    o->hitbox_flags_group = HITBOX_FLAG_GROUP_PLAYER;
    obj_tag(g, o, OBJ_TAG_OWL);
    o->render_priority = RENDER_PRIO_OWL;
    o->flags =
        OBJ_FLAG_ACTOR |
        OBJ_FLAG_CLAMP_ROOM_X |
        OBJ_FLAG_LIGHT;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS |
                    OBJ_MOVER_GLUE_GROUND |
                    OBJ_MOVER_ONE_WAY_PLAT |
                    OBJ_MOVER_AVOID_HEADBUMP |
                    OBJ_MOVER_SLIDE_Y_NEG;
    h->n_air_jumps_max = 5;
    return o;
}

void owl_on_update(g_s *g, obj_s *o, inp_s inp)
{
    owl_s  *h      = (owl_s *)o->heap;
    i32     dp_x   = inps_x(inp);
    i32     dp_y   = inps_y(inp);
    bool32  dp_any = dp_x | dp_y;
    i32     st     = owl_state_check(g, o);
    wire_s *r      = o->wire;
    if (!h->ground_crouch && o->h != OWL_H) {
        owl_try_force_normal_height(g, o);
    }
    if (h->hurt_ticks) {
        h->hurt_ticks--;
        if (h->hurt_ticks < OWL_HURT_TICKS_SPRITE) {
            o->blinking = 1;
        }
    } else {
        o->blinking = 0;
    }
    if (h->dead_anim_ticks) {
        h->dead_anim_ticks++;
    }
    if (h->carry) {
        h->carry++;
    }
    if (h->ground_pull_wire_prev && !h->ground_pull_wire) {
        h->ground_pull_wire_anim = 0;
    }
    h->ground_pull_wire_prev = h->ground_pull_wire;
    h->ground_pull_wire      = 0;

    bool32 facing_locked = !o->health ||
                           h->ground_crouch_crawl ||
                           h->ground_skid_ticks ||
                           h->climb ||
                           h->ground_push_pull ||
                           h->ground_stomp_landing_ticks ||
                           h->air_stomp ||
                           h->attack_tick ||
                           h->air_walljump_ticks ||
                           (h->carry && h->carry < OWL_CARRY_PICKUP_TICKS);

    if (st == OWL_ST_AIR && r && (u32)g->ghook.len_max_q4 <= wire_len_qx(g, r, 4)) {
        facing_locked = 1;
    }

    if (!facing_locked && dp_x == -o->facing) {
        o->facing = dp_x;
    }

    bool32 try_snap_to_ladder = o->health &&
                                (st == OWL_ST_GROUND || st == OWL_ST_AIR) &&
                                !o->wire &&
                                !obj_handle_valid(h->carried);
    if (try_snap_to_ladder && inps_btn_jp(inp, INP_DU) && owl_climb_try_snap_to_ladder(g, o)) {
        st = OWL_ST_NULL;
    }

    if (st == OWL_ST_AIR) {
        if (o->health && dp_x && sgn_i32(o->v_q12.x) == dp_x && !o->wire && owl_climb_still_on_wall(g, o, dp_x, 0, 0)) {
            owl_set_to_climb(g, o);
            o->facing = dp_x;
            h->climb  = OWL_CLIMB_WALL;
            st        = OWL_ST_CLIMB;
        }
    }

    if (st == OWL_ST_GROUND || st == OWL_ST_AIR) {
        if (h->attack_tick) {
            assert(h->stance == OWL_STANCE_ATTACK);
            h->attack_tick++;

            switch (h->attack_type) {
            case OWL_ATTACK_SIDE: {
                i32 f = ani_frame(ANIID_OWL_ATTACK, h->attack_tick);
                if (3 <= f && f <= 4) {
                    i32 whit = 40;
                    i32 hhit = 20;
                    i32 yhit = o->pos.y + o->h - hhit;
                    i32 xhit = 0 < o->facing ? o->pos.x + o->w : o->pos.x - whit;

                    hitbox_s *hb     = hitbox_gen(g, h->hitboxUID, HITBOXID_OWL_WING, owl_cb_hitbox, o);
                    hb->dx_q4        = o->facing * 16;
                    hb->damage       = 1;
                    i32 flags_target = HITBOX_FLAG_GROUP_ENEMY |
                                       HITBOX_FLAG_GROUP_ENVIRONMENT;
                    hitbox_set_flags_group(hb, flags_target);
                    hitbox_set_user(hb, o);
                    hitbox_type_rec(hb, xhit, yhit, whit, hhit);
                }
                if (f < 0) {
                    h->attack_tick       = 0;
                    h->attack_last_frame = 0;
                }
                break;
            }
            case OWL_ATTACK_UP: {
                i32 f = ani_frame(ANIID_OWL_ATTACK_UP, h->attack_tick);
                if (3 <= f && f <= 4) {

                    i32 whit = 40;
                    i32 hhit = 40;
                    i32 yhit = o->pos.y - hhit;
                    i32 xhit = o->pos.x + o->w / 2 - whit / 2;

                    hitbox_s *hb     = hitbox_gen(g, h->hitboxUID, HITBOXID_OWL_BEAK, owl_cb_hitbox, o);
                    hb->damage       = 1;
                    i32 flags_target = HITBOX_FLAG_GROUP_ENEMY |
                                       HITBOX_FLAG_GROUP_ENVIRONMENT;
                    hitbox_set_flags_group(hb, flags_target);
                    hitbox_set_user(hb, o);
                    hitbox_type_rec(hb, xhit, yhit, whit, hhit);
                }
                if (f < 0) {
                    h->attack_tick       = 0;
                    h->attack_last_frame = 0;
                }
                break;
            }
            case OWL_ATTACK_DOWN: {
                i32 f = ani_frame(ANIID_OWL_ATTACK_DOWN, h->attack_tick);
                if (3 <= f && f <= 4) {

                    i32 whit = 40;
                    i32 hhit = 40;
                    i32 yhit = o->pos.y + o->h;
                    i32 xhit = o->pos.x + o->w / 2 - whit / 2;

                    hitbox_s *hb     = hitbox_gen(g, h->hitboxUID, HITBOXID_OWL_DOWN, owl_cb_hitbox, o);
                    hb->damage       = 1;
                    i32 flags_target = HITBOX_FLAG_GROUP_ENEMY |
                                       HITBOX_FLAG_GROUP_ENVIRONMENT;
                    hitbox_set_flags_group(hb, flags_target);
                    hitbox_set_user(hb, o);
                    hitbox_type_rec(hb, xhit, yhit, whit, hhit);
                }
                if (f < 0) {
                    h->attack_tick       = 0;
                    h->attack_last_frame = 0;
                }
                break;
            }
            }
        }
    } else {
        owl_cancel_attack(g, o);
    }

    bool32 can_attack = o->health &&
                        h->stance == OWL_STANCE_ATTACK &&
                        !h->ground_push_pull &&
                        (st == OWL_ST_GROUND || st == OWL_ST_AIR) &&
                        !h->ground_crouch &&
                        owl_upgrade_has(o, OWL_UPGRADE_COMPANION) &&
                        !h->carry;
    bool32 can_hook = o->health &&
                      // h->stance == OWL_STANCE_GRAPPLE &&
                      !h->ground_push_pull &&
                      (st == OWL_ST_GROUND || st == OWL_ST_AIR) &&
                      !h->ground_crouch &&
                      owl_upgrade_has(o, OWL_UPGRADE_HOOK) &&
                      !h->carry;

    if (h->aim_crank_t_falloff) {
        h->aim_crank_t_falloff--;
    } else {
        h->aim_crank_acc = (h->aim_crank_acc * 250) >> 8;
    }
    if (h->aim_ticks && h->aim_ticks < 255) {
        h->aim_ticks++;
    } else if (can_hook && !o->wire) {
        i32 d_crank = inps_crankdt_q16(inp);
        i32 a_crank = abs_i32(d_crank);

        if (10 <= a_crank) {
            h->aim_crank_t_falloff = 10;
            h->aim_crank_acc += a_crank;

            if (32768 <= h->aim_crank_acc) {
                // owl_cancel_hook_aim(g, o); // reset all aim variables, although aiming is not "cancelled"
                // h->aim_ticks = 1;
                // owl_cancel_swap(g, o);
                pltf_log("aim\n");
            }
        }
    }

    if (inps_btn(inp, INP_B)) {
        bool32 may_swap = owl_upgrade_has(o, OWL_UPGRADE_COMPANION) &&
                          obj_get_comp(g) != 0;
        bool32 b_neutral_jp = inps_btn_jp(inp, INP_B) && !dp_any;

        i32 st_switch = st;
        if (h->carry && inps_btn_jp(inp, INP_B)) {
            owl_cancel_carry(g, o);
            st_switch = OWL_ST_NULL;
        }

        switch (st_switch) {
        case OWL_ST_NULL: break;
        case OWL_ST_CLIMB: {
            if (may_swap && b_neutral_jp && h->climb != OWL_CLIMB_WALLJUMP) {
                h->stance_swap_tick = 1;
                h->climb_anim       = 0;
            }
            break;
        }
        case OWL_ST_WATER: {
            if (may_swap && b_neutral_jp) {
                h->stance_swap_tick = 1;
            }
            break;
        }
        default: {
            switch (h->stance) {
            case OWL_STANCE_ATTACK: {
#if OWL_STOMP_ONLY_WITH_COMP_ON_B
                if (inps_btn_jp(inp, INP_B) && 0 < dp_y && st == OWL_ST_AIR && owl_attack_cancellable(o)) {
                    o->v_q12.x = 0;
                    o->v_q12.y = 0;
                    owl_cancel_attack(g, o);
                    owl_cancel_air(g, o);
                    h->air_stomp = 1;
#else
                if (0) {
#endif
                } else if (may_swap && b_neutral_jp) {
                    h->stance_swap_tick = 1;
                } else if (inps_btn_jp(inp, INP_B) && can_attack && dp_any) {
                    h->hitboxUID       = hitbox_UID_gen(g);
                    h->attack_tick     = 1;
                    h->attack_flipflop = 1 - h->attack_flipflop;

                    if (dp_y == 0) {
                        h->attack_type = OWL_ATTACK_SIDE;
                        o->v_q12.y     = 0;
                    } else if (dp_y < 0) {
                        h->attack_type = OWL_ATTACK_UP;
                        // o->v_q12.y     = Q_VOBJ(1.0);
                        //  o->v_q12.y     = 0;
                    } else if (dp_y > 0) {
                        h->attack_type = OWL_ATTACK_DOWN;
                        // o->v_q12.y     = Q_VOBJ(1.0);
                        //  o->v_q12.y     = 0;
                    }

                    h->jump_ticks = 0;
                    sfx_cuef(SFXID_WINGATTACK, 0.75f, rngr_f32(0.9f, 1.1f));
                }
                break;
            }
            case OWL_STANCE_GRAPPLE: {
                if (o->wire && (inps_btn_jp(inp, INP_B) || inps_btn_jp(inp, INP_A))) {
                    owl_ungrapple(g, o);
                } else if (inps_btn_jp(inp, INP_B) && dp_any && !h->aim_ticks && can_hook) {
                    // 0 = upwards
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

                    v2_i32 vlaunch         = grapplinghook_vlaunch_from_angle(ang, Q_12(13.0));
                    g->ghook.throw_snd_iID = sfx_cuef(SFXID_HOOK_THROW, 0.5f, rngr_f32(0.95f, 1.05f));
                    v2_i32 center          = obj_pos_center(o);

                    if (0 < o->v_q12.y) {
                        o->v_q12.y >>= 1;
                    }
                    grapplinghook_create(g, &g->ghook, o, center, vlaunch);
                } else if (inps_btn_jp(inp, INP_B) && h->aim_ticks) {
                    h->aim_ticks           = 0;
                    i32 ang                = inps_crank_q16(inp);
                    g->ghook.throw_snd_iID = sfx_cuef(SFXID_HOOK_THROW, 0.5f, rngr_f32(0.95f, 1.05f));
                    v2_i32 vlaunch         = grapplinghook_vlaunch_from_angle(ang, Q_12(19.0));
                    v2_i32 center          = obj_pos_center(o);

                    if (0 < o->v_q12.y) {
                        o->v_q12.y >>= 1;
                    }
                    grapplinghook_create(g, &g->ghook, o, center, vlaunch);
                } else if (may_swap && b_neutral_jp) {
                    h->stance_swap_tick = 1;
                }
                break;
            }
            }
            break;
        }
        }

        // swapping
        if (h->stance_swap_tick) {
            h->stance_swap_tick++;
            if (owl_swap_ticks() <= h->stance_swap_tick) {
                h->stance_swap_tick = 0;
                owl_set_stance(g, o, 1 - h->stance);
            }
        }
    } else if (!inps_btn(inp, INP_B)) {
        owl_cancel_swap(g, o);

        switch (h->stance) {
        case OWL_STANCE_ATTACK: {

            break;
        }
        case OWL_STANCE_GRAPPLE: {
            if (inps_btn_jr(inp, INP_B) && o->wire && 2 <= g->ghook.state)
                owl_ungrapple(g, o);
            break;
        }
        }
    }

    switch (st) {
    case OWL_ST_NULL: break;
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

    // TODO: put somewhere else?
    grapplinghook_s *gh = &g->ghook;
    grapplinghook_update(g, gh);
    v2_i32 gh_v                = {0};
    i32    gh_f                = 0;
    bool32 constrain_rope_owl  = 0;
    bool32 constrain_rope_hook = 0;

    if (r && grapplinghook_rope_intact(g, gh)) {
        wire_optimize(g, r);
        i32    d_dt_q4 = grapplinghook_stretched_dt_len_abs(g, gh);
        v2_i32 v_wire  = owl_rope_v_to_connected_node(g, o);
        grapplinghook_animate(g, gh);

        if (gh->state) {
            if (gh->state == GRAPPLINGHOOK_FLYING) {
                obj_s *ohook        = obj_from_handle(gh->o2);
                v2_i32 drope        = rope_v_to_neighbour(r, ohook->wirenode);
                gh_f                = grapplinghook_f_at_obj_proj_v(gh, ohook, drope, &gh_v);
                constrain_rope_hook = gh_f;

            } else if (!h->air_stomp) {
                v2_i32 drope       = rope_v_to_neighbour(r, o->wirenode);
                gh_f               = grapplinghook_f_at_obj_proj_v(gh, o, drope, &gh_v);
                constrain_rope_owl = gh_f;
            }
        }

        if (st == OWL_ST_GROUND && d_dt_q4 < 256) {
            constrain_rope_owl = 0;
            if (d_dt_q4 && v_wire.x) {
                h->ground_pull_wire = sgn_i32(v_wire.x);
                o->facing           = sgn_i32(v_wire.x);
            }
        }
#if 0
        if (d_dt_q4) {
            obj_s *ograbbed = obj_from_handle(g->ghook.o2.o->linked_solid);
            if (ograbbed) {
                i32 dt_pushpull = -sgn_i32(v_wire.x) * 2;

                ograbbed->flags &= ~OBJ_FLAG_SOLID;
                bool32 can_push = !ograbbed->on_pushpull_blocked(g, ograbbed, dt_pushpull, 0);
                ograbbed->flags |= OBJ_FLAG_SOLID;

                if (can_push) {
                    obj_move(g, ograbbed, dt_pushpull, 0);
                    if (ograbbed->on_pushpull) {
                        ograbbed->on_pushpull(g, ograbbed, dt_pushpull, 0);
                    }
                }
            }
        }
#endif
    }
    if (constrain_rope_hook) {
        obj_s *ohook   = obj_from_handle(gh->o2);
        v2_i32 v_hero  = grapplinghook_v_damping(r, ohook->wirenode, ohook->subpos_q12, ohook->v_q12);
        ohook->v_q12.x = v_hero.x + gh_v.x;
        ohook->v_q12.y = v_hero.y + gh_v.y;
    }

    if (constrain_rope_owl) {
        v2_i32 v_hero = grapplinghook_v_damping(r, o->wirenode, o->subpos_q12, o->v_q12);
        o->v_q12.x    = v_hero.x + gh_v.x;
        o->v_q12.y    = v_hero.y + gh_v.y;
        // pltf_log("%i | %i\n", o->v_q12.x, o->v_q12.y);
    }
}

obj_s *owl_if_present_and_alive(g_s *g)
{
    obj_s *o = obj_get_owl(g);
    return (o && o->health ? o : 0);
}

void owl_stomp_land(g_s *g, obj_s *o)
{
    owl_s *h     = (owl_s *)o->heap;
    h->air_stomp = 0;
    v2_i32 p     = obj_pos_bottom_center(o);
    p.y -= 24;
    animobj_create(g, p, ANIMOBJ_STOMP);

    b32 did_hit = 0;

    for (obj_each(g, i)) {

        switch (i->ID) {
        case OBJID_STOMPABLE_BLOCK: {
            if (obj_standing_on(o, i, 0, 0)) {
                stompable_block_break(g, i);
                did_hit = 1;
            }
            break;
        }
        }
    }

    if (did_hit) {
        // g->freeze_tick = max_i32(2, g->freeze_tick);
        o->v_q12.y = -Q_VOBJ(4.0);
        cam_screenshake_xy(&g->cam, 14, 0, 5);
    }
}

i32 owl_state_check(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;

    if (h->carry && !obj_handle_valid(h->carried)) {
        owl_cancel_carry(g, o);
    }

    // WATER?
    if (owl_in_water(g, o)) {
        // owl_stamina_modify(o, h->stamina_max);
        if (!h->swim) {
            h->swim = OWL_SWIM_SURFACE;
            owl_cancel_climb(g, o);
            owl_cancel_air(g, o);
            owl_cancel_ground(g, o);
            owl_cancel_hook_aim(g, o);
            owl_cancel_carry(g, o);
            owl_cancel_attack(g, o);
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
                owl_stomp_land(g, o);
                h->ground_stomp_landing_ticks = OWL_STOMP_LANDING_TICKS;
                cam_screenshake_xy(&g->cam, 14, 0, 5);
            } else {
                h->ground_impact_ticks = 5;
            }
            h->ground = 1;
            if (OWL_VX_SPRINT <= abs_i32(o->v_q12.x)) {
                h->sprint = 1;
            }
        }
        return OWL_ST_GROUND;
    }
    if (h->ground) {
        h->ground = 0;
        owl_cancel_ground(g, o);
    }

    // STILL CLIMBING?
    if ((h->climb == OWL_CLIMB_LADDER && owl_climb_still_on_ladder(g, o, 0, 0)) ||
        (h->climb >= OWL_CLIMB_WALL && owl_climb_still_on_wall(g, o, o->facing, 0, 0))) {
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
    owl_s *h                = (owl_s *)o->heap;
    h->climb                = 0;
    h->climb_ladderx        = 0;
    h->climb_anim           = 0;
    h->climb_tick           = 0;
    h->climb_from_x         = 0;
    h->climb_wall_move_tick = 0;
    h->climb_slide_down     = 0;
    h->climb_move_acc       = 0;
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
    owl_s *h              = (owl_s *)o->heap;
    h->jump_ticks         = 0;
    h->jump_index         = 0;
    h->jump_snd_iID       = 0;
    h->jump_ground_ticks  = 0;
    h->air_gliding        = 0;
    h->air_stomp          = 0;
    h->air_walljump_ticks = 0;
    h->jump_anim_ticks    = 0;
}

void owl_cancel_knockback(g_s *g, obj_s *o)
{
    owl_s *h     = (owl_s *)o->heap;
    h->knockback = 0;
}

void owl_cancel_carry(g_s *g, obj_s *o)
{
    owl_s *h       = (owl_s *)o->heap;
    obj_s *o_carry = obj_from_handle(h->carried);
    if (o_carry && o_carry->on_carried_removed) {
        o_carry->on_carried_removed(g, o_carry);
    }
    h->carried    = handle_from_obj(0);
    h->carry_anim = 0;
    h->carry      = 0;
}

void owl_cancel_swap(g_s *g, obj_s *o)
{
    owl_s *h      = (owl_s *)o->heap;
    obj_s *o_comp = obj_get_comp(g);
    if (o_comp) {
        v2_i32 p      = companion_pos_swap(o_comp, o);
        o_comp->pos.x = p.x - o_comp->w / 2;
        o_comp->pos.y = p.y - o_comp->h / 2;
    }
    h->stance_swap_tick = 0;
}

void owl_cancel_push_pull(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    if (h->ground_push_pull) {
        h->ground_anim = 0;
    }
    h->ground_push_pull = 0;
    h->ground_push_tick = 0;
}

void owl_cancel_ground(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    owl_cancel_push_pull(g, o);
    h->ground                        = 0;
    h->ground_impact_ticks           = 0;
    h->ground_skid_ticks             = 0;
    h->ground_anim                   = 0;
    h->ground_sprint_doubletap_ticks = 0;
    h->ground_stomp_landing_ticks    = 0;
    h->interactable                  = handle_from_obj(0);
    h->dead_ground_ticks             = 0;
    if (o->h != OWL_H) {
        owl_try_force_normal_height(g, o);
    }
}

void owl_cancel_attack(g_s *g, obj_s *o)
{
    owl_s *h       = (owl_s *)o->heap;
    h->attack_tick = 0;
    h->attack_type = 0;
}

bool32 owl_attack_cancellable(obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    if (!h->attack_tick) return 1;

    switch (h->attack_type) {
    case OWL_ATTACK_SIDE:
        return (6 <= ani_frame(ANIID_OWL_ATTACK, h->attack_tick));
    case OWL_ATTACK_UP:
        return (6 <= ani_frame(ANIID_OWL_ATTACK_UP, h->attack_tick));
    }
    return 1;
}

void owl_cancel_hook_aim(g_s *g, obj_s *o)
{
    owl_s *h               = (owl_s *)o->heap;
    h->aim_ticks           = 0;
    h->aim_crank_t_falloff = 0;
    h->aim_crank_acc       = 0;
}

void owl_on_controlled_by_other(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    owl_cancel_air(g, o);
    owl_cancel_hook_aim(g, o);
    owl_cancel_attack(g, o);
    owl_cancel_ground(g, o);
    owl_cancel_swim(g, o);
    owl_cancel_climb(g, o);
    h->wallj_ticks = 0;
    h->sprint      = 0;
}

void owl_ungrapple(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    if (!o->wire) return;

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
        h->ground_crouch               = 0;
        h->ground_crouch_crawl         = 0;
        h->ground_crouch_standup_ticks = 0;
        h->ground_anim                 = 0;
        o->h                           = OWL_H;
        o->pos.y                       = r.y;
        return 1;
    }
    return 0;
}

i32 owl_swap_ticks()
{
#if 0
    switch (SETTINGS.swap_ticks) {
    case SETTINGS_SWAP_TICKS_SHORT: return 15;
    case SETTINGS_SWAP_TICKS_NORMAL: return 25;
    case SETTINGS_SWAP_TICKS_LONG: return 35;
    }
#else
    return 20;
#endif
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
        if (obj_from_handle(h->jumpstomped[n].h) == o) {
            return 1;
        }
    }
    if (h->n_jumpstomped == ARRLEN(h->jumpstomped)) return 0;

    obj_jumpstomped_s js = {handle_from_obj(o), stomped};

    h->jumpstomped[h->n_jumpstomped++] = js;
    return 2;
}

bool32 owl_jump_wall(g_s *g, obj_s *o, i32 dirx_jump)
{
    if (!dirx_jump) return 0;

    i32    sx_wall   = -sgn_i32(dirx_jump); // direction of the wall to jump off
    bool32 can_wallj = 0;

    for (i32 x = 0; x < OWL_WALLJUMP_DIST_PX; x++) {
        i32 dx = sx_wall * x;
        if (owl_climb_still_on_wall(g, o, -dirx_jump, dx, 0)) {
            can_wallj = 1;
            if (x) {
                obj_move(g, o, dx, 0);
            }
            break;
        }
    }

    if (!can_wallj) {
        return 0;
    }

    owl_s *h = (owl_s *)o->heap;

    owl_set_to_climb(g, o);
    o->facing       = sx_wall;
    o->bumpflags    = 0;
    o->v_q12.x      = 0;
    o->v_q12.y      = 0;
    h->climb        = OWL_CLIMB_WALLJUMP;
    h->wallj_from_x = o->pos.x;
    h->wallj_from_y = o->pos.y;
    h->wallj_ticks  = 1;
    return 1;
}

void owl_walljump_execute(g_s *g, obj_s *o)
{
    owl_s *h           = (owl_s *)o->heap;
    // h->air_walljump_ticks = OWL_WALLJUMP_TICKS_BLOCK;
    h->jump_index      = OWL_JUMP_WALL;
    owl_jumpvar_s jv   = g_owl_jumpvar[OWL_JUMP_WALL];
    i32           vy   = jv.vy;
    h->jump_ticks_max  = jv.ticks;
    h->jump_ticks      = h->jump_ticks_max;
    h->jump_vy0        = jv.v0;
    h->jump_vy1        = jv.v1;
    h->jump_anim_ticks = 1;

    o->v_q12.x = 0;
    o->v_q12.y = 0;
    if (o->v_q12.y <= -vy) {
        o->v_q12.y -= Q_VOBJ(0.5);
    } else {
        o->v_q12.y = -vy;
    }
    o->v_q12.x   = o->facing * OWL_WALLJUMP_VX;
    o->bumpflags = 0;

    sfx_cuef(SFXID_JUMP, 0.5f, rngr_f32(0.9f, 1.1f));
    v2_i32 prtp = obj_pos_bottom_center(o);
    prtp.y -= 4;
    particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_JUMP, prtp);

    for (obj_each(g, i)) {
        if (!(i->flags & (OBJ_FLAG_SOLID | OBJ_FLAG_PLATFORM_ANY))) continue;
        if (!i->on_impulse) continue;

        rec_i32 rtop = {i->pos.x, i->pos.y, i->w, 1};
        if (overlap_rec(obj_rec_bottom(o), rtop)) {
            i->on_impulse(g, i, -Q_VOBJ(2.0), Q_VOBJ(1.0));
        }
    }
}

bool32 owl_upgrade_add(obj_s *o, u32 ID)
{
    owl_s *h = (owl_s *)o->heap;
    if (h->upgrades & ID) return 0;
    h->upgrades |= ID;
    return 1;
}

bool32 owl_upgrade_rem(obj_s *o, u32 ID)
{
    owl_s *h = (owl_s *)o->heap;
    if (!(h->upgrades & ID)) return 0;
    h->upgrades &= ~ID;

    switch (ID) {
    case OWL_UPGRADE_COMPANION:
        if (owl_upgrade_has(o, OWL_UPGRADE_HOOK)) {
            h->stance = OWL_STANCE_GRAPPLE;
        } else {
            h->stance = OWL_STANCE_ATTACK;
        }
        break;
    case OWL_UPGRADE_HOOK:
        h->stance = OWL_STANCE_ATTACK;
        break;
    }
    return 1;
}

bool32 owl_upgrade_has(obj_s *o, u32 ID)
{
    owl_s *h = (owl_s *)o->heap;
    return (h->upgrades & ID);
}

i32 owl_hook_aim_angle_crank(i32 crank_q16)
{
    return 32768 - crank_q16;
}

v2_i32 owl_hook_aim_vec_from_angle(i32 a_q16, i32 l)
{
    i32    a    = a_q16 << 1;
    v2_i32 vaim = {shr_balanced_i32(l * sin_q15(a), 15),
                   shr_balanced_i32(l * cos_q15(a), 15)};
    return vaim;
}

v2_i32 owl_hook_aim_vec_from_crank_angle(i32 crank_q16, i32 l)
{
    return owl_hook_aim_vec_from_angle(32768 - crank_q16, l);
}

v2_i32 owl_rope_v_to_connected_node(g_s *g, obj_s *o)
{
    v2_i32 v = {0};
    if (!o->wire) return v;

    wirenode_s *w1 = o->wirenode;
    wirenode_s *w2 = wirenode_neighbour_of_end_node(o->wire, o->wirenode);
    return v2_i32_sub(w2->p, w1->p);
}

void owl_kill(g_s *g, obj_s *o)
{
    // if (!o->health) return;

    owl_s *h  = (owl_s *)o->heap;
    o->health = 0;
    owl_cancel_air(g, o);
    owl_cancel_swim(g, o);
    owl_cancel_attack(g, o);
    owl_cancel_push_pull(g, o);
    owl_cancel_climb(g, o);
    owl_cancel_ground(g, o);
    h->aim_ticks           = 0;
    h->aim_crank_t_falloff = 0;
    h->aim_crank_acc       = 0;
    h->air_stomp           = 0;
    h->hurt_ticks          = 0;
    h->wallj_from_x        = 0;
    h->wallj_from_y        = 0;
    h->wallj_ticks         = 0;
    o->v_q12.y             = -Q_VOBJ(5.0);
    // o->v_q12.x             = 0;
    h->dead_anim_ticks     = 1;
    o->subpos_q12.y        = 0;
    h->dead_bounce_counter = 0;
    o->bumpflags           = 0;
    owl_set_stance(g, o, OWL_STANCE_GRAPPLE);
    obj_s *ocomp = obj_get_comp(g);
    if (ocomp) {
        companion_on_owl_died(g, ocomp);
    }
    cs_gameover_enter(g);
}

void owl_set_stance(g_s *g, obj_s *o, i32 stance)
{
    owl_s *h = (owl_s *)o->heap;
    if (h->stance == stance) return;

    obj_s *ocomp = obj_get_comp(g);
    if (ocomp && h->stance == OWL_STANCE_ATTACK) {
        ocomp->pos.x = o->pos.x + o->w / 2 - ocomp->w / 2 - o->facing * 16;
        ocomp->pos.y = o->pos.y + o->h / 2 - ocomp->h / 2 - 8;
        companion_on_enter_mode(g, ocomp, stance);
    }

    h->stance      = stance;
    h->attack_tick = 0;
}

void owl_special_state(g_s *g, obj_s *o, i32 special_state)
{
    owl_s *h = (owl_s *)o->heap;
    owl_cancel_air(g, o);
    owl_cancel_hook_aim(g, o);
    owl_cancel_attack(g, o);
    owl_cancel_ground(g, o);
    owl_cancel_swim(g, o);
    owl_cancel_climb(g, o);
    owl_cancel_carry(g, o);
    owl_ungrapple(g, o);
    h->wallj_ticks         = 0;
    h->sprint              = 0;
    //
    h->special_state       = special_state;
    h->special_state_timer = 0;
}

void owl_special_state_unset(obj_s *o)
{
    owl_s *h         = (owl_s *)o->heap;
    h->special_state = 0;
}

const owl_jumpvar_s g_owl_jumpvar[NUM_OWL_JUMP] = {
    {Q_VOBJ(3.13), 30, Q_VOBJ(0.39), Q_VOBJ(0.12)}, // out of water
    {Q_VOBJ(4.45), 35, Q_VOBJ(0.25), Q_VOBJ(0.12)}, // ground
    {Q_VOBJ(5.86), 40, Q_VOBJ(0.20), Q_VOBJ(0.08)}, // ground boosted
    {Q_VOBJ(2.34), 50, Q_VOBJ(0.55), Q_VOBJ(0.08)}, // air 0
    {Q_VOBJ(1.34), 40, Q_VOBJ(0.55), Q_VOBJ(0.08)}, // air 1
    {Q_VOBJ(0.00), 10, Q_VOBJ(0.00), Q_VOBJ(0.00)}, // air 2
    {Q_VOBJ(3.80), 30, Q_VOBJ(0.25), Q_VOBJ(0.05)}, // wall
    {Q_VOBJ(3.00), 20, Q_VOBJ(0.25), Q_VOBJ(0.05)}  // carry
};

void owl_cb_hitbox(g_s *g, hitbox_s *hb, void *arg)
{
    obj_s *o = (obj_s *)arg;
    owl_s *h = (owl_s *)o->heap;
    if (!h->attack_tick) return;

    switch (hb->ID) {
    case HITBOXID_OWL_WING: {
        o->v_q12.x = -sgn_i32(hb->dx_q4) * Q_VOBJ(2.0);
        break;
    }
    case HITBOXID_OWL_BEAK: {
        o->v_q12.y    = max_i32(o->v_q12.y, 0);
        h->jump_ticks = 0;
        break;
    }
    case HITBOXID_OWL_DOWN: {
        o->v_q12.y = min_i32(o->v_q12.y, -Q_VOBJ(5.0));
        break;
    }
    }
}