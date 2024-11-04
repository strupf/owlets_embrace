// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_update_ground(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    hero_restore_grounded_stuff(g, o);
    o->v_q8.y += HERO_GRAVITY;

    h->edgeticks = 6;
    i32 dpad_x   = inp_x();
    i32 dpad_y   = inp_y();

    v2_i32 posc         = obj_pos_center(o);
    obj_s *interactable = hero_interactable_available(g, o);
    h->interactable     = obj_handle_from_obj(interactable);

    if (h->holds_weapon) {
        if (dpad_y == 1 && inp_action(INP_B)) {
            h->holds_weapon++;
            h->dropped_weapon = 1;
            h->b_hold_tick    = 0;
            h->attack_tick    = 0;
            if (HERO_WEAPON_DROP_TICKS <= h->holds_weapon) {
                h->holds_weapon = 0;
                weapon_pickup_place(g, o);
            }
        } else {
            h->holds_weapon = 1;
        }
    }

    if (h->dropped_weapon && dpad_y != 1) {
        h->dropped_weapon = 0;
    }

    if (o->v_prev_q8.x == 0 && o->v_q8.x != 0) {
        o->animation = 0;
    }

    // crawling ----------------------------------------------------------------

    if (h->crouched && dpad_y <= 0) {
        hero_try_stand_up(g, o);
    } else if (!h->crouched && 0 < dpad_y && !o->rope) {
        o->pos.y += HERO_HEIGHT - HERO_HEIGHT_CROUCHED;
        o->h              = HERO_HEIGHT_CROUCHED;
        h->crouched       = 1;
        h->crouch_standup = 0;
        h->sprint         = 0;
    }

    if (h->crouched) {
        if (0 < dpad_y && h->action_jump && !h->action_jumpp) {
            o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
            h->jump_btn_buffer = 0;
            obj_move_by(g, o, 0, 1);
        }

        if (h->crouched < HERO_CROUCHED_MAX_TICKS) {
            h->crouched++;
            o->v_q8.x = 0;
        } else {
            if (!h->crawl && dpad_x) {
                h->crawl = dpad_x;
            }
            o->v_q8.x = dpad_x * HERO_VX_CRAWL;
        }
    } else {
        if (h->crouch_standup) {
            h->crouch_standup--;
            obj_vx_q8_mul(o, 220);
        }

        if (0 < h->jump_btn_buffer) {
            h->ground_impact_ticks = 6;
            hero_start_jump(g, o, HERO_JUMP_GROUND);
        }

        if (!h->sprint) {
            if (h->sprint_dtap) {
                h->sprint_dtap--;
            }

            if (inp_action_jp(INP_DR) || inp_action_jp(INP_DL)) {
                if (h->sprint_dtap) {
                    h->sprint      = 1;
                    h->sprint_dtap = 0;
                    if (dpad_x == -1) {
                        o->v_q8.x = min_i32(o->v_q8.x, -HERO_VX_WALK);
                    } else if (dpad_x == +1) {
                        o->v_q8.x = max_i32(o->v_q8.x, +HERO_VX_WALK);
                    }
                } else {
                    h->sprint_dtap = 12;
                }
            }
        }
        i32 vs = sgn_i32(o->v_q8.x);
        i32 va = abs_i32(o->v_q8.x);
        i32 ax = 0;

        if (h->skidding) {
            obj_vx_q8_mul(o, 256 <= va ? 220 : 128);
        } else if (dpad_x != vs) {
            if (HERO_VX_SPRINT <= va) {
                o->facing   = vs;
                h->skidding = 15;
            } else {
                obj_vx_q8_mul(o, 128);
            }
        }

        if (vs == 0 && h->skidding < 6) {
            ax = 200;
        } else if (dpad_x == +vs) { // press same dir as velocity
            if (0) {
            } else if (va < HERO_VX_WALK) {
                ax = lerp_i32(200, 20, va, HERO_VX_WALK);
            } else if (va < HERO_VX_SPRINT && h->sprint) {
                ax = lerp_i32(20, 4, va, HERO_VX_SPRINT);
            }
            ax = min_i32(ax, HERO_VX_SPRINT - va);

        } else if (dpad_x == -vs && h->skidding < 6) {
            ax = min_i32(70, va);
        }

        o->v_q8.x += ax * dpad_x;
        if (o->rope) {
            ropenode_s *rn = ropenode_neighbour(o->rope, o->ropenode);
            if (260 <= rope_stretch_q8(g, o->rope) &&
                sgn_i32(rn->p.x - o->pos.x) == -sgn_i32(o->v_q8.x)) {
                o->v_q8.x = 0;
            }
        }
    }
}

void hero_restore_grounded_stuff(game_s *g, obj_s *o)
{
    hero_s *h    = &g->hero_mem;
    h->swimticks = HERO_SWIM_TICKS;
    if (h->stomp) {
        h->stomp = 0;
        snd_play(SNDID_STOMP, 1.f, 1.f);

        particle_desc_s prt = {0};
        {
            prt.p.p_q8      = v2_shl(obj_pos_center(o), 8);
            prt.p.v_q8.x    = 0;
            prt.p.v_q8.y    = -300;
            prt.p.a_q8.y    = 20;
            prt.p.size      = 3;
            prt.p.ticks_max = 20;
            prt.ticksr      = 10;
            prt.pr_q8.x     = 5000;
            prt.pr_q8.y     = 1000;
            prt.vr_q8.x     = 400;
            prt.vr_q8.y     = 200;
            prt.ar_q8.y     = 5;
            prt.sizer       = 1;
            prt.p.gfx       = PARTICLE_GFX_CIR;
            prt.p.col       = GFX_COL_WHITE;
            particles_spawn(g, prt, 15);
            prt.p.col = GFX_COL_BLACK;
            particles_spawn(g, prt, 15);
        }
    }

    hero_flytime_add_ui(g, o, 10000);
    if (h->flytime_added == 0) {
        h->jump_ui_may_hide = 1;
    }
    staminarestorer_respawn_all(g, o);
}