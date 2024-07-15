// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    HERO_ANIMID_WALK,
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
    const i32     idle_animp = h->idle_anim;

    h->idle_anim   = 0;
    h->is_idle     = 0;
    sprite->trec.t = asset_tex(TEXID_HERO);
    sprite->flip   = o->facing == -1 ? SPR_FLIP_X : 0;
    sprite->offs.x = o->w / 2 - 32;
    sprite->offs.y = o->h - 64 + 16;
    i32 animID     = 0;
    i32 frameID    = 0;
    i32 state      = hero_determine_state(g, o, h);

    if (h->sliding) {
        state = -1; // override other states
        sprite->offs.y -= 16;
        animID  = HERO_ANIMID_DREAM_START;
        frameID = 0;
    }

    o->n_sprites = 1;
    if (h->attack_tick || h->b_hold_tick) {
        state = -1; // override other states
        sprite->offs.y -= 16;
        sprite->offs.x += o->facing == 1 ? +12 : -7;
        animID          = HERO_ANIMID_ATTACK;
        u32 attack_hold = h->attack_hold_tick + h->b_hold_tick;

        if (attack_hold) {
            frameID = (attack_hold < 6 ? 0 : (attack_hold < 10 ? 1 : 2));
            if (h->attack_flipflop == 0 && 10 <= attack_hold) {
                frameID = 2;
            }
            h->attack_hold_frame = frameID;
        } else {
            // times when to display specific frames
            static const u8 frametimes[8] = {0, 1, 2, 4, 6, 10, 12, 14};

            for (i32 n = ARRLEN(frametimes) - 1; 0 <= n; n--) {
                if (frametimes[n] <= h->attack_tick) {
                    frameID = n;
                    break;
                }
            }
            frameID = max_i32(frameID, h->attack_hold_frame);
        }
        frameID += h->attack_flipflop * 8;
    } else {
        h->attack_hold_frame = 0;
    }

    switch (state) {
    case HERO_STATE_GROUND: {
        sprite->offs.y -= 16;
        if (0 < h->ground_impact_ticks && !h->carrying) {
            animID  = HERO_ANIMID_AIR; // "oof"
            frameID = 6;
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
                o->animation += o->vel_q8.x;
                frameID = 8 + ((u32)o->animation / 1500) % 6;
            }
        } else {
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

            if (o->vel_q8.x != 0) {
                if (was_idle) {
                    animID  = HERO_ANIMID_SLEEP;
                    frameID = 3;
                    break;
                }
                animID           = h->carrying ? HERO_ANIMID_CARRY_WALK : HERO_ANIMID_WALK; // "carrying/walking"
                i32 frameID_prev = (o->animation / 2000) & 7;
                o->animation += abs_i(o->vel_q8.x);
                frameID = (o->animation / 2000) & 7;
                if (frameID_prev != frameID && (frameID & 3) == 1) {
                    snd_play(SNDID_FOOTSTEP_LEAVES, 0.6f, rngr_f32(0.9f, 1.0f));
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
                animID = HERO_ANIMID_CARRY_IDLE;
                o->animation++;
                i32 carryf = (o->animation >> 5) & 3;
                frameID    = (carryf == 0 ? 1 : 0);
            } else {
                animID     = HERO_ANIMID_IDLE; // "carrying/idle"
                h->is_idle = 1;
                h->idle_ticks++;
                if (!was_idle) {
                    h->idle_ticks = 0;
                }

                if (idle_animp) {
                    h->idle_anim = idle_animp + 1;
                } else if (100 <= h->idle_ticks && rngr_i32(0, 512) == 0) {
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
                    if (o->vel_prev_q8.x != 0) {
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
            frameID = ((o->animation >> 3) % 6);
        } else {
            animID   = HERO_ANIMID_AIR; // "air"
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

        animID = HERO_ANIMID_AIR; // "air"
        if (0 < h->ground_impact_ticks) {
            frameID = 0;
        } else if (+500 <= o->vel_q8.y) {
            frameID = 5;
        } else if (+300 <= o->vel_q8.y) {
            frameID = 4;
        } else if (-200 <= o->vel_q8.y) {
            frameID = 3;
        } else if (-600 <= o->vel_q8.y) {
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
    rec_i32 frame  = {frameID * 64, animID * 64, 64, 64};
    sprite->trec.r = frame;
}

void hero_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    hero_s *h = (hero_s *)&g->hero_mem;
    if (h->b_hold_tick < 12) return;
    gfx_ctx_s ctx  = gfx_ctx_display();
    v2_i32    oc   = v2_add(obj_pos_center(o), cam);
    i32       t    = g->save.tick * 20000;
    v2_i32    offs = {
        (26 * sin_q16(t)) >> 16,
        (26 * cos_q16(t)) >> 16};
    v2_i32 offs2 = {
        (26 * sin_q16(t - 20000)) >> 16,
        (26 * cos_q16(t - 20000)) >> 16};

    offs  = v2_add(oc, offs);
    offs2 = v2_add(oc, offs2);

    gfx_ctx_s ctxw = ctx;
    ctxw.pat       = gfx_pattern_4x4(B4(0111),
                                     B4(1010),
                                     B4(1101),
                                     B4(1010));
    gfx_lin_thick(ctx, oc, offs2, GFX_COL_WHITE, 6);
    gfx_lin_thick(ctx, oc, offs2, GFX_COL_BLACK, 4);

    gfx_lin_thick(ctx, oc, offs, GFX_COL_WHITE, 10);
    gfx_lin_thick(ctx, oc, offs, GFX_COL_BLACK, 8);
    gfx_lin_thick(ctxw, oc, offs, GFX_COL_WHITE, 3);
}