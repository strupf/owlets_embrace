// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#if 0
#include "game.h"

typedef struct {
    boss_plant_s *bp;
    obj_s        *puppet_comp;
    obj_s        *puppet_hero;
} cs_bossplant_outro_s;

#define CS_BOSSPLANT_OUTRO_TICKS_1 20
#define CS_BOSSPLANT_OUTRO_TICKS_2 200
#define CS_BOSSPLANT_OUTRO_TICKS_3 50
#define CS_BOSSPLANT_OUTRO_TICKS_7 100

void cs_bossplant_outro_update(g_s *g, cs_s *cs, inp_s inp);
void cs_bossplant_outro_on_trigger(g_s *g, cs_s *cs, i32 trigger);
void cs_bossplant_outro_draw(g_s *g, cs_s *cs, v2_i32 cam);
void cs_bossplant_outro_draw_gp(g_s *g, cs_s *cs, v2_i32 cam);
void cs_bossplant_outro_cb_comp(g_s *g, obj_s *o, void *ctx);

void cs_bossplant_outro_enter(g_s *g)
{
    cs_s                 *cs = &g->cs;
    cs_bossplant_outro_s *dm = (cs_bossplant_outro_s *)cs->mem;
    cs_reset(g);
    cs->on_update          = cs_bossplant_outro_update;
    cs->on_draw            = cs_bossplant_outro_draw;
    cs->on_draw_bh_terrain = cs_bossplant_outro_draw_gp;
    dm->bp                 = &g->boss.plant;
    cs->on_trigger         = cs_bossplant_outro_on_trigger;
    g->flags |= GAME_FLAG_BLOCK_UPDATE;
    dm->bp->phase      = BOSS_PLANT_OUTRO0;
    dm->bp->phase_tick = 0;
}

