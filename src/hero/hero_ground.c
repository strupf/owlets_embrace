// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_update_ground(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    hero_restore_grounded_stuff(g, o);

    if (h->trys_lifting) return;
    o->drag_q8.x = 250;
    h->edgeticks = 6;

    if (h->sprint_dtap) {
        h->sprint_dtap += sgn_i32(h->sprint_dtap);
        if (HERO_SPRINT_DTAP_TICKS <= abs_i32(h->sprint_dtap)) {
            h->sprint_dtap = 0;
        }
    }

    i32 dpad_x = inp_x();
    i32 dpad_y = inp_y();

    v2_i32 posc         = obj_pos_center(o);
    obj_s *interactable = obj_closest_interactable(g, posc);
    h->interactable     = obj_handle_from_obj(interactable);

    if (0 < dpad_y && !o->rope && !h->crawlingp) { // sliding
        h->sprint_dtap = 0;
        i32 accx       = 0;
        if (0) {
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+1, 0})) {
            accx = +85;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-1, 0})) {
            accx = -85;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){+2, 0})) {
            accx = +40;
        } else if (!obj_grounded_at_offs(g, o, (v2_i32){-2, 0})) {
            accx = -40;
        }

        if (accx) {
            h->sliding = 1;
        }

        if (h->sliding) {
            o->drag_q8.x = accx ? HERO_DRAG_SLIDING : 245;
            o->vel_q8.x += accx;

            if (rngr_i32(0, 2)) {
                particle_desc_s prt = {0};
                v2_i32          bc  = obj_pos_bottom_center(o);
                bc.y -= 2;
                prt.p.p_q8      = v2_shl(bc, 8);
                prt.p.v_q8.x    = 0;
                prt.p.v_q8.y    = -rngr_i32(100, 200);
                prt.p.a_q8.y    = 20;
                prt.p.size      = 2;
                prt.p.ticks_max = 70;
                prt.ticksr      = 20;
                prt.pr_q8.x     = 2000;
                prt.pr_q8.y     = 2000;
                prt.vr_q8.x     = 100;
                prt.vr_q8.y     = 100;
                prt.ar_q8.y     = 4;
                prt.sizer       = 1;
                prt.p.gfx       = PARTICLE_GFX_CIR;
                particles_spawn(g, &g->particles, prt, 1);
            }
        }
    }

    if ((inp_action_jp(INP_A) || 0 < h->jump_btn_buffer)) {
        hero_start_jump(g, o, HERO_JUMP_GROUND);
        if (h->sliding) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SLIDE_Q8) >> 8;
        }
        if (hero_is_sprinting(h)) { // boosting
            o->vel_q8.x = (o->vel_q8.x * HERO_VX_BOOST_SPRINT_Q8) >> 8;
            o->vel_q8.y -= HERO_VY_BOOST_SPRINT_ABS;
        }
        h->ground_impact_ticks = 6;
    }

    bool32 can_crawl = !h->sliding &&
                       !h->carrying &&
                       !h->trys_lifting &&
                       !h->grabbingp;
    if (can_crawl) {
        if (0 < dpad_y) {
            h->crawling = 1;
            if (!h->crawlingp) {
                hero_unhook(g, o);
                o->pos.y += HERO_HEIGHT - HERO_HEIGHT_CRAWL;
                o->h = HERO_HEIGHT_CRAWL;
            }
        } else if (h->crawlingp) {
            if (!hero_stand_up(g, o)) {
                h->crawling = 1;
            }
        }
    }

    if (!h->sliding) {
        if (dpad_x != sgn_i(o->vel_q8.x)) {
            o->vel_q8.x /= 2;
        }

        if (0 < dpad_y && h->jump_boost_tick) {
            h->jump_boost_tick++;
            if (100 <= h->jump_boost_tick) {
                h->jump_boost_tick = 0;
            }
        }

        if (0 < dpad_y) {
            if (inp_action_jp(INP_A)) {
                o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
                o->tomove.y += 1;
            } else {
            }
        }

        if (o->vel_prev_q8.x == 0 && o->vel_q8.x != 0) {
            o->animation = 0;
        }

        i32 dtap_s = 0;
        if (inp_action_jp(INP_DL)) {
            dtap_s = -1;
        }
        if (inp_action_jp(INP_DR)) {
            dtap_s = +1;
        }
        if (!hero_is_sprinting(h) && hero_has_upgrade(g, HERO_UPGRADE_SPRINT) &&
            dtap_s != 0) {

            if (h->sprint_dtap == 0) {
                h->sprint_dtap = dtap_s;
            } else if (sgn_i32(h->sprint_dtap) == dtap_s) {
                h->sprint_ticks = 1;
            }
        }

        if (dpad_x) {
            i32 vt = HERO_VX_WALK;
            if (h->sprint_ticks) {
                vt = lerp_i32(vt, HERO_VX_SPRINT, min_i32(h->sprint_ticks++, 10), 10);
            }
            if (h->crawling) {
                vt = (HERO_VX_WALK * 2) / 4;
            }
            i32 i0 = (dpad_x == sgn_i32(o->vel_q8.x) ? abs_i32(o->vel_q8.x) : 0);
            i32 ax = (max_i32(vt - i0, 0) * 256) >> 8;
            o->vel_q8.x += ax * dpad_x;
        } else {
            h->sprint_ticks = 0;
        }
    }
}

void hero_restore_grounded_stuff(game_s *g, obj_s *o)
{
    hero_s *h    = &g->hero_mem;
    h->swimticks = HERO_SWIM_TICKS;
    h->flytime   = g->save.flytime;
}