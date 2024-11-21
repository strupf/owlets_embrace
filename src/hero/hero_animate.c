// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    HERO_ANIMID_WALK,
    HERO_ANIMID_RUN,
    HERO_ANIMID_IDLE,
    HERO_ANIMID_SLEEP,
    HERO_ANIMID_DREAM_START,
    HERO_ANIMID_DREAM,
    HERO_ANIMID_DREAM_POOF,
    HERO_ANIMID_AIR,
    HERO_ANIMID_SWIM,
    HERO_ANIMID_CARRY_IDLE,
    HERO_ANIMID_CARRY_WALK,
    HERO_ANIMID_HANG_HOOK,
    HERO_ANIMID_CRAWL,
    HERO_ANIMID_LADDER,
    HERO_ANIMID_LIFT,
    HERO_ANIMID_ATTACK,
};

void hero_on_animate(g_s *g, obj_s *o)
{
    obj_sprite_s *sprite     = &o->sprites[0];
    hero_s       *h          = (hero_s *)o->heap;
    const bool32  was_idle   = h->is_idle;
    const u32     idle_animp = h->idle_anim;

    o->n_sprites    = 1;
    h->idle_anim    = 0;
    h->is_idle      = 0;
    sprite->trec.t  = asset_tex(TEXID_HERO);
    sprite->flip    = o->facing == -1 ? SPR_FLIP_X : 0;
    sprite->offs.x  = o->w / 2 - 32;
    sprite->offs.y  = o->h - 64 + 16;
    i32 animID      = 0;
    i32 frameID     = 0;
    i32 frameID_add = 0;
    i32 state       = hero_determine_state(g, o, h);

#if 0
    if (h->attack_tick || h->b_hold_tick) {
        state = HERO_STATE_NULL; // override other states
        sprite->offs.y -= 16;
        // sprite->offs.x += o->facing == 1 ? 0 : -60;
        sprite->offs.x += o->facing * 12;

        u32 attack_hold = h->attack_hold_tick + h->b_hold_tick;
        if (attack_hold) {
            frameID = (attack_hold < 6 ? 0 : (attack_hold < 10 ? 1 : 2));
            if (h->attack_flipflop == 0 && 10 <= attack_hold) {
                frameID = 2;
            }
            h->attack_hold_frame = frameID;
        } else {
            static const u8 frametimes[2][8] = {{0, 1, 2, 4, 6, 8, 10, 12},
                                                {0, 1, 2, 3, 4, 5, 7, 9}};

            for (i32 n = 8 - 1; 0 <= n; n--) {
                if (frametimes[0][n] <= h->attack_tick) {
                    frameID = n;
                    break;
                }
            }
            frameID = max_i32(frameID, h->attack_hold_frame);
        }
        frameID += 8 * h->attack_flipflop;
        animID = 16;

        rec_i32 frame_attack = {64 * frameID,
                                64 * animID,
                                64,
                                64};
        sprite->trec.r       = frame_attack;
        return;
    }
#else
    if (h->attack_tick || h->b_hold_tick) {
        state = HERO_STATE_NULL; // override other states
        sprite->offs.y -= 16;
        sprite->offs.x += o->facing == 1 ? 0 : -60;

        u32 attack_hold = h->attack_hold_tick + h->b_hold_tick;
        if (attack_hold) {
            frameID = (attack_hold < 6 ? 0 : (attack_hold < 10 ? 1 : 2));
            if (h->attack_flipflop == 0 && 10 <= attack_hold) {
                frameID = 2;
            }
            h->attack_hold_frame = frameID;
        } else {
            static const u8 frametimes[2][8] = {{0, 1, 2, 4, 6, 8, 10, 12},
                                                {0, 1, 2, 3, 4, 5, 7, 9}};

            for (i32 n = 8 - 1; 0 <= n; n--) {
                if (frametimes[h->attack_ID][n] <= h->attack_tick) {
                    frameID = n;
                    break;
                }
            }
            frameID = max_i32(frameID, h->attack_hold_frame);
        }

        if (h->attack_ID == 1) {
            animID = 31;
        } else {
            animID = 29 + h->attack_flipflop;
        }

        rec_i32 frame_attack = {128 * frameID,
                                64 * animID,
                                128,
                                64};
        sprite->trec.r       = frame_attack;
        return;
    }
#endif
    h->attack_hold_frame = 0;

    switch (state) {
    case HERO_STATE_GROUND: {
        sprite->offs.y -= 16;
        if (0 < h->impact_ticks && !h->holds_weapon) {
            if (h->holds_weapon) {
                animID  = 23;
                frameID = 0;
            } else {
                animID  = HERO_ANIMID_AIR; // "oof"
                frameID = 7;
            }

            break;
        }

        if (h->pushing) {
            animID  = 2;
            frameID = 12 + ((pltf_time() >> 3) & 3);
            break;
        }

        if (h->crouched) {
            if (h->crawl) {
                animID = HERO_ANIMID_CRAWL;
                o->animation += o->v_q8.x * o->facing;
                frameID = 8 + modu_i32(o->animation / 1500, 6);
            } else {
                animID = HERO_ANIMID_CRAWL - 1;
                if (h->crouched < HERO_CROUCHED_MAX_TICKS) {
                    frameID = 11 + (h->crouched * 2) / HERO_CROUCHED_MAX_TICKS;
                } else {
                    o->animation += 1;
                    frameID = 13 + modu_i32(o->animation / 14, 3);
                }
            }

            break;
        }
        if (h->crouch_standup) {
            animID  = HERO_ANIMID_CRAWL - 1;
            frameID = 11 + (h->crouch_standup * 2) / HERO_CROUCHED_MAX_TICKS;
            break;
        }

        if (o->rope) {
            ropenode_s *rn          = ropenode_neighbour(o->rope, o->ropenode);
            i32         dir         = sgn_i32(rn->p.x - o->pos.x);
            i32         ropestretch = rope_stretch_q8(g, o->rope);
            if (252 <= ropestretch && dir) {
                sprite->flip = 0 < dir ? 0 : SPR_FLIP_X;
                animID       = 1;
                frameID      = 12;
                if (260 <= ropestretch) {
                    frameID += (g->gameplay_tick >> 4) & 1;
                }

                break;
            }
        }

        if (o->v_q8.x != 0) {
            if (was_idle && h->idle_anim) {
                animID  = HERO_ANIMID_SLEEP;
                frameID = 3;
                break;
            }

            // "carrying/walking"
            if (h->holds_weapon) {
                animID = 22;
                if (o->facing == -1) frameID_add = 8;
            } else {
                animID = HERO_ANIMID_WALK;
            }

            i32 frameID_prev = (o->animation / 2000) & 7;
            i32 va           = abs_i32(o->v_q8.x);
            if (HERO_VX_SPRINT <= va && animID == HERO_ANIMID_WALK && !h->holds_weapon) {
                animID = HERO_ANIMID_RUN;
                o->animation += (va * 220) >> 8;
            } else {
                o->animation += va;
            }

            if (h->skidding && !h->holds_weapon) {
                animID  = HERO_ANIMID_RUN;
                frameID = 8 + (h->skidding < 4);
                break;
            }

            frameID = (o->animation / 2000) & 7;
            if (frameID_prev != frameID && (frameID & 3) == 1) {
                snd_play(SNDID_FOOTSTEP_LEAVES, rngr_f32(0.25f, 0.35f), rngr_f32(0.8f, 1.0f));
                v2_i32 posc = obj_pos_bottom_center(o);
                posc.x -= 16 + sgn_i32(o->v_q8.x) * 4;
                posc.y -= 30;
                rec_i32 trp = {0, 284, 32, 32};
                spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, posc, TEXID_MISCOBJ, trp, 12, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
            }
            break;
        }

        if (h->holds_weapon) {
            animID = 23; // "carrying/idle"
            if (o->facing == -1) frameID_add = 8;
        } else {
            animID = HERO_ANIMID_IDLE; // "carrying/idle"
        }

        h->is_idle = 1;
        h->idle_ticks++;
        if (!was_idle) {
            h->idle_ticks = 0;
        }

        if (idle_animp) {
            h->idle_anim = idle_animp + 1;
        } else if (!h->holds_weapon && 100 <= h->idle_ticks && rngr_i32(0, 512) == 0) {
            h->idle_anim = 1;
        }

#define HERO_TICKS_IDLE_SIT 60
        if (h->idle_anim) {
            animID = HERO_ANIMID_SLEEP;
            if (HERO_TICKS_IDLE_SIT <= h->idle_anim) {
                frameID = 12 + (((h->idle_anim - HERO_TICKS_IDLE_SIT) >> 6) & 1);
            } else {
                frameID = lerp_i32(0, 13, h->idle_anim, HERO_TICKS_IDLE_SIT);
                frameID = clamp_i32(frameID, 0, 12);
            }

        } else {
            if (o->v_prev_q8.x != 0) {
                o->animation = 0; // just got idle
            } else {
                o->animation++;
                i32 idlea = o->animation / 15;
                frameID   = idlea & 3;
                if (((idlea >> 2) % 3) == 1 && frameID <= 1) {
                    frameID += 4; // blink
                }
            }
        }

        break;
    }

    case HERO_STATE_LADDER: {
        animID  = HERO_ANIMID_LADDER; // "ladder"
        frameID = (-(o->pos.y >> 2)) & 7;
        break;
    }
    case HERO_STATE_SWIMMING: { // repurpose jumping animation for swimming
        if (inp_x()) {
            animID  = HERO_ANIMID_SWIM; // swim
            frameID = hero_swim_frameID(o->animation);
        } else {
            animID   = HERO_ANIMID_AIR; // "air"
            i32 swim = hero_swim_frameID_idle(o->animation);
            if (swim <= 1) {
                sprite->offs.y += swim == 0 ? 0 : -2;
                frameID = 2;
            } else {
                sprite->offs.y += swim - 2;
                frameID = 2 + ((swim - 2) % 3);
            }
        }
        break;
    }
    case HERO_STATE_CLIMB: {
        i32 climbdir = inp_y();
        if (!hero_stamina_left(g, o)) {
            climbdir = +1;
        }
        if (h->impact_ticks) {
            climbdir = 0;
        }

        switch (climbdir) {
        case -1:
            animID  = 9;
            frameID = 8 + modu_i32(o->animation >> 3, 6);
            break;
        case +0:
            animID  = 8;
            frameID = h->impact_ticks ? 6 : 8;
            break;
        case +1:
            animID  = 8;
            frameID = 7;
            break;
        }
        break;
    }
    case HERO_STATE_AIR: {
        if (o->rope) {
            animID          = HERO_ANIMID_HANG_HOOK;
            ropenode_s *rn2 = ropenode_neighbour(o->rope, o->ropenode);
            v2_i32      rdt = v2_sub(rn2->p, o->ropenode->p);

            f32 ang = atan2f((f32)rdt.y, -(f32)o->facing * (f32)rdt.x);

            frameID = -(i32)((ang * 4.f)) - 3;
            frameID = clamp_i32(frameID, 0, 5);
            sprite->offs.y += 12;
            break;
        }

        if (h->stomp) {
            animID  = 7;
            frameID = 9 + min_i32(4, h->stomp >> 2);
            break;
        }

        if (h->holds_weapon) {
            sprite->offs.y -= 16;
            animID  = 22;
            frameID = 6 + ((o->animation >> 4) & 1);
            if (o->facing == -1) frameID_add = 8;
            break;
        }

        if (h->walljump_tick) {
            i32 wj_s    = sgn_i32(h->walljump_tick);
            i32 wj_tick = abs_i32(h->walljump_tick) -
                          (WALLJUMP_MOM_TICKS - WALLJUMP_ANIM_TICKS);
            if (0 <= wj_tick) {
                animID       = 8;
                frameID      = 9 + 1 - (2 * (wj_tick - 1)) / WALLJUMP_ANIM_TICKS;
                sprite->flip = wj_s == 1 ? SPR_FLIP_X : 0;
                break;
            }
        }

        animID = HERO_ANIMID_AIR; // "air"
        if (h->impact_ticks) {
            frameID = 0;
            sprite->offs.y -= 16;
        } else if (+100 <= o->v_q8.y) {
            frameID = 6;
        } else if (-100 <= o->v_q8.y) {
            frameID = 5;
        } else if (-400 <= o->v_q8.y) {
            frameID = 4;
        } else if (-800 <= o->v_q8.y) {
            frameID = 3;
        } else {
            frameID = 2;
        }
        break;
    }
    case HERO_STATE_DEAD: {
        sprite->offs.y -= 16;
        frameID = 0;
        animID  = HERO_ANIMID_WALK;
        break;
    }
    }

    rec_i32 frame  = {64 * (frameID + frameID_add),
                      64 * animID,
                      64,
                      64};
    sprite->trec.r = frame;
}