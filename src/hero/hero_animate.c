// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void hero_on_animate(game_s *g, obj_s *o)
{
    obj_sprite_s *sprite = &o->sprites[0];
    o->n_sprites         = 1;
    sprite->trec.t       = asset_tex(TEXID_HERO);
    sprite->flip         = o->facing == -1 ? SPR_FLIP_X : 0;
    sprite->offs.x       = o->w / 2 - 32;
    sprite->offs.y       = o->h - 64 + 16;
    sprite->mode         = ((o->invincible_tick >> 2) & 1) ? SPR_MODE_INV : 0;
    hero_s *h            = &g->hero_mem;
    i32     animID       = 0;
    i32     frameID      = 0;
    i32     state        = hero_determine_state(g, o, h);

    const bool32 was_idle   = h->is_idle;
    const i32    idle_animp = h->idle_anim;
    h->idle_anim            = 0;
    h->is_idle              = 0;

    if (h->sliding) {
        state = -1; // override other states
        sprite->offs.y -= 16;
        animID  = 3;
        frameID = 0;
    }
    if (h->attack_tick || h->attack_hold_tick) {
        state = -1; // override other states
        sprite->offs.y -= 16;
        sprite->offs.x += o->facing == 1 ? +12 : -6;
        animID = 11 + h->attack_flipflop;

        if (h->attack_hold_tick) {
            frameID = 0;
        } else {
            if (h->attack_tick <= 15) {
                frameID = lerp_i32(0, 5, h->attack_tick, 15);
            } else {
                frameID = 4;
            }
            frameID = min_i(frameID, 4);
        }
    }

    switch (state) {
    case HERO_STATE_GROUND: {
        if (0 < h->ground_impact_ticks && !h->carrying) {
            sprite->offs.y -= 4;
            animID  = 6; // "oof"
            frameID = 6;
            break;
        }

        sprite->offs.y -= 16;
        bool32 crawl = 0 < inp_y();

#if 1
        crawl = 0;
#endif

        if (crawl) {
            animID = 0; // todo
            if (o->vel_q8.x != 0) {
            } else {
            }
        } else {
            if (o->vel_q8.x != 0) {
                if (was_idle) {
                    animID  = 2;
                    frameID = 3;
                    break;
                }
                animID           = h->carrying ? 12 : 0; // "carrying/walking"
                i32 frameID_prev = (o->animation / 2000) & 7;
                o->animation += abs_i(o->vel_q8.x);
                frameID = (o->animation / 2000) & 7;
                if (frameID_prev != frameID && (frameID & 1) == 0) {
                    // snd_play_ext(SNDID_STEP, 0.5f, 1.f);
                    v2_i32 posc = obj_pos_bottom_center(o);
                    posc.x -= 16 + sgn_i(o->vel_q8.x) * 4;
                    posc.y -= 30;
                    rec_i32 trp = {0, 284, 32, 32};
                    spritedecal_create(g, RENDER_PRIO_HERO - 1, NULL, posc, TEXID_MISCOBJ, trp, 12, 5, rngr_i32(0, 1) ? 0 : SPR_FLIP_X);
                }
                break;
            }

            // idle standing
            if (h->carrying) {
                animID = 11;
                o->animation++;
                i32 carryf = (o->animation >> 5) & 3;
                frameID    = (carryf == 0 ? 1 : 0);
            } else {
                animID     = 1; // "carrying/idle"
                h->is_idle = 1;
                h->idle_ticks++;
                if (!was_idle) {
                    h->idle_ticks = 0;
                }

                if (idle_animp) {
                    h->idle_anim = idle_animp + 1;
                } else if (100 <= h->idle_ticks && rngr_i32(0, 256) == 0) {
                    h->idle_anim = 1;
                }

#define HERO_TICKS_IDLE_SIT 60
                if (h->idle_anim) {
                    animID = 2;
                    if (HERO_TICKS_IDLE_SIT <= h->idle_anim) {
                        frameID = 12 + (((h->idle_anim - HERO_TICKS_IDLE_SIT) >> 6) & 1);
                    } else {
                        frameID = lerp_i32(0, 13, h->idle_anim, HERO_TICKS_IDLE_SIT);
                        frameID = clamp_i32(frameID, 0, 12);
                    }

                } else {
                    if (o->vel_prev_q8.x != 0) {
                        o->animation = 0; // just got idle
                    } else {
                        o->animation += 100;
                        frameID = (o->animation / 1500) & 3;
                    }
                }
            }
        }

        break;
    }

    case HERO_STATE_LADDER: {
        animID       = 8; // "ladder"
        frameID      = 0;
        sprite->flip = ((o->animation >> 3) & 1) ? SPR_FLIP_X : 0;
        break;
    }
    case HERO_STATE_SWIMMING: { // repurpose jumping animation for swimming
        if (inp_x()) {
            animID  = 9; // swim
            frameID = ((o->animation >> 3) % 6);
        } else {
            animID   = 6; // "air"
            i32 swim = ((o->animation >> 4) & 7);
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
        if (h->walljumpticks) {
            animID       = 6; // "walljump"
            sprite->flip = 0 < o->vel_q8.x ? 0 : SPR_FLIP_X;
            break;
        }

        if (h->carrying) {
            sprite->offs.y -= 16;
            animID = 12; // "carrying air"
            break;
        }

        animID = 6; // "air"
        if (0 < h->ground_impact_ticks) {
            frameID = 0;
        } else if (h->gliding) {
            frameID = 2 + (8 <= (o->animation & 63));
        } else if (+600 <= o->vel_q8.y) {
            frameID = 4 + ((o->animation >> 2) & 1);
        } else if (-10 <= o->vel_q8.y) {
            frameID = 3;
        } else if (-300 <= o->vel_q8.y) {
            frameID = 2;
        } else {
            frameID = 1;
        }
        break;
    }
    case HERO_STATE_DEAD: {
        sprite->offs.y -= 16;
        frameID = 0;
        animID  = 0;
    } break;
    }

    rec_i32 frame  = {frameID * 64, animID * 64, 64, 64};
    sprite->trec.r = frame;
}