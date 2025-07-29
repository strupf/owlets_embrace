// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl.h"

void owl_ground(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h    = (owl_s *)o->heap;
    i32    dp_x = inps_x(inp);
    i32    dp_y = inps_y(inp);
    owl_on_touch_ground(g, o);

    o->v_q12.y += OWL_GRAVITY;
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y      = 0;
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

    bool32 is_idle_standing = !o->v_q12.x &&
                              !h->crouch &&
                              !h->ground_stomp_landing_ticks;
    if (is_idle_standing) {
        h->ground_walking = 0;
    } else {
        h->ground_idle_ticks = 0;
    }
    if (h->crouch_standup_ticks) {
        h->crouch_standup_ticks--;
        obj_vx_q8_mul(o, 220);
    }
    if (h->ground_sprint_doubletap_ticks) {
        h->ground_sprint_doubletap_ticks--;
    }

    if (h->ground_stomp_landing_ticks) {
        h->ground_stomp_landing_ticks--;
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
    } else if (inps_btn_jp(inp, INP_A)) {
        owl_jump_ground(g, o);
    } else if (h->crouch) {
        h->ground_sprint_doubletap_ticks = 0;
        h->ground_idle_ticks             = 0;
        h->ground_impact_ticks           = 0;
        owl_ground_crawl(g, o, inp);
    } else if (0 < dp_y && !h->stance_swap_tick) {
        o->pos.y += OWL_H - OWL_H_CROUCH;
        o->h                             = OWL_H_CROUCH;
        h->crouch                        = 1;
        h->crouch_standup_ticks          = 0;
        h->crouch_crawl                  = 0;
        h->crouch_crawl_anim             = 0;
        h->ground_sprint_doubletap_ticks = 0;
        h->sprint                        = 0;
    } else if (inps_btn_jp(inp, INP_DU) && obj_handle_valid(h->interactable)) {
        obj_s *oi       = obj_from_obj_handle(h->interactable);
        o->v_q12.x      = 0;
        o->subpos_q12.x = 0;
        h->sprint       = 0;

        assert(oi->on_interact);
        oi->on_interact(g, oi);
    } else {
        i32 push_dir = 0;
        if (dp_x) {
            rec_i32 pushr = {o->pos.x + (0 < dp_x ? o->w : -1), o->pos.y, 1, o->h};
            if (map_blocked(g, pushr)) {
                h->ground_push_tick++;
                push_dir          = dp_x;
                h->ground_pushing = dp_x;
                o->facing         = dp_x;
                o->v_q12.x        = 0;
                o->subpos_q12.x   = 0;

                i32    n_pushables  = 0;
                obj_s *pushables[8] = {0};

                if (4 <= h->ground_push_tick) {
                    h->ground_push_tick = 0;
                    for (i32 n = 0; n < 2; n++) {
                        for (obj_each(g, i)) {
#if 0
                            if (i->ID == OBJID_PUSHBLOCK) {
                                u32 fp = i->flags;
                                i->flags &= ~OBJ_FLAG_SOLID;
                                bool32 could_push = !map_blocked(g, pushr);
                                i->flags          = fp;
                                if (could_push) {
                                    obj_move(g, i, dp_x, 0);
                                    obj_move(g, o, dp_x, 0);
                                } else {
                                    break;
                                }
                            }
#endif
                        }
                    }
                }
            }
        }

        bool32 can_sprint          = !o->rope && !h->attack_tick;
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
            ax = Q_VOBJ(0.78);
        } else if (dp_x == +vs) { // press same dir as velocity
            if (0) {
            } else if (va < OWL_VX_WALK) {
                ax = lerp_i32(Q_VOBJ(0.78), Q_VOBJ(0.08), va, OWL_VX_WALK);
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

        if (OWL_VX_MAX_GROUND < abs_i32(o->v_q12.x)) {
            obj_vx_q8_mul(o, 252);
            if (abs_i32(o->v_q12.x) < OWL_VX_MAX_GROUND) {
                o->v_q12.x = sgn_i32(o->v_q12.x) * OWL_VX_MAX_GROUND;
            }
        }
    }

    if (!h->crouch && o->v_q12.x) {
        h->ground_walking += o->v_q12.x;
    }
    obj_move_by_v_q12(g, o);
}

void owl_ground_crawl(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h    = (owl_s *)o->heap;
    i32    dp_x = inps_x(inp);
    i32    dp_y = inps_y(inp);

    if (dp_y <= 0 && owl_try_force_normal_height(g, o)) {
        h->crouch_standup_ticks = OWL_CROUCH_STANDUP_TICKS;
    } else if (0 < dp_y && inps_btn_jp(inp, INP_A)) {
        o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
        obj_move(g, o, 0, 1);
        o->moverflags |= OBJ_MOVER_ONE_WAY_PLAT;
        h->jump_edgeticks = 0;
    } else if (0 < h->crouch && h->crouch < OWL_CROUCH_MAX_TICKS) {
        h->crouch++;
        o->v_q12.x = 0;
    } else {
        o->v_q12.x = dp_x * OWL_VX_CRAWL;
        if (h->crouch_crawl) {
            h->crouch_crawl_anim += abs_i32(o->v_q12.x);
        } else if (dp_x) {
            h->crouch_crawl      = 1;
            h->crouch_crawl_anim = 0;
            o->facing            = dp_x;
        } else {
            h->crouch_crawl_anim++;
        }
    }
}

void owl_jump_ground(g_s *g, obj_s *o)
{
    owl_s        *h  = (owl_s *)o->heap;
    owl_jumpvar_s jv = g_owl_jumpvar[0];
    h->jump_index    = 0;
    h->jump_ticks    = (u8)jv.ticks;

    if (o->v_q12.y <= -jv.vy) {
        o->v_q12.y -= Q_VOBJ(0.5);
    } else {
        o->v_q12.y = -jv.vy;
    }

    h->jump_from_y          = o->pos.y;
    h->jump_ground_ticks    = 2; // glue owl to ground for 2 more frames visually
    h->crouch               = 0;
    h->crouch_crawl         = 0;
    h->crouch_standup_ticks = 0;

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
}