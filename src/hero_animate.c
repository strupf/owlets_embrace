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
    obj_sprite_s *sprite = &o->sprites[0];
    hero_s       *h      = (hero_s *)o->heap;
    inp_s         inp    = inp_cur();
    rope_s       *r      = o->rope;
    ropenode_s   *rn     = r ? ropenode_neighbour(r, o->ropenode) : 0;

    if (!(h->idle_state & 1)) {
        h->idle_animID    = 0;
        h->idle_tick      = 0;
        h->idle_anim_tick = 0;
    }

    v2_i32 hc = obj_pos_center(o);
    hc.x -= o->facing * 10;
    hc.y += 2;
    hc.x = (hc.x << 8) + (o->subpos_q12.x >> 4);
    hc.y = (hc.y << 8) + (o->subpos_q12.y >> 4);

    o->n_sprites   = 1;
    sprite->trec.t = asset_tex(TEXID_HERO);
    sprite->flip   = o->facing == -1 ? SPR_FLIP_X : 0;
    sprite->offs.x = o->w / 2 - 32;
    sprite->offs.y = o->h - 64 + 16 + 1;
    i32 fr_y       = 0;
    i32 fr_x       = 0;
    i32 state      = hero_get_actual_state(g, o);
    sprite->trec.w = 64;
    sprite->trec.h = 64;
    if (h->impact_ticks) {
        h->impact_ticks--;
    }
    if (h->hook_throw_anim_tick) {
        h->hook_throw_anim_tick--;
        sprite->offs.y -= 16;
        fr_y  = 5;
        fr_x  = lerp_i32(10, 14, HERO_THROW_HOOK_ANIM_TICKS - h->hook_throw_anim_tick, HERO_THROW_HOOK_ANIM_TICKS);
        fr_x  = min_i32(fr_x, 13);
        state = 0;
    }

    if ((HERO_HURT_TICKS * 3) / 4 <= h->hurt_ticks) {
        state = 0;
        sprite->offs.y -= 16;
        fr_y = 16;
        fr_x = 8;
    } else if (o->health == 1) {
        // o->n_sprites = 1 <= (g->gameplay_tick % 4);
    } else {
        o->n_sprites = 1;
    }

    if (h->attack_tick) {
        fr_y  = 22 + h->attack_flipflop;
        fr_x  = ani_frame(ANIID_HERO_ATTACK, h->attack_tick);
        fr_x  = max_i32(fr_x, h->attack_last_frame);
        state = 0;
        sprite->offs.x -= 16;
        sprite->trec.w = 96;
        sprite->trec.h = 64;
        sprite->offs.y -= 16;
        h->attack_last_frame = min_i32(fr_x, 2);
        if (h->b_hold_tick && fr_x == 6) {
            h->attack_tick       = 0;
            h->attack_ID         = 0;
            h->attack_last_frame = 0;
            if (h->attack_flipflop == 1) {
                fr_y = 22;
                fr_x = 2;
            }
        }
    } else if (h->b_hold_tick && h->mode == HERO_MODE_COMBAT) {
        h->attack_tick       = 0;
        h->attack_ID         = 0;
        h->attack_last_frame = 0;
        sprite->offs.x -= 16;
        sprite->trec.w = 96;
        sprite->trec.h = 64;
        sprite->offs.y -= 16;
        if (h->attack_flipflop == 0) {
            fr_y = 22;
            fr_x = 6;
        } else {
            fr_y = 22;
            fr_x = 2;
        }

        state = 0;
    }

    if (o->health == 0) {
        fr_y = 16;

        switch (state) {
        case HERO_ST_GROUND: {
            sprite->offs.y -= 16;

            if (h->dead_bounce) {
                fr_x = 9 + (10 <= o->animation);
            } else {
                fr_x = 8;
            }

            break;
        }
        case HERO_ST_AIR: {
            fr_x = 0 + ((o->animation / 4) & 7);
            break;
        }
        case HERO_ST_WATER: {
            break;
        }
        }

        state = 0;
    }

    if (h->squish) {
        sprite->offs.y -= 4;
        state = 0;
        if (h->squish < HERO_SQUISH_TICKS) {
            fr_x = lerp_i32(0, 10, h->squish, HERO_SQUISH_TICKS);
            fr_y = 10;
        } else {
            o->n_sprites = 0;
        }
    }

    switch (state) {
    case HERO_ST_GROUND: {
        sprite->offs.y -= 16;

        if (0 < h->impact_ticks) {
            fr_y = 7; // "oof"
            fr_x = (h->impact_ticks <= 2 ? 8 : 7);
            break;
        }

        if (h->grabbing) {
            o->animation++;
            fr_y = 15;

            if (abs_i32(h->grabbing) == 1) { // static
                fr_x = 9 + ((o->animation >> 5) & 1);
            } else { // actively push/pull
                if (sgn_i32(h->grabbing) == o->facing) {
                    fr_x = 4 + ((o->animation >> 3) & 3);
                } else {
                    fr_x = 0 + ((o->animation >> 3) & 3);
                }
            }
            break;
        }

        if (h->crouched) {
            fr_y = 11;
            if (h->crouched < 0) { // crawling
                                   // o->animation += o->v_q8.x * o->facing;
                fr_x = 5 + modu_i32(o->animation / 1500, 6);
            } else { // sit
                if (h->crouched < HERO_CROUCHED_MAX_TICKS) {
                    fr_x = (h->crouched * 2) / HERO_CROUCHED_MAX_TICKS;
                } else {
                    o->animation += 1;
                    fr_x = 2 + modu_i32(o->animation / 14, 3);
                }
            }

            break;
        }
        if (h->crouch_standup) {
            fr_y = 11;
            fr_x = (h->crouch_standup * 2) / HERO_CROUCHED_MAX_TICKS;
            break;
        }

        if (r) {
            i32 dir         = sgn_i32(rn->p.x - o->pos.x);
            i32 ropestretch = rope_stretch_q8(g, r);
            if (252 <= ropestretch && dir) {
                sprite->flip = 0 < dir ? 0 : SPR_FLIP_X;
                fr_y         = 12;
                fr_x         = 10;
                if (260 <= ropestretch) {
                    fr_x += (g->tick >> 4) & 1;
                }

                break;
            }
        }

        if (o->v_q12.x != 0) {
            // "carrying/walking"
            fr_y = HERO_ANIMID_WALK;

            i32 frameID_prev = (o->animation / 2000) & 7;
            i32 va           = abs_i32(o->v_q12.x);
            if (HERO_VX_SPRINT <= va && fr_y == HERO_ANIMID_WALK) {
                fr_y = HERO_ANIMID_RUN;
                // o->animation += (va * 220) >> 8;
            } else {
                //  o->animation += va;
            }

            if (h->skidding) {
                fr_y = HERO_ANIMID_RUN;
                fr_x = 8 + (h->skidding < 4);
                break;
            }

            fr_x = (o->animation / 2000) & 7;
            if (frameID_prev != fr_x && (fr_x & 3) == 1) {
                snd_play(SNDID_FOOTSTEP_LEAVES, rngr_f32(0.25f, 0.35f), rngr_f32(0.8f, 1.0f));
                v2_i32 posc = obj_pos_bottom_center(o);
                posc.x -= sgn_i32(o->v_q12.x) * 4;
                posc.y -= 2;
                i32 pID = (abs_i32(o->v_q12.x) == HERO_VX_SPRINT ? PARTICLE_EMIT_ID_HERO_WALK_FAST
                                                                 : PARTICLE_EMIT_ID_HERO_WALK);
                particle_emit_ID(g, pID, posc);
            }
            break;
        }

        fr_y = HERO_ANIMID_IDLE; // "carrying/idle"
        if (inp_y() < 0) {
            fr_x = 13;
            fr_y = 2;
            break;
        }

        if (h->b_hold_tick) {
            fr_y = 14;
            fr_x = 6 + ani_frame(ANIID_PREPARE_SWAP, h->b_hold_tick);
            break;
        }

        o->animation++;
        i32 idlea = o->animation / 15;
        fr_x      = idlea & 3;
        if (((idlea >> 2) % 3) == 1 && fr_x <= 1) {
            fr_x += 4; // blink
        }
        break;
    }
    case HERO_ST_LADDER: {
        fr_y = HERO_ANIMID_LADDER; // "ladder"
        fr_x = (o->animation >> 2) & 7;
        break;
    }
    case HERO_ST_WATER: { // repurpose jumping animation for swimming
        sprite->offs.y += 4;
        if (inps_x(inp)) {
            fr_y = HERO_ANIMID_SWIM; // swim
            fr_x = hero_swim_frameID(o->animation);
        } else {
            fr_y     = HERO_ANIMID_AIR; // "air"
            i32 swim = hero_swim_frameID_idle(o->animation);
            if (swim <= 1) {
                sprite->offs.y += swim == 0 ? 0 : -2;
                fr_x = 2;
            } else {
                sprite->offs.y += swim - 2;
                fr_x = 2 + ((swim - 2) % 3);
            }
        }
        break;
    }
    case HERO_ST_CLIMB: {
        i32 climbdir = inps_y(inp);
        if (!hero_stamina_left(o)) {
            climbdir = +1;
        }
        if (h->impact_ticks) {
            climbdir = 0;
        }

        switch (climbdir) {
        case -1:
            fr_y = 9;
            fr_x = modu_i32(o->animation >> 3, 6);
            break;
        case +0:
            fr_y = 9;
            fr_x = h->impact_ticks ? 6 : 8;
            break;
        case +1:
            fr_y = 9;
            fr_x = 7;
            break;
        }
        break;
    }
    case HERO_ST_STOMP: {
        fr_y = 4;

        if (0 < h->stomp) {
            fr_x = 0 + min_i32(4, h->stomp >> 2);
        }
        if (h->stomp < 0) { // stomp landing
            sprite->offs.y -= 16;
            i32 landingt = HERO_STOMP_LANDING_TICKS + h->stomp;
            fr_x         = 5 +
                   (3 <= landingt) +
                   ((HERO_STOMP_LANDING_TICKS - 3) <= landingt);
        }
        break;
    }
    case HERO_ST_AIR: {
        if (r) {
            v2_i32 rdt = v2_i32_sub(rn->p, o->ropenode->p);
            if (rdt.y < -100 && 254 <= rope_stretch_q8(g, r)) {
                // actively swinging
                f32 ang = atan2f((f32)rdt.y, -(f32)o->facing * (f32)rdt.x);

                fr_y = 12;
                fr_x = -(i32)((ang * 4.f)) - 3;
                fr_x = 3 + clamp_i32(fr_x, 0, 6);
                sprite->offs.y -= 4;

                i32 lsq = v2_i32_len_appr(o->v_q12);
                if (250000 <= lsq) {
                    v2_i32 pcenter = obj_pos_center(o);
                    if (350000 <= lsq || (g->tick & 1)) {
                        particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_SWING, pcenter);
                    }
                }
            } else {
                fr_y = 12;
                sprite->offs.y -= 4;

                if (+500 <= o->v_q12.y) {
                    fr_x = 0;
                } else if (-100 <= o->v_q12.y) {
                    fr_x = 1;
                } else {
                    fr_x = 2;
                }
            }
            break;
        }

        if (h->walljump_tick) {
            i32 wja = abs_i32(h->walljump_tick);
            fr_y    = 9;
            fr_x    = lerp_i32(12, 8, wja, WALLJUMP_TICKS + 1);
            break;
        }

        fr_y      = HERO_ANIMID_AIR; // "air"
        i32 vanim = o->v_q12.y + pow2_i32(h->gliding) * 10;

        if (h->impact_ticks) {
            fr_x = 0;
            sprite->offs.y -= 16;
        } else if (+(100 << 4) <= vanim) {
            fr_x = 6;
        } else if (-(100 << 4) <= vanim) {
            fr_x = 5;
        } else if (-(400 << 4) <= vanim) {
            fr_x = 4;
        } else if (-(800 << 4) <= vanim) {
            fr_x = 3;
        } else {
            fr_x = 2;
        }

        if (h->mode == HERO_MODE_NORMAL && h->b_hold_tick &&
            2 <= fr_x && fr_x <= 6) {
            fr_x += 7;
        }
        break;
    }
    }

    if (h->mode == HERO_MODE_COMBAT && hero_has_upgrade(g, HERO_UPGRADE_COMPANION)) {
        fr_y += 24;
    }
    sprite->trec.x = sprite->trec.w * fr_x;
    sprite->trec.y = sprite->trec.h * fr_y;
}