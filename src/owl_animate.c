// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl.h"

void owl_on_animate(g_s *g, obj_s *o)
{
    owl_s *h  = (owl_s *)o->heap;
    i32    st = owl_state_check(g, o);

    // animate stamina ui
    if (h->stamina_added_delay_ticks) {
        h->stamina_added_delay_ticks--;
    } else {
        h->stamina_added = max_i32((i32)h->stamina_added - 32, 0);
    }
    if (h->stamina < h->stamina_max || h->stamina_added) {
        h->stamina_ui_show = 1;
    } else {
        h->stamina_ui_show = 0;
    }
    if (st == OWL_ST_WATER) {
        owl_stamina_modify(o, h->stamina_max);
        h->stamina_ui_show = 0;
    }
    if (h->stamina_ui_show && h->stamina_ui_fade_ticks < OWL_STAMINA_TICKS_UI_FADE) {
        h->stamina_ui_fade_ticks++;
    }
    if (!h->stamina_ui_show && h->stamina_ui_fade_ticks) {
        h->stamina_ui_fade_ticks--;
    }

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
    spr->offs.x       = (o->w >> 1) - 32;
    spr->offs.y       = (o->h) - 64 + 1;

    rope_s *r  = o->rope;
    i32     fx = 0;
    i32     fy = 0;
    i32     fw = 64;
    i32     fh = 64;

    // adds 24 to y frame if in "attack" stance
    // manually unset to ignore the stance
    bool32 consider_stance = h->stance;

    if (h->stance == OWL_STANCE_ATTACK) {
        if (h->attack_tick) {
            st              = OWL_ST_NULL;
            consider_stance = 0;
            spr->offs.x -= 16;
            fw = 96;
            fy = 46 + h->attack_flipflop;
            fx = ani_frame(ANIID_OWL_ATTACK, h->attack_tick);

            if (h->stance_swap_tick) {
                fx                   = min_i32(fx, 5);
                h->attack_last_frame = fx;
            }
        } else if (h->stance_swap_tick) {
            st              = OWL_ST_NULL;
            consider_stance = 0;
            spr->offs.x -= 16;
            fw = 96;
            fy = 47 - h->attack_flipflop;
            if (0) {
            } else if (h->stance_swap_tick < owl_swap_ticks() - 4) {
                fx = 2;
            } else if (h->stance_swap_tick < owl_swap_ticks() - 1) {
                fx = 1;
            } else {
                fx = 0;
            }
        }
    }

    switch (st) {
    case -1: break;
    case OWL_ST_AIR: {
        ropenode_s *rn = o->ropenode;
        if (r) {
            v2_i32 rdt = v2_i32_sub(rn->p, o->ropenode->p);
            if (rdt.y < -100 && 254 <= rope_stretch_q8(g, r)) {
                // actively swinging
                f32 ang = atan2_f((f32)rdt.y, -(f32)o->facing * (f32)rdt.x);

                fy = 12;
                fx = -(i32)((ang * 4.f)) - 3;
                fx = 3 + clamp_i32(fx, 0, 6);
                spr->offs.y += 12;

                i32 lsq = v2_i32_len_appr(o->v_q12);
                if (250000 <= lsq) {
                    v2_i32 pcenter = obj_pos_center(o);
                    if (350000 <= lsq || (g->tick & 1)) {
                        particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_SWING, pcenter);
                    }
                }
            } else {
                fy = 12;
                spr->offs.y += 12;

                if (+500 <= o->v_q12.y) {
                    fx = 0;
                } else if (-100 <= o->v_q12.y) {
                    fx = 1;
                } else {
                    fx = 2;
                }
            }
        } else if (h->air_stomp) {
            spr->offs.y += 16;
            fy = 4;
            fx = lerp_i32(0, 4, h->air_stomp, OWL_AIR_STOMP_MAX);
        } else if (h->jump_ground_ticks) {
            // visually glue owl to ground for 2 more frames after
            // jumping off the ground
            h->jump_ground_ticks--;
            fy = 7;
            fx = (h->jump_ground_ticks == 0);
            spr->offs.y -= o->pos.y - h->jump_from_y;
        } else {
            i32 vanim = o->v_q12.y + pow2_i32(h->air_gliding) * 10;
            fy        = 7;
            spr->offs.y += 16;
            if (0) {
            } else if (+Q_VOBJ(0.3) <= vanim) {
                fx = 6;
            } else if (-Q_VOBJ(0.3) <= vanim) {
                fx = 5;
            } else if (-Q_VOBJ(1.0) <= vanim) {
                fx = 4;
            } else if (-Q_VOBJ(3.0) <= vanim) {
                fx = 3;
            } else {
                fx = 2;
            }

            if (h->stance_swap_tick) {
                fx += 7;
            }
        }

        break;
    }
    case OWL_ST_GROUND: {
        if (h->ground_pushing) {
            h->ground_pushing_anim++;
            fy = 15;
            fx = 4 + ((h->ground_pushing_anim >> 3) & 3);
        } else if (h->ground_stomp_landing_ticks) {
            fy = 4;
            fx = 5 + lerp_i32(0, 2, h->ground_stomp_landing_ticks, OWL_STOMP_LANDING_TICKS);
        } else if (h->crouch) {
            // duck down/crawl
            fy = 11;
            if (h->crouch < OWL_CROUCH_MAX_TICKS) {
                fx = lerp_i32(0, 2, h->crouch, OWL_CROUCH_MAX_TICKS - 1);
            } else if (h->crouch_crawl) {
                fx = 5 + modu_i32(shr_balanced_i32(h->crouch_crawl_anim, 14), 6);
            } else {
                fx = 2 + (shr_balanced_i32(h->crouch_crawl_anim, 4) % 3);
            }
        } else if (h->stance_swap_tick) {
            fy         = 14;
            i32 f_swap = 0;

            // TODO: smooth out swapping after attack swing
            if (h->stance_swap_tick < 12) {
                f_swap = lerp_i32(0, 5, h->stance_swap_tick, 12);
            } else if (owl_swap_ticks() - 3 <= h->stance_swap_tick) {
                f_swap = 3;
            } else if (owl_swap_ticks() - 7 <= h->stance_swap_tick) {
                f_swap = 4;
            } else {
                f_swap = 5;
            }
            fx = 6 + f_swap;
        } else if (inp_btn(INP_DU) && !inp_x()) {
            // looking up
            fx = 13;
            fy = 2;
        } else if (h->crouch_standup_ticks) {
            // standing up
            fy = 11;
            fx = lerp_i32(0, 2, h->crouch_standup_ticks, OWL_CROUCH_STANDUP_TICKS);
        } else if (h->ground_skid_ticks) {
            // cancel sprint
            fy = 1;
            fx = 8 + (h->ground_skid_ticks < 5);
        } else if (h->ground_impact_ticks) {
            h->ground_impact_ticks--;
            fy = 7;
            fx = 7 + (h->ground_impact_ticks == 0 || 3 < h->ground_impact_ticks);
        } else if (o->v_q12.x) {
            // walking/sprinting
            if (h->sprint) {
                fy = 1;
                fx = modu_i32(shr_balanced_i32(h->ground_walking, 15), 8);
            } else {
                fy = 0;
                fx = modu_i32(shr_balanced_i32(h->ground_walking, 15), 8);
            }
        } else {
            // idle
            fy = 2;
            h->ground_idle_ticks++;
            fx = (h->ground_idle_ticks >> 3) & 7;
        }
        break;
    }
    case OWL_ST_WATER: {
        spr->offs.y += 16;
        h->swim_anim++;

        if (h->swim_sideways) {
            i32 swim = owl_swim_frameID(h->swim_anim);
            fy       = 8; // swim
            fx       = swim;

            // sfx
            i32 swim2 = owl_swim_frameID(h->swim_anim - 1);
            if ((swim2 == 1 && swim == 2)) {
                snd_play(SNDID_WATER_SWIM_1, 0.25f, rngr_f32(0.9f, 1.1f));
            }
        } else {
            fy = 7; // "air"
            spr->offs.y += 3;
            i32 swim = owl_swim_frameID_idle(h->swim_anim);
            if (swim <= 1) {
                spr->offs.y += swim == 0 ? 0 : -2;
                fx = 2;
            } else {
                spr->offs.y += swim - 2;
                fx = 2 + ((swim - 2) % 3);
            }

            // sfx
            i32 swim2 = owl_swim_frameID_idle(h->swim_anim - 1);
            if (((swim2 == 1 || swim2 == 4) && swim != swim2)) {
                snd_play(SNDID_WATER_SWIM_2, 0.20f, rngr_f32(0.9f, 1.1f));
            }
        }
        break;
    }
    case OWL_ST_CLIMB: {
        spr->offs.y += 16;

        // smoothing of the player position on snapping to a ladder
        if (h->climb_camx_smooth <= 2) { // two ticks, starts on 1
            i32 cx = o->pos.x + (OWL_W >> 1);
            spr->offs.x -= (cx - h->climb_from_x) / 2;
            h->climb_from_x = (cx + h->climb_from_x) / 2; // average closer
        }

        switch (h->climb) {
        case OWL_CLIMB_LADDER: {
            fy = 13;
            fx = shr_balanced_i32(h->climb_anim, 2) & 7;
            break;
        }
        }
        break;
    }
    }

    if (consider_stance) {
        fy += 24;
    }

    spr->trec.t = asset_tex(TEXID_HERO);
    spr->flip   = o->facing == -1 ? SPR_FLIP_X : 0;
    spr->trec.w = fw;
    spr->trec.h = fh;
    spr->trec.x = fw * fx;
    spr->trec.y = fh * fy;
}