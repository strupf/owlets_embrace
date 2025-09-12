// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    boss_plant_s *bp;
    obj_s        *puppet_comp;
    obj_s        *puppet_hero;
    v2_i32        eyepos_1;
    bool32        seen;
} cs_bossplant_intro_s;

#define CS_BOSSPLANT_INTRO_FAST  0
#define CS_BOSSPLANT_INTRO_SHORT 1

void cs_bossplant_intro_update(g_s *g, cs_s *cs, inp_s inp);
void cs_bossplant_intro_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_bossplant_intro_draw(g_s *g, cs_s *cs, v2_i32 cam);
void cs_bossplant_intro_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_bossplant_intro_enter(g_s *g)
{
    cs_s                 *cs = &g->cs;
    cs_bossplant_intro_s *dm = (cs_bossplant_intro_s *)cs->mem;
    cs_reset(g);
    cs->on_update  = cs_bossplant_intro_update;
    cs->on_draw    = cs_bossplant_intro_draw;
    dm->bp         = &g->boss.plant;
    cs->on_trigger = cs_bossplant_intro_on_trigger;
    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
    if (save_event_exists(g, 222)) {
        dm->seen = 1;
    } else {
        save_event_register(g, 222);
        game_update_savefile(g);
    }
}

void cs_bossplant_intro_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs_bossplant_intro_s *dm      = (cs_bossplant_intro_s *)cs->mem;
    obj_s                *o_eye   = obj_from_handle(dm->bp->eye);
    obj_s                *o_eyefl = obj_from_handle(dm->bp->eye_fake[0]);
    obj_s                *o_eyefr = obj_from_handle(dm->bp->eye_fake[1]);

    switch (cs->phase) {
    case 0: {
        if (!cs_wait_and_pause_for_owl_idle(g)) break;

        mus_play_extv(0, 0, 0, 2000, 0, 0);
        cs->phase++;
        cs->tick = 0;
        if (dm->seen) {
            // game_darken_bg(g, +128);
        }

        obj_s *owl      = obj_get_owl(g);
        obj_s *ocomp    = obj_get_tagged(g, OBJ_TAG_COMPANION);
        dm->puppet_hero = puppet_owl_put(g, owl);
        puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_IDLE, 0);
        dm->puppet_comp = puppet_companion_put(g, ocomp);

#if 0
        if (g->owl.mode == HERO_MODE_COMBAT) {
            v2_i32 hp = obj_pos_center(ohero);
            hp.y -= 16;
            hp.x -= ohero->facing * 8;
            dm->puppet_comp->pos = hp;
        }
#endif

        v2_i32 hpos = {dm->bp->x - 115, dm->bp->y + 74};

        puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, +1);
        puppet_move_ext(dm->puppet_comp, hpos, 40, 0, 0, cs_bossplant_intro_cb_comp, cs);
        break;
    }
    case 3: {
        if (dm->seen) {
            if (cs->tick == 20) {
                mus_play_extx("M_BOSS", 280271, 418236, 0, 10, 10, 256);

                cs->phase           = 11;
                cs->tick            = 0;
                dm->bp->plant_frame = 2;
                snd_play(SNDID_BPLANT_SHOW, 1.5f, rngr_f32(0.9f, 1.1f));
                boss_plant_eye_show(g, o_eye);
                boss_plant_eye_show(g, o_eyefl);
                boss_plant_eye_show(g, o_eyefr);
                puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_SHOOK, 0);
                puppet_move_ext(dm->puppet_hero, (v2_i32){-20, 0}, 80, 0, 1, 0, 0);
                puppet_move_ext(dm->puppet_comp, (v2_i32){dm->bp->x - 120, dm->bp->y + 100}, 10, 0, 0, 0, 0);
            }
        } else {
            if (cs->tick == 240) {
                mus_play_extv("M_BOSS", 418236, 0, 0, 100, 256);
            }

            switch (cs->tick) {
            case 1:
                // game_darken_bg(g, +24);
                break;
            case 110:
            case 410:
                puppet_move_ext(dm->puppet_comp, (v2_i32){-20, 0}, 30, 0, 1, 0, cs);
                break;
            case 150:
                dm->bp->phase      = BOSS_PLANT_INTRO0;
                dm->bp->tick       = 0;
                dm->bp->phase_tick = 0;
                break;
#if !CS_BOSSPLANT_INTRO_FAST
            case 260:
                puppet_move_ext(dm->puppet_comp, (v2_i32){+20, 0}, 30, 0, 1, 0, cs);
                break;
            case 100:
            case 300:
            case 350:
            case 400:
                snd_play(SNDID_STOMP_LAND, 0.75f, 1.f);
                puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_BUMP_ONCE, 0);
                break;
            case 550:
#else
            case 151:
#endif
                cs->phase           = 11;
                cs->tick            = 0;
                dm->bp->plant_frame = 2;
                snd_play(SNDID_BPLANT_SHOW, 1.5f, rngr_f32(0.9f, 1.1f));
                boss_plant_eye_show(g, o_eye);
                boss_plant_eye_show(g, o_eyefl);
                boss_plant_eye_show(g, o_eyefr);
                puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_SHOOK, 0);
                puppet_move_ext(dm->puppet_hero, (v2_i32){-20, 0}, 80, 0, 1, 0, 0);
                puppet_move_ext(dm->puppet_comp, (v2_i32){dm->bp->x - 120, dm->bp->y + 100}, 10, 0, 0, 0, 0);
                break;
            }
        }
        break;
    }
    case 11: {
        cam_screenshake_xy(&g->cam, 10, 2, 2);

#if CS_BOSSPLANT_INTRO_FAST
        if (22 <= cs->tick) {
#else
        if (60 <= cs->tick) {
#endif
            cs->phase++;
            cs->tick       = 0;
            dm->eyepos_1   = obj_pos_center(o_eyefl);
            o_eyefl->state = BOSS_PLANT_EYE_GRAB_COMP;

            puppet_set_anim(dm->puppet_comp, 0, -1);
            puppet_move_ext(dm->puppet_comp, (v2_i32){dm->bp->x - 140, dm->bp->y + 170}, 15, 0, 0, 0, cs);
        }
        break;
    }
    case 12: {
        if (cs->tick <= 15) {
            v2_i32 pl = v2_i32_lerp(dm->eyepos_1, obj_pos_center(dm->puppet_comp),
                                    cs->tick, 15);
            boss_plant_eye_move_to_centerpx(g, o_eyefl, pl.x, pl.y);

            if (cs->tick == 15) {
                dm->eyepos_1.x = pl.x;
                dm->eyepos_1.y = pl.y;
                obj_delete(g, dm->puppet_comp);
                dm->puppet_comp = 0;
                snd_play(SNDID_HURT, 1.f, 1.f);
                o_eyefl->state = BOSS_PLANT_EYE_GRABBED_COMP;
                puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_QUICKDUCK, 0);
            }
        }
        if (90 < cs->tick) {

            puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_AVENGE, 0);
            v2_i32 pl = v2_i32_lerp(dm->eyepos_1, (v2_i32){dm->bp->x, dm->bp->y},
                                    cs->tick - 90, 40);
            boss_plant_eye_move_to_centerpx(g, o_eyefl, pl.x, pl.y);
            if (90 + 40 == cs->tick) {
                boss_plant_eye_hide(g, o_eyefl);
                cs->phase++;
                cs->tick = 0;
            }
        }

        break;
    }
    case 13: {
#if CS_BOSSPLANT_INTRO_FAST
        if (1 <= cs->tick) {
            boss_plant_eye_hide(g, o_eyefr);
#else
        if (20 == cs->tick) {
            boss_plant_eye_hide(g, o_eyefr);
        }

        if (50 <= cs->tick) {
#endif
            cs->phase++;
            cs->tick = 0;
            snd_play(SNDID_BPLANT_HIDE, 1.0f, rngr_f32(0.9f, 1.1f));

            puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_IDLE, 0);
            dm->bp->plant_frame = 0;
            boss_plant_eye_hide(g, o_eye);
            boss_plant_barrier_poof(g);
        }
        break;
    }
    case 14: {
#if CS_BOSSPLANT_INTRO_FAST
        if (10 <= cs->tick) {
#else
        if (10 <= cs->tick) {
#endif

#if 0
            // leave
            g->block_hero_control = 0;
            g->hero.upgrades &= ~HERO_UPGRADE_COMPANION;
            g->hero.mode       = HERO_MODE_NORMAL;
            dm->bp->phase      = BOSS_PLANT_CLOSED;
            dm->bp->tick       = 0;
            dm->bp->phase_tick = 0;
            dm->bp->draw_vines = 1;
#endif

            obj_s *owl = obj_get_owl(g);
            puppet_owl_replace_and_del(g, owl, dm->puppet_hero);
            cs_reset(g);
        }
        break;
    }
    }
}

void cs_bossplant_intro_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    cs_bossplant_intro_s *dm = (cs_bossplant_intro_s *)cs->mem;
}

void cs_bossplant_intro_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    cs_bossplant_intro_s *dm = (cs_bossplant_intro_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 2: {
        if (trigger == TRIGGER_DIA_END) {
            puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, 0);
            puppet_move_ext(dm->puppet_comp, (v2_i32){30, 0}, 40, 0, 1, 0, cs);
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    }
}

void cs_bossplant_intro_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s                 *cs = (cs_s *)ctx;
    cs_bossplant_intro_s *dm = (cs_bossplant_intro_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    case 1: {
        if (dm->seen) {
            cs->phase = 3;
            cs->tick  = 0;
        } else {
            cs->phase++;
            cs->tick = 0;
            puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_HUH, 0);
            dia_load_from_wad(g, "D_BPLANT_0");
        }
        break;
    }
    }
}