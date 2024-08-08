// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_update_ground(game_s *g, obj_s *o)
{
    hero_s *h = &g->hero_mem;
    hero_restore_grounded_stuff(g, o);

    if (h->trys_lifting) return;
    h->edgeticks = 6;

    i32 dpad_x = inp_x();
    i32 dpad_y = inp_y();

    v2_i32 posc         = obj_pos_center(o);
    obj_s *interactable = obj_closest_interactable(g, posc);
    h->interactable     = obj_handle_from_obj(interactable);

    if (0 < dpad_y && !o->rope && !h->crawlingp) { // sliding
        i32 accx = 0;
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
            o->v_q8.x += accx;

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

    if (0 < h->jump_btn_buffer) {
        h->ground_impact_ticks = 6;
        hero_start_jump(g, o, HERO_JUMP_GROUND);
        if (h->sliding) { // boosting
            o->v_q8.x = (o->v_q8.x * HERO_VX_BOOST_SLIDE_Q8) >> 8;
        }
    }

    bool32 can_crawl = !h->sliding &&
                       !h->carrying &&
                       !h->trys_lifting &&
                       !h->grabbingp;
    if (can_crawl) {
        if (0 < dpad_y) {
            h->crawling = 1;
            if (!h->crawlingp) {
                hero_action_ungrapple(g, o);
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
        if (0 < dpad_y && h->action_jump && !h->action_jumpp) {
            o->moverflags &= ~OBJ_MOVER_ONE_WAY_PLAT;
            o->tomove.y += 1;
        }

        if (o->v_prev_q8.x == 0 && o->v_q8.x != 0) {
            o->animation = 0;
        }

        i32 vs = sgn_i32(o->v_q8.x);
        i32 va = abs_i32(o->v_q8.x);
        i32 ax = 0;

        if (h->skidding) {
            if (256 <= va) {
                o->v_q8.x = (o->v_q8.x * 220) / 256;
            } else {
                o->v_q8.x = (o->v_q8.x * 128) / 256;
            }
        }

        if (dpad_x != vs) {
            if (va >= HERO_VX_SPRINT) {
                if (!h->skidding) {
                    o->facing   = vs;
                    h->skidding = 15;
                }
            } else if (!h->skidding) {
                o->v_q8.x = (o->v_q8.x * 128) / 256;
            }
        }

        if (vs == 0 && h->skidding < 6) {
            ax = 200;
        } else if (dpad_x == +vs) { // press same dir as velocity
            if (0) {
            } else if (va < HERO_VX_WALK) {
                ax = lerp_i32(200, 20, va, HERO_VX_WALK);
            } else if (va < HERO_VX_SPRINT) {
                ax = lerp_i32(20, 4, va, HERO_VX_SPRINT);
            }
            ax = min_i32(ax, HERO_VX_SPRINT - va);
        } else if (dpad_x == -vs && h->skidding < 6) {
            ax = min_i32(70, va);
        }

        o->v_q8.x += ax * dpad_x;
        if (h->crawling) {
            o->v_q8.x = clamp_i32(o->v_q8.x, -HERO_VX_CRAWL, +HERO_VX_CRAWL);
        }
    }
}

void hero_restore_grounded_stuff(game_s *g, obj_s *o)
{
    hero_s *h    = &g->hero_mem;
    h->swimticks = HERO_SWIM_TICKS;
    hero_flytime_add_ui(g, o, 10000);
    if (h->flytime_added == 0) {
        h->jump_ui_may_hide = 1;
    }
    staminarestorer_respawn_all(g, o);
}