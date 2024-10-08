// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
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

void hero_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *sprite     = &o->sprites[0];
    hero_s       *h          = &g->hero_mem;
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

    if (h->sliding) {
        state = HERO_STATE_NULL; // override other states
        sprite->offs.y -= 16;
        animID  = HERO_ANIMID_DREAM_START;
        frameID = 0;
    }

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

    h->attack_hold_frame = 0;

    switch (state) {
    case HERO_STATE_GROUND: {
        sprite->offs.y -= 16;
        if (0 < h->ground_impact_ticks && !h->carrying && !h->holds_weapon) {
            if (h->holds_weapon) {
                animID  = 23;
                frameID = 0;
            } else {
                animID  = HERO_ANIMID_AIR; // "oof"
                frameID = 6;
            }

            break;
        }

        if (h->crawling) {
            animID = HERO_ANIMID_CRAWL;
            if (!h->crawlingp) {
                o->animation         = 0;
                h->crawling_to_stand = 0;
            }
#define HERO_START_CRAWL_TICK 10
            if (h->crawling_to_stand < HERO_START_CRAWL_TICK) {
                h->crawling_to_stand++;

                frameID = min_i32((h->crawling_to_stand * 6) / HERO_START_CRAWL_TICK, 5);
            } else {
                o->animation += o->v_q8.x;
                frameID = 8 + ((u32)o->animation / 1500) % 6;
            }
            break;
        }

        if (h->crawlingp) {
            o->animation         = 0;
            h->crawling_to_stand = 5;
        }

        if (h->crawling_to_stand) {
            h->crawling_to_stand--;
            animID  = HERO_ANIMID_CRAWL;
            frameID = (h->crawling_to_stand * 6) / 8;
            break;
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
                animID = h->carrying ? HERO_ANIMID_CARRY_WALK : HERO_ANIMID_WALK;
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
                if (4 <= h->skidding) {
                    animID  = HERO_ANIMID_RUN;
                    frameID = 8;
                } else {
                    animID  = HERO_ANIMID_RUN;
                    frameID = 9;
                }

                break;
            }

            frameID = (o->animation / 2000) & 7;
            if (frameID_prev != frameID && (frameID & 3) == 1) {
                snd_play(SNDID_FOOTSTEP_LEAVES, rngr_f32(0.25f, 0.35f), rngr_f32(0.8f, 1.0f));
                v2_i32 posc = obj_pos_bottom_center(o);
                posc.x -= 16 + sgn_i(o->v_q8.x) * 4;
                posc.y -= 30;
                rec_i32 trp = {0, 284, 32, 32};
                spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, posc, TEXID_MISCOBJ, trp, 12, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
            }
            break;
        }

        // idle standing
        if (h->carrying) {
            o->animation++;
            i32 carryf = (o->animation >> 5) & 3;
            animID     = HERO_ANIMID_CARRY_IDLE;
            frameID    = (carryf == 0 ? 1 : 0);
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
    case HERO_STATE_AIR: {
        if (o->rope) {
            animID          = HERO_ANIMID_HANG_HOOK;
            ropenode_s *rn2 = ropenode_neighbour(o->rope, o->ropenode);
            v2_i32      rv1 = o->ropenode->p;
            v2_i32      rv2 = rn2->p;
            v2_i32      rdt = v2_sub(rv2, rv1);

            f32 ang = atan2f((f32)rdt.y, -(f32)o->facing * (f32)rdt.x);

            frameID = -(i32)((ang * 4.f)) - 3;
            frameID = clamp_i32(frameID, 0, 5);
            sprite->offs.y += 12;
            break;
        }

        if (h->carrying) {
            sprite->offs.y -= 16;
            animID = HERO_ANIMID_CARRY_WALK; // "carrying air"
            break;
        }

        if (h->stomp) {
            animID  = 7;
            frameID = 7 + min_i32(4, h->stomp >> 2);
            break;
        }

        if (h->holds_weapon) {
            sprite->offs.y -= 16;
            animID  = 22;
            frameID = 6 + ((o->animation >> 4) & 1);
            if (o->facing == -1) frameID_add = 8;
            break;
        }

        animID = HERO_ANIMID_AIR; // "air"
        if (0 < h->ground_impact_ticks) {
            frameID = 0;
        } else if (+500 <= o->v_q8.y) {
            frameID = 5;
        } else if (+300 <= o->v_q8.y) {
            frameID = 4;
        } else if (-200 <= o->v_q8.y) {
            frameID = 3;
        } else if (-600 <= o->v_q8.y) {
            frameID = 2;
        } else {
            frameID = 1;
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