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
    inp_s         inp        = inp_cur();
    rope_s       *r          = o->rope;
    ropenode_s   *rn         = r ? ropenode_neighbour(r, o->ropenode) : 0;
    const bool32  was_idle   = h->is_idle;
    const u32     idle_animp = h->idle_anim;

    o->n_sprites   = 1;
    h->idle_anim   = 0;
    h->is_idle     = 0;
    sprite->trec.t = asset_tex(TEXID_HERO);
    sprite->flip   = o->facing == -1 ? SPR_FLIP_X : 0;
    sprite->offs.x = o->w / 2 - 32;
    sprite->offs.y = o->h - 64 + 16;
    i32 fr_y       = 0;
    i32 fr_x       = 0;
    i32 fr_x_add   = 0;
    i32 state      = hero_get_actual_state(g, o);
    sprite->trec.w = 64;
    sprite->trec.h = 64;

    // h->holds_weapon = 0;

    if ((HERO_HURT_TICKS * 3) / 4 <= h->hurt_ticks) {
        state = 0;
        sprite->offs.y -= 16;
        fr_y = 0;
        fr_x = 12;
    } else if (o->health == 1) {
        // o->n_sprites = 1 <= (g->gameplay_tick % 4);
    } else {
        o->n_sprites = 1;
    }

    if (o->health == 0) {
        switch (state) {
        case HERO_ST_GROUND: {
            sprite->offs.y -= 16;
            fr_y = 0;
            if (h->dead_bounce) {
                fr_x = 13 + (10 <= o->animation);
            } else {
                fr_x = 12;
            }

            break;
        }
        case HERO_ST_AIR: {
            fr_y = 0;
            fr_x = 8 + ((o->animation / 4) & 3);
            break;
        }
        case HERO_ST_WATER: {
            break;
        }
        }

        state = 0;
    }

    if (state && h->holds_spear && !h->stomp) {
        sprite->trec.w = 96;
    }

    if (state && 0 < h->spinattack) {
        state = 0;
        fr_y  = 10;
        if (h->spinattack < 4) {
            fr_x = 11;
        } else if ((HERO_TICKS_SPIN_ATTACK - 4) <= h->spinattack) {
            fr_x = 10;
        } else {
            fr_x = 12 + ((h->spinattack / 3) % 4);
        }
        sprite->offs.y -= 16;
    }

    if ((h->attack_tick || (h->b_hold_tick && h->b_hold_tick < HERO_ITEMSWAP_TICKS_INIT)) &&
        h->holds_spear) {
        state = 0;
        if (o->facing == 1) {
            sprite->offs.x -= 2;
        } else {
            sprite->offs.x -= 30;
        }

        sprite->offs.y -= 16;

        if (h->b_hold_tick) {
            i32 bt = min_i32(h->b_hold_tick, 6);
            fr_x   = lerp_i32(0, 2, bt, 6);
            fr_y   = 24 + h->attack_flipflop;
        } else {
            switch (h->attack_ID) {
            case HERO_ATTACK_GROUND: {
                fr_x = ani_frame(ANIID_HERO_ATTACK, h->attack_tick);
                fr_y = 24 + h->attack_flipflop;
                if (o->facing == -1) {
                    fr_y += 2;
                }
                break;
            }
            case HERO_ATTACK_AIR: {
                if (o->facing == 1) {
                    sprite->offs.x -= 16;
                } else {
                    sprite->offs.x += 16;
                }
                sprite->offs.y -= 4;
                sprite->trec.h = 96;
                fr_y           = 19;
                fr_x           = ani_frame(ANIID_HERO_ATTACK_AIR, h->attack_tick);
                break;
            }
            }
        }
    }
    if (h->squish) {
        sprite->offs.y -= 4;
        state = 0;
        if (h->squish < HERO_SQUISH_TICKS) {
            fr_x = 6 + lerp_i32(0, 10, h->squish, HERO_SQUISH_TICKS);
            fr_y = 14;
        } else {
            o->n_sprites = 0;
        }
    }

    if (h->holds_spear && state && !h->stomp) {
        sprite->offs.x -= 14;
    }

    switch (state) {
    case HERO_ST_GROUND: {
        sprite->offs.y -= 16;

        if (0 < h->impact_ticks && !h->holds_spear) {
            fr_y = 7; // "oof"
#if 1
            fr_x = (h->impact_ticks <= 2 ? 8 : 7);
#else

#endif
            break;
        }

        if (h->grabbing) {
            o->animation++;
            if (abs_i32(h->grabbing) == 1) { // static
                fr_y = 1;
                fr_x = 14 + ((o->animation >> 5) & 1);
            } else { // actively push/pull
                fr_y = 2;
                if (sgn_i32(h->grabbing) == o->facing) {
                    fr_x = 12 + ((o->animation >> 3) & 3);
                } else {
                    fr_x = 8 + ((o->animation >> 3) & 3);
                }
            }
            break;
        }

        if (h->crouched) {
            fr_y = 11;
            if (h->crouched < 0) { // crawling
                o->animation += o->v_q8.x * o->facing;
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
                fr_x         = 14;
                if (260 <= ropestretch) {
                    fr_x += (g->tick >> 4) & 1;
                }

                break;
            }
        }

        if (o->v_q8.x != 0) {
            if (was_idle && h->idle_anim) {
                fr_y = HERO_ANIMID_SLEEP;
                fr_x = 3;
                break;
            }

            // "carrying/walking"
            if (h->holds_spear) {
                fr_y = o->facing == +1 ? 22 : 23;
            } else {
                fr_y = HERO_ANIMID_WALK;
            }

            i32 frameID_prev = (o->animation / 2000) & 7;
            i32 va           = abs_i32(o->v_q8.x);
            if (HERO_VX_SPRINT <= va && fr_y == HERO_ANIMID_WALK && !h->holds_spear) {
                fr_y = HERO_ANIMID_RUN;
                o->animation += (va * 220) >> 8;
            } else {
                o->animation += va;
            }

            if (h->skidding && !h->holds_spear) {
                fr_y = HERO_ANIMID_RUN;
                fr_x = 8 + (h->skidding < 4);
                break;
            }

            fr_x = (o->animation / 2000) & 7;
            if (frameID_prev != fr_x && (fr_x & 3) == 1) {
                snd_play(SNDID_FOOTSTEP_LEAVES, rngr_f32(0.25f, 0.35f), rngr_f32(0.8f, 1.0f));
                v2_i32 posc = obj_pos_bottom_center(o);
                posc.x -= sgn_i32(o->v_q8.x) * 4;
                posc.y -= 2;
                i32 pID = (abs_i32(o->v_q8.x) == HERO_VX_SPRINT ? PARTICLE_EMIT_ID_HERO_WALK_FAST
                                                                : PARTICLE_EMIT_ID_HERO_WALK);
                particle_emit_ID(g, pID, posc);
            }
            break;
        }

        if (h->holds_spear) {
            fr_y = o->facing == +1 ? 22 : 23;
            fr_x = 0;
            break;
        }

        fr_y       = HERO_ANIMID_IDLE; // "carrying/idle"
        h->is_idle = 1;
        h->idle_ticks++;
        if (!was_idle) {
            h->idle_ticks = 0;
        }

        if (idle_animp) {
            h->idle_anim = idle_animp + 1;
        } else if (!h->holds_spear && 100 <= h->idle_ticks && rngr_i32(0, 512) == 0) {
            h->idle_anim = 1;
        }

#define HERO_TICKS_IDLE_SIT 60
        if (h->idle_anim) {
            fr_y = HERO_ANIMID_SLEEP;
            if (HERO_TICKS_IDLE_SIT <= h->idle_anim) {
                fr_x = 12 + (((h->idle_anim - HERO_TICKS_IDLE_SIT) >> 6) & 1);
            } else {
                fr_x = lerp_i32(0, 13, h->idle_anim, HERO_TICKS_IDLE_SIT);
                fr_x = clamp_i32(fr_x, 0, 12);
            }

        } else {
            if (o->v_prev_q8.x != 0) {
                o->animation = 0; // just got idle
            } else {
                o->animation++;
                i32 idlea = o->animation / 15;
                fr_x      = idlea & 3;
                if (((idlea >> 2) % 3) == 1 && fr_x <= 1) {
                    fr_x += 4; // blink
                }
            }
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
            fr_x = 8 + modu_i32(o->animation >> 3, 6);
            break;
        case +0:
            fr_y = 8;
            fr_x = h->impact_ticks ? 6 : 8;
            break;
        case +1:
            fr_y = 8;
            fr_x = 7;
            break;
        }
        break;
    }
    case HERO_ST_STOMP: {
        fr_y = 4;
        if (h->holds_spear) {
            sprite->flip = 0;
            fr_y += 0 < o->facing ? 2 : 1;
        }

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

                i32 lsq = v2_i16_lensq(o->v_q8);
                if (2500000 <= lsq) {
                    v2_i32 pcenter = obj_pos_center(o);
                    if (3500000 <= lsq || (g->tick & 1)) {
                        particle_emit_ID(g, PARTICLE_EMIT_ID_HERO_SWING, pcenter);
                    }
                }
            } else {
                fr_y = 12;
                sprite->offs.y -= 4;

                if (+500 <= o->v_q8.y) {
                    fr_x = 0;
                } else if (-100 <= o->v_q8.y) {
                    fr_x = 1;
                } else {
                    fr_x = 2;
                }
            }
            break;
        }

        if (h->holds_spear) {
            o->animation++;
            sprite->offs.y -= 6;
            if (+100 <= o->v_q8.y) {
                fr_x = 3 - ((o->animation >> 4) & 1);
            } else if (-100 <= o->v_q8.y) {
                fr_x = 2;
            } else if (-400 <= o->v_q8.y) {
                fr_x = 1;
            } else {
                fr_x = 0;
            }

            fr_y = o->facing == +1 ? 20 : 21;
            break;
        }

        if (h->walljump_tick) {
            i32 wj_s    = sgn_i32(h->walljump_tick);
            i32 wj_tick = abs_i32(h->walljump_tick) -
                          (WALLJUMP_MOM_TICKS - WALLJUMP_ANIM_TICKS);
            if (0 <= wj_tick) {
                fr_y         = 8;
                fr_x         = 9 + 1 - (2 * (wj_tick - 1)) / WALLJUMP_ANIM_TICKS;
                sprite->flip = wj_s == 1 ? SPR_FLIP_X : 0;
                break;
            }
        }

        fr_y      = HERO_ANIMID_AIR; // "air"
        i32 vanim = o->v_q8.y + pow2_i32(h->gliding) * 10;

        if (h->impact_ticks) {
            fr_x = 0;
            sprite->offs.y -= 16;
        } else if (+100 <= vanim) {
            fr_x = 6;
        } else if (-100 <= vanim) {
            fr_x = 5;
        } else if (-400 <= vanim) {
            fr_x = 4;
        } else if (-800 <= vanim) {
            fr_x = 3;
        } else {
            fr_x = 2;
        }
        break;
    }
    }

    sprite->trec.x = sprite->trec.w * (fr_x + fr_x_add);
    sprite->trec.y = sprite->trec.h * fr_y;
#ifdef PLTF_DEBUG
    rec_i32 rsprite = {sprite->trec.x, sprite->trec.y, sprite->trec.w, sprite->trec.h};
    rec_i32 rtex    = {0, 0, sprite->trec.t.w, sprite->trec.t.h};
    rec_i32 rinter;
    intersect_rec(rtex, rsprite, &rinter);
    assert(rinter.w == sprite->trec.w && rinter.h == sprite->trec.h);
#endif
}