void cs_bossplant_outro_update(g_s *g, cs_s *cs, inp_s inp)
{
    cs_bossplant_outro_s *dm      = (cs_bossplant_outro_s *)cs->mem;
    obj_s                *o_eye   = obj_from_handle(dm->bp->eye);
    obj_s                *o_eyefl = obj_from_handle(dm->bp->eye_fake[0]);
    obj_s                *o_eyefr = obj_from_handle(dm->bp->eye_fake[1]);

    switch (cs->phase) {
    case 0: {
        // if (!cs_wait_and_pause_for_hero_idle(g)) break;

        // mus_play_extv(0, 0, 0, 1000, 0, 0);
        cs->phase++;
        cs->tick = 0;

        // dm->puppet_comp = obj_find_ID(g, OBJID_PUPPET_COMPANION, 0);
        //  v2_i32 hpos     = {dm->bp->x - 115, dm->bp->y + 74};

        // puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, +1);
        //  puppet_move_ext(dm->puppet_comp, hpos, 40, 0, 0, cs_bossplant_outro_cb_comp, cs);
        break;
    }
    case 1: {
        if (CS_BOSSPLANT_OUTRO_TICKS_1 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 2: {
        if (CS_BOSSPLANT_OUTRO_TICKS_2 <= cs->tick) {
            cs->phase++;
            cs->tick   = 0;
            obj_s *owl = obj_get_owl(g);
            owl_s *h   = (owl_s *)owl->heap;
            if (owl->pos.x < dm->bp->x - 120) {
                owl->pos.x = dm->bp->x - 120;
            }
            if (owl->pos.x > dm->bp->x + 120) {
                owl->pos.x = dm->bp->x + 120;
            }
            owl->facing   = dm->bp->x < owl->pos.x ? -1 : +1;
            // h->stamina_ui_fade_out = 0;
            // h->stamina             = hero_stamina_max(ohero);
            owl->blinking = 0;
            owl->health   = 3;
            obj_delete(g, o_eye);
            obj_delete(g, o_eyefl);
            obj_delete(g, o_eyefr);
            objs_cull_to_delete(g);
        }
        break;
    }
    case 3: {
        if (CS_BOSSPLANT_OUTRO_TICKS_3 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
            g->flags &= ~GAME_FLAG_BLOCK_UPDATE;
            g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
            dm->bp->draw_vines = 0;
        }
        break;
    }
    case 4: {
        if (owl_wait_for_idle(g)) {
            cs->phase++;
            cs->tick        = 0;
            obj_s *owl      = obj_get_owl(g);
            dm->puppet_hero = puppet_owl_put(g, owl);
            puppet_set_anim(dm->puppet_hero, PUPPET_OWL_ANIMID_IDLE, 0);
            // mus_play_ext(0, "M_SHOWCASE", 0, 0, 0, 2000, 256);
        }
        break;
    }
    case 5: {
        if (((cs->tick >> 3) & 1)) {
            dm->bp->plant_frame = 2;
        } else {
            dm->bp->plant_frame = 3;
        }

        if (150 <= cs->tick) {
            cs->phase++;
            cs->tick               = 0;
            dm->bp->plant_frame    = 3;
            dm->puppet_comp        = puppet_create(g, OBJID_PUPPET_COMPANION);
            dm->puppet_comp->pos.x = dm->bp->x;
            dm->puppet_comp->pos.y = dm->bp->y;
            obj_s *owl             = obj_get_owl(g);
            v2_i32 hpos            = obj_pos_center(owl);
            v2_i32 pp              = v2_i32_lerp(dm->puppet_comp->pos, hpos, 80, 100);
            i32    facing          = (pp.x < dm->puppet_comp->pos.x ? -1 : +1);
            puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, facing);
            puppet_move_ext(dm->puppet_comp, pp, 60, ease_in_out_quad, 0, 0, 0);
            puppet_set_anim(dm->puppet_hero, 0, -facing);
        }
        break;
    }
    case 6: {
        if (cs->tick == 60) {
            puppet_set_anim(dm->puppet_comp, PUPPET_COMPANION_ANIMID_NOD_ONCE, 0);
        }
        if (150 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
            g->flags |= GAME_FLAG_BLOCK_UPDATE;
        }
        break;
    }
    case 7: {
        if (CS_BOSSPLANT_OUTRO_TICKS_7 <= cs->tick) {
            cs->phase++;
            cs->tick = 0;
        }
        break;
    }
    case 8: {

        break;
    }
    }
#if 0
                b->phase                 = BOSS_PLANT_DEAD;
            b->phase_tick            = 0;
            obj_s *o_attract         = obj_from_handle(b->o_cam_attract);
            o_attract->cam_attract_r = BPLANT_CAM_ATTRACT_R;

            obj_delete(g, o_eye);
            obj_delete(g, o_eyefl);
            obj_delete(g, o_eyefr);
            obj_handle_delete(g, b->exitblocker[0]);
            obj_handle_delete(g, b->exitblocker[1]);
            boss_plant_barrier_poof(g);
            game_cue_area_music(g);
#endif
}

void cs_bossplant_outro_draw(g_s *g, cs_s *cs, v2_i32 cam)
{
    cs_bossplant_outro_s *dm     = (cs_bossplant_outro_s *)cs->mem;
    gfx_ctx_s             ctx    = gfx_ctx_display();
    texrec_s              treye  = asset_texrec(0, 8 * 64, 3 * 64, 64, 64);
    v2_i32                poseye = v2_i32_add(dm->bp->eye_teared, cam);
    poseye.x -= 32;
    poseye.y -= 32;
    i32 eye_q8 = 0;

    switch (cs->phase) {
    case 0: {
        eye_q8 = 256;
        break;
    }
    case 1: {
        eye_q8  = 256;
        ctx.pat = gfx_pattern_interpolate(cs->tick, CS_BOSSPLANT_OUTRO_TICKS_1);
        gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240}, PRIM_MODE_WHITE);
        break;
    }
    case 2: {
        eye_q8 = 256;
        gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240}, PRIM_MODE_WHITE);
        if (100 <= cs->tick) {
            eye_q8 = lerp_i32(256, 0, min_i32(cs->tick - 100, 50), 50);
        }
        break;
    }
    case 3: {
        ctx.pat = gfx_pattern_interpolate(CS_BOSSPLANT_OUTRO_TICKS_3 - cs->tick, CS_BOSSPLANT_OUTRO_TICKS_3);
        gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240}, PRIM_MODE_WHITE);
        break;
    }
    case 7: {
        ctx.pat = gfx_pattern_interpolate(cs->tick, CS_BOSSPLANT_OUTRO_TICKS_7);
        gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240}, PRIM_MODE_WHITE);
        break;
    }
    case 8: {
        gfx_rec_fill(ctx, (rec_i32){0, 0, 400, 240}, PRIM_MODE_WHITE);
        fnt_s    fnt    = asset_fnt(FNTID_LARGE);
        texrec_s trlogo = asset_texrec(TEXID_COVER, 576, 256, 192, 128);
        ctx.pat         = gfx_pattern_interpolate(min_i32(cs->tick, 50), 50);
        fnt_draw_outline_style(ctx, fnt, (v2_i32){200, 150}, "Thanks for playing the demo!", 0, 1);
        gfx_spr(ctx, trlogo, (v2_i32){200 - 192 / 2, 40}, 0, 0);
        break;
    }
    }

    if (eye_q8) {
        gfx_ctx_s ctxeye = ctx;
        ctxeye.pat       = gfx_pattern_interpolate(eye_q8, 256);
        gfx_spr(ctxeye, treye, poseye, 0, 0);
    }
}

void cs_bossplant_outro_draw_gp(g_s *g, cs_s *cs, v2_i32 cam)
{
    cs_bossplant_outro_s *dm  = (cs_bossplant_outro_s *)cs->mem;
    gfx_ctx_s             ctx = gfx_ctx_display();

    switch (cs->phase) {
    case 0: {
        break;
    }
    }
}

void cs_bossplant_outro_on_trigger(g_s *g, cs_s *cs, i32 trigger)
{
    cs_bossplant_outro_s *dm = (cs_bossplant_outro_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    }
}

void cs_bossplant_outro_cb_comp(g_s *g, obj_s *o, void *ctx)
{
    cs_s                 *cs = (cs_s *)ctx;
    cs_bossplant_outro_s *dm = (cs_bossplant_outro_s *)cs->mem;

    switch (cs->phase) {
    default: break;
    }
}
#endif