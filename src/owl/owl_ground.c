// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl/owl.h"

void owl_ground(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h    = (owl_s *)o->heap;
    i32    dp_x = inps_x(inp);
    i32    dp_y = inps_y(inp);
    owl_on_touch_ground(g, o);
    i32    l_dt_q4 = grapplinghook_stretched_dt_len_abs(g, &g->ghook);
    v2_i32 v_wire  = owl_rope_v_to_connected_node(g, o);

    if (h->dead_ground_ticks && h->dead_ground_ticks < 255) {
        h->dead_ground_ticks++;
    }

    o->v_q12.y += OWL_GRAVITY;
    if (o->bumpflags & OBJ_BUMP_Y) {
        if (o->health || h->dead_bounce_counter) {
            o->v_q12.y = 0;
            if (!o->health && !h->dead_ground_ticks) {
                h->dead_ground_ticks = 1;
            }
        } else {
            h->dead_bounce_counter = 1;
            o->v_q12.y             = -Q_VOBJ(2.5);
        }

        o->subpos_q12.y = 0;
    }
    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q12.x                       = 0;
        o->subpos_q12.x                  = 0;
        h->sprint                        = 0;
        h->ground_sprint_doubletap_ticks = 0;
        h->ground_skid_ticks             = 0;
    }
    o->bumpflags = 0;

    i32 grabdir = dp_x;
    if (h->ground_push_pull == OWL_GROUND_PULL && inps_btn(inp, INP_B)) {
        grabdir = o->facing;
    } else {
        h->ground_push_pull = 0;
    }

    bool32  may_push_pull = !h->ground_crouch && o->h == OWL_H && !o->wire && !h->carry;
    rec_i32 pushr         = {o->pos.x + (0 < grabdir ? o->w : -1), o->pos.y, 1, 12};
    obj_s  *pushable      = 0;
    i32     n_pushable    = 0;

    if (may_push_pull && (h->ground_push_pull == OWL_GROUND_PULL || dp_x) && map_blocked(g, pushr)) {
        if (inps_btn(inp, INP_B)) {
            h->ground_push_pull = OWL_GROUND_PULL;
        } else {
            h->ground_push_pull = OWL_GROUND_PUSH;
        }

        i32 s_gpt = sgn_i32(h->ground_push_tick);
        if (s_gpt != dp_x) {
            h->ground_anim      = 0;
            h->ground_push_tick = dp_x;
        } else {
            h->ground_push_tick = clamp_sym_i32(h->ground_push_tick + s_gpt, 96);
        }

        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
        owl_cancel_attack(g, o);
        owl_cancel_hook_aim(g, o);
        h->sprint = 0;

        for (obj_each(g, i)) {
            if ((i->flags & OBJ_FLAG_PUSHABLE_SOLID) == OBJ_FLAG_PUSHABLE_SOLID && overlap_rec(pushr, obj_aabb(i))) {
                if (i->on_pushpull) { // call with 0 movement (every frame) for more consistent behaviour logics
                    i->on_pushpull(g, i, 0, 0);
                }
                n_pushable++;
                pushable = i;
            }
        }
    } else {
        owl_cancel_push_pull(g, o);
    }

    if (n_pushable == 1 && pushable && dp_x) {
        // try to push by 2 px amounts (alignment of solids)
        i32     dt_pushpull = dp_x * 2;
        rec_i32 rowlpushed  = obj_aabb(o);
        rowlpushed.w += 2;
        if (dp_x < 0) {
            rowlpushed.x -= 2;
        }

        bool32 is_pulling = h->ground_push_pull == OWL_GROUND_PULL && dp_x == -o->facing;
        bool32 is_pushing = h->ground_push_pull == OWL_GROUND_PUSH && dp_x == +o->facing;

        pushable->flags &= ~OBJ_FLAG_SOLID;
        bool32 can_push = (OWL_PUSH_TICKS_MIN + pushable->pushable_weight) <= abs_i32(h->ground_push_tick) &&
                          !pushable->on_pushpull_blocked(g, pushable, dt_pushpull, 0) &&
                          !map_blocked(g, rowlpushed) &&
                          obj_grounded_at_offs(g, o, (v2_i32){dt_pushpull, 0});
        pushable->flags |= OBJ_FLAG_SOLID;

        if (pushable->ID == OBJID_BOMBPLANT && is_pulling) {
            bombplant_on_pickup(g, pushable);
            owl_cancel_push_pull(g, o);
            obj_s *o_bomb = bomb_create(g);
            bomb_set_carried(o_bomb);
            h->carried = handle_from_obj(o_bomb);
            h->carry   = 1;
        }

        if (can_push) {
            h->ground_push_tick = clamp_sym_i32(h->ground_push_tick, OWL_PUSH_TICKS_MIN);

            if (is_pulling) { // pull, move owlet first
                obj_move(g, o, dt_pushpull, 0);
                obj_move(g, pushable, dt_pushpull, 0);
            } else { // push, move solid first
                obj_move(g, pushable, dt_pushpull, 0);
                obj_move(g, o, dt_pushpull, 0);
            }
            if (pushable->on_pushpull) {
                pushable->on_pushpull(g, pushable, dt_pushpull, 0);
            }
        }
    }

    bool32 is_idle_standing = !o->v_q12.x &&
                              !h->ground_push_pull &&
                              !h->ground_crouch &&
                              !h->ground_stomp_landing_ticks;

    if (h->ground_crouch_standup_ticks) {
        h->ground_crouch_standup_ticks--;
        obj_vx_q8_mul(o, 220);
    }
    if (h->ground_sprint_doubletap_ticks) {
        h->ground_sprint_doubletap_ticks--;
    }

    if (h->ground_push_pull) {
    } else if (h->ground_stomp_landing_ticks) {
        h->ground_stomp_landing_ticks--;
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
    } else if (h->ground_crouch) {
        h->ground_sprint_doubletap_ticks = 0;
        h->ground_impact_ticks           = 0;
        owl_ground_crawl(g, o, inp);
    } else if (inps_btn_jp(inp, INP_A) && !h->attack_tick) {
        owl_jump_ground(g, o);
    }
#if OWL_ONEWAY_PLAT_DOWN_JUST_DOWN
    else if (inps_btn_jp(inp, INP_DD) && !map_blocked_excl_offs(g, obj_rec_bottom(o), o, 0, 1)) {
        o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
        obj_move(g, o, 0, +1);
        o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
        o->bumpflags = 0;
        owl_cancel_ground(g, o);
        h->jump_edgeticks = 0;
    }
#endif
    else if (0 < dp_y && !h->stance_swap_tick && !h->aim_ticks) {
        o->pos.y += OWL_H - OWL_H_CROUCH;
        o->h = OWL_H_CROUCH;
        owl_cancel_hook_aim(g, o);
        h->ground_crouch                 = 1;
        h->ground_crouch_standup_ticks   = 0;
        h->ground_crouch_crawl           = 0;
        h->ground_anim                   = 0;
        h->ground_sprint_doubletap_ticks = 0;
        h->sprint                        = 0;
    } else if (inps_btn_jp(inp, INP_DU) && obj_handle_valid(h->interactable)) {
        obj_s *oi       = obj_from_handle(h->interactable);
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
        h->sprint       = 0;

        assert(oi->on_interact);
        oi->on_interact(g, oi);
    } else if (h->attack_tick) {
        obj_vx_q8_mul(o, 250);
    } else if (h->carry && h->carry < OWL_CARRY_PICKUP_TICKS) {

    } else {
        bool32 can_sprint          = !o->wire && !h->attack_tick && !h->carry;
        bool32 can_start_sprinting = 0;

        if (!h->sprint &&
            (inps_btn_jp(inp, INP_DL) || inps_btn_jp(inp, INP_DR))) {
            if (h->ground_sprint_doubletap_ticks) {
                can_start_sprinting              = 1;
                h->ground_sprint_doubletap_ticks = 0;
            } else {
                h->ground_sprint_doubletap_ticks = 12;
            }
        }

        if (!can_sprint) {
            h->ground_sprint_doubletap_ticks = 0;
            h->sprint                        = 0;
        } else if (!h->sprint && can_start_sprinting) {
            h->sprint = 1;

            switch (dp_x) {
            case -1: o->v_q12.x = min_i32(o->v_q12.x, -OWL_VX_WALK); break;
            case +1: o->v_q12.x = max_i32(o->v_q12.x, +OWL_VX_WALK); break;
            case +0: break;
            }
        }

        i32 vs = sgn_i32(o->v_q12.x);
        i32 va = abs_i32(o->v_q12.x);
        i32 ax = 0;

        if (h->ground_skid_ticks) {
            h->ground_skid_ticks--;
            obj_vx_q8_mul(o, Q_VOBJ(1.0) <= va ? 224 : 128);
        } else if (dp_x != vs) {
            if (OWL_VX_SPRINT <= va) {
                o->facing            = vs;
                h->sprint            = 0;
                h->ground_skid_ticks = 15;
                snd_play(SNDID_STOPSPRINT, 0.1f, rngr_f32(0.9f, 1.1f));
            } else {
                obj_vx_q8_mul(o, 128);
            }
        }

        if (vs == 0 && h->ground_skid_ticks < 6) {
            ax = Q_VOBJ(0.80);
        } else if (dp_x == +vs) { // press same dir as velocity
            if (0) {
            } else if (va < OWL_VX_WALK) {
                ax = lerp_i32(Q_VOBJ(0.80), Q_VOBJ(0.08), va, OWL_VX_WALK);
                ax = min_i32(ax, OWL_VX_WALK - va);
            } else if (va < OWL_VX_SPRINT && h->sprint) {
                ax = min_i32(Q_VOBJ(0.08), OWL_VX_SPRINT - va);
            }
        } else if (dp_x == -vs && h->ground_skid_ticks < 6) {
            h->sprint = 0;
            ax        = min_i32(Q_VOBJ(0.27), va);
        }

        // threshold for moving -> idle
        if (!dp_x && va < Q_VOBJ(0.5)) {
            o->v_q12.x = 0;
        } else {
            o->v_q12.x += ax * dp_x;
        }

        if (o->v_q12.x == 0 && va) { // just got standing
            h->ground_anim = 0;
        }

        if (OWL_VX_MAX_GROUND < abs_i32(o->v_q12.x)) {
            obj_vx_q8_mul(o, 252);
            if (abs_i32(o->v_q12.x) < OWL_VX_MAX_GROUND) {
                o->v_q12.x = sgn_i32(o->v_q12.x) * OWL_VX_MAX_GROUND;
            }
        }
    }
    if (h->carry) {
        o->v_q12.x = clamp_sym_i32(o->v_q12.x, OWL_VX_CARRY);
    }

    if (!h->ground_crouch && o->v_q12.x) {
        h->ground_anim += shr_balanced_i32(o->v_q12.x, 4);
    }

    if (l_dt_q4) {
        if (sgn_i32(o->v_q12.x) == -sgn_i32(v_wire.x)) {
            o->v_q12.x = 0;
        }
    }

    obj_move_by_v_q12(g, o);
}

void owl_ground_crawl(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h    = (owl_s *)o->heap;
    i32    dp_x = inps_x(inp);
    i32    dp_y = inps_y(inp);

    if (dp_y <= 0 && owl_try_force_normal_height(g, o)) {
        h->ground_crouch_standup_ticks = OWL_CROUCH_STANDUP_TICKS;
    }
#if !OWL_ONEWAY_PLAT_DOWN_JUST_DOWN
    else if (0 < dp_y && inps_btn_jp(inp, INP_A)) {
        o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
        obj_move(g, o, 0, 1);
        o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
        h->jump_edgeticks = 0;
    }
#endif
    else if (0 < h->ground_crouch && h->ground_crouch < OWL_CROUCH_MAX_TICKS) {
        h->ground_crouch++;
        o->v_q12.x = 0;
    } else {
        o->v_q12.x = dp_x * OWL_VX_CRAWL;
        if (h->ground_crouch_crawl) {
            h->ground_anim += abs_i32(o->v_q12.x);
        } else if (dp_x) {
            h->ground_crouch_crawl = 1;
            h->ground_anim         = 0;
            o->facing              = dp_x;
        } else {
            h->ground_anim++;
        }
    }
}

void owl_jump_ground(g_s *g, obj_s *o)
{
    owl_s *h      = (owl_s *)o->heap;
    h->jump_index = OWL_JUMP_GROUND;
    if (obj_handle_valid(h->carried)) {
        h->jump_index = OWL_JUMP_CARRY;
    }

    owl_jumpvar_s jv = g_owl_jumpvar[h->jump_index];

    i32 vy            = jv.vy;
    h->jump_ticks_max = jv.ticks;
    h->jump_ticks     = h->jump_ticks_max;
    h->jump_vy0       = jv.v0;
    h->jump_vy1       = jv.v1;

    o->v_q12.y                     = -vy;
    h->jump_from_y                 = o->pos.y;
    h->jump_ground_ticks           = 2; // glue owl to ground for 2 more frames visually
    h->ground_crouch               = 0;
    h->ground_crouch_crawl         = 0;
    h->ground_crouch_standup_ticks = 0;
    owl_cancel_ground(g, o);
    snd_play(SNDID_JUMP, 0.5f, rngr_f32(0.9f, 1.1f));
    v2_i32 prtp = obj_pos_bottom_center(o);
    prtp.y -= 4;
    particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_JUMP, prtp);

    for (obj_each(g, i)) {
        if (!(i->flags & (OBJ_FLAG_SOLID | OBJ_FLAG_PLATFORM_ANY))) continue;
        if (!i->on_impulse) continue;

        rec_i32 rtop = {i->pos.x, i->pos.y, i->w, 1};
        if (overlap_rec(obj_rec_bottom(o), rtop)) {
            i->on_impulse(g, i, -o->v_q12.x, Q_VOBJ(2.0));
        }
    }
}

void owl_on_touch_ground(g_s *g, obj_s *o)
{
    owl_s *h = (owl_s *)o->heap;
    owl_stamina_modify(o, h->stamina_max);
    h->jump_edgeticks = OWL_JUMP_EDGE_TICKS;
    h->n_air_jumps    = 0;
}