// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss_plant.h"
#include "game.h"

#define BPLANT_INTRO_TICKS         160
#define BPLANT_DYING_TICKS         150
#define BPLANT_FLASH_TICKS_IN      4
#define BPLANT_FLASH_TICKS_OUT     40
#define BPLANT_FLASH_TICKS         (BPLANT_FLASH_TICKS_IN + BPLANT_FLASH_TICKS_OUT)
#define BPLANT_CAM_ATTRACT_R       300
#define BPLANT_OUTRO0_TICKS        10
#define BPLANT_OUTRO1_TICKS        200
#define BPLANT_OUTRO2_TICKS        50
#define BPLANT_CLOSED_TEARED_TICKS 200

enum {
    BOSS_PLANT_OPEN_UP_SHOW,
    BOSS_PLANT_OPEN_UP_ATK,
};

enum {
    BOSS_PLANT_TENTACLE_PT_NONE,
    //
    BOSS_PLANT_TENTACLE_PT_BURST,
    BOSS_PLANT_TENTACLE_PT_BURST_TWICE,
    BOSS_PLANT_TENTACLE_PT_LE_RI,
    BOSS_PLANT_TENTACLE_PT_RI_LE,
    //
    NUM_BOSS_PLANT_TENTACLE_PT = 3
};

void boss_plant_update_seg(bplant_seg_s *segs, i32 num, i32 l)
{
    for (i32 k = 1; k < num - 1; k++) {
        bplant_seg_s *s0  = &segs[k - 1];
        v2_i32        tmp = s0->p_q8;
        s0->p_q8.x += (s0->p_q8.x - s0->pp_q8.x);
        s0->p_q8.y += (s0->p_q8.y - s0->pp_q8.y);
        s0->pp_q8 = tmp;
    }

    for (i32 i = 0; i < 5; i++) {
        for (i32 k = 1; k < num; k++) {
            bplant_seg_s *s0  = &segs[k - 1];
            bplant_seg_s *s1  = &segs[k];
            v2_i32        p0  = s0->p_q8;
            v2_i32        p1  = s1->p_q8;
            v2_i32        dt  = v2_i32_sub(p1, p0);
            i32           len = v2_i32_len_appr(dt);
            if (len <= l) continue;

            i32    new_l = l + ((len - l) >> 1);
            v2_i32 vadd  = v2_i32_setlenl(dt, len, new_l);

            if (1 < k) {
                s0->p_q8 = v2_i32_sub(p1, vadd);
            }
            if (k < num - 1) {
                s1->p_q8 = v2_i32_add(p0, vadd);
            }
        }
    }
}

void   boss_plant_barrier_poof(g_s *g);
void   boss_plant_tentacle_pt_update(g_s *g);
void   boss_plant_tentacle_try_emerge(g_s *g, i32 tile_x);
obj_s *boss_plant_tentacle_emerge(g_s *g, i32 x, i32 y);
void   boss_plant_tentacle_on_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
obj_s *boss_plant_eye_create(g_s *g, i32 ID);
void   boss_plant_eye_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
void   boss_plant_eye_hide(g_s *g, obj_s *o);
void   boss_plant_eye_show(g_s *g, obj_s *o);
bool32 boss_plant_eye_is_hooked(obj_s *o);
bool32 boss_plant_eye_is_busy(obj_s *o);
bool32 boss_plant_eye_try_attack(g_s *g, obj_s *o, i32 slash_y_slot, i32 slash_sig, b32 x_slash);

void boss_plant_tentacle_pt_update(g_s *g)
{
    boss_plant_s *b = &g->boss.plant;

    if (!b->tentacle_pt) {
        b->tentacle_pt_ticks_ended++;
        return;
    }

    v2_i32 panchor             = {b->x, b->y};
    b->tentacle_pt_ticks_ended = 0;
    b->tick_tentacle_pt++;

    switch (b->tentacle_pt) {
    case BOSS_PLANT_TENTACLE_PT_BURST:
    case BOSS_PLANT_TENTACLE_PT_BURST_TWICE: {
        switch (b->tentacle_pt_n) {
        case 0:
            for (i32 n = -10; n <= +10; n += 4) {
                boss_plant_tentacle_try_emerge(g, n);
            }
            if (b->tentacle_pt == BOSS_PLANT_TENTACLE_PT_BURST) {
                b->tentacle_pt = 0;
            } else {
                b->tentacle_pt_n++;
                b->tick_tentacle_pt = 0;
            }
            break;
        case 1:
            if (b->tick_tentacle_pt < 120) break;

            for (i32 n = -8; n <= +8; n += 4) {
                boss_plant_tentacle_try_emerge(g, n);
            }
            b->tentacle_pt = 0;
            break;
        }
        break;
    }
    case BOSS_PLANT_TENTACLE_PT_RI_LE:
    case BOSS_PLANT_TENTACLE_PT_LE_RI: {
        i32 d = (b->tentacle_pt == BOSS_PLANT_TENTACLE_PT_LE_RI ? +1 : -1);

        if (0 <= b->tick_tentacle_pt) {
            b->tick_tentacle_pt -= 20;
            boss_plant_tentacle_try_emerge(g, -d * 11 + 2 * d * b->tentacle_pt_n);
            b->tentacle_pt_n++;

            if (b->tentacle_pt_n == 12) {
                b->tentacle_pt = 0;
                break;
            }
        }
        break;
    }
    }
}

bool32 boss_plant_start_tentacle_pt(g_s *g, i32 i_pt)
{
    boss_plant_s *b = &g->boss.plant;
    if (b->tentacle_pt) return 0;
    b->tentacle_pt             = i_pt;
    b->tick_tentacle_pt        = 1;
    b->tentacle_pt_n           = 0;
    b->tentacle_pt_ticks_ended = 0;
    return 1;
}

void boss_plant_barrier_poof(g_s *g)
{
    boss_plant_s *b       = &g->boss.plant;
    v2_i32        panchor = {b->x, b->y};
    for (i32 n = 0; n < 3; n++) {
        v2_i32 p1           = {panchor.x - 200, panchor.y + 96 + n * 32};
        v2_i32 p2           = {panchor.x + 200, panchor.y + 96 + n * 32};
        obj_s *o1           = objanim_create(g, p1, OBJANIMID_ENEMY_EXPLODE);
        obj_s *o2           = objanim_create(g, p2, OBJANIMID_ENEMY_EXPLODE);
        o1->render_priority = 255;
        o2->render_priority = 255;
    }
}

// BOSS
void boss_plant_load(g_s *g, map_obj_s *mo)
{
    boss_plant_s *b = &g->boss.plant;
    mclr(b, sizeof(boss_plant_s));

    b->x                     = mo->x + mo->w / 2;
    b->y                     = mo->y;
    obj_s *o_attract         = obj_create(g);
    o_attract->pos.x         = b->x;
    o_attract->pos.y         = b->y + 16;
    o_attract->cam_attract_r = BPLANT_CAM_ATTRACT_R;
    o_attract->ID            = OBJID_CAMATTRACTOR;
    b->o_cam_attract         = obj_handle_from_obj(o_attract);

    if (save_event_exists(g, SAVE_EV_BOSS_PLANT)) {
        b->phase = BOSS_PLANT_DEAD;
    } else {
        b->phase = BOSS_PLANT_SLEEP;
    }
}

void boss_plant_wake_up(g_s *g)
{
    boss_plant_s *b          = &g->boss.plant;
    obj_s        *o_attract  = obj_from_obj_handle(b->o_cam_attract);
    o_attract->cam_attract_r = 0;

    b->phase               = BOSS_PLANT_INTRO0;
    b->phase_tick          = 0;
    b->tick                = 0;
    g->block_hero_control  = 1;
    g->dialog.script_input = 1;
    obj_s *o_eye           = boss_plant_eye_create(g, OBJID_BOSS_PLANT_EYE);
    b->eye                 = obj_handle_from_obj(o_eye);
    b->eye_fake[0]         = obj_handle_from_obj(boss_plant_eye_create(g, OBJID_BOSS_PLANT_EYE_FAKE_L));
    b->eye_fake[1]         = obj_handle_from_obj(boss_plant_eye_create(g, OBJID_BOSS_PLANT_EYE_FAKE_R));

    v2_i32 panchor = {b->x, b->y};

    for (i32 n = 0; n < 2; n++) {
        {
            obj_s *o          = obj_create(g);
            o->ID             = OBJID_EXIT_BLOCKER;
            o->flags          = OBJ_FLAG_SOLID;
            o->w              = 16;
            o->h              = 240;
            o->pos.y          = panchor.y;
            o->pos.x          = panchor.x + (n == 0 ? -200 : +200) - 8;
            b->exitblocker[n] = obj_handle_from_obj(o);
        }
        {
            obj_s *o         = obj_create(g);
            o->ID            = OBJID_EXIT_BLOCKER;
            o->flags         = OBJ_FLAG_HURT_ON_TOUCH | OBJ_FLAG_DESTROY_HOOK;
            o->w             = 32;
            o->h             = 240;
            o->pos.y         = panchor.y;
            o->pos.x         = panchor.x + (n == 0 ? -200 : +200) - 16;
            b->exithurter[n] = obj_handle_from_obj(o);
        }
    }
    game_darken_bg(g, +24);
    mus_play_extv("M_BOSS1", 0, 0, 250, 100, 256);
}

void boss_plant_update(g_s *g)
{
    boss_plant_s *b       = &g->boss.plant;
    v2_i32        panchor = {b->x, b->y};
    obj_s        *o_eye   = obj_from_obj_handle(b->eye);
    obj_s        *o_eyefl = obj_from_obj_handle(b->eye_fake[0]);
    obj_s        *o_eyefr = obj_from_obj_handle(b->eye_fake[1]);
    i32           n_eyes  = (o_eyefl != 0) + (o_eyefr != 0);
    b->phase_tick++;
    b->tick++;

    if (b->phase != BOSS_PLANT_DEAD &&
        b->phase != BOSS_PLANT_SLEEP) {
        g->cam.trg.x        = b->x;
        g->cam.trg.y        = b->y + 120;
        g->cam.has_trg      = 1;
        g->cam.trg_fade_spd = 50;
    } else {
        g->cam.has_trg = 0;
    }

    switch (b->phase) {
    case BOSS_PLANT_INTRO0: {
        if (300 <= b->phase_tick) {
            b->phase++;
            b->phase_tick = 0;
            boss_plant_eye_show(g, o_eye);
            boss_plant_eye_show(g, o_eyefl);
            boss_plant_eye_show(g, o_eyefr);
        }
        break;
    }
    case BOSS_PLANT_INTRO1: {
        cam_screenshake_xy(&g->cam, 10, 2, 2);
        if (170 <= b->phase_tick) {
            b->phase++;
            b->phase_tick = 0;
            boss_plant_eye_hide(g, o_eye);
            boss_plant_eye_hide(g, o_eyefl);
            boss_plant_eye_hide(g, o_eyefr);
        }
        break;
    }
    case BOSS_PLANT_INTRO2: {
        if (110 <= b->phase_tick) {
            b->phase++;
            b->phase_tick = 0;
            boss_plant_barrier_poof(g);
        }
        break;
    }
    case BOSS_PLANT_INTRO3: {
        if (30 <= b->phase_tick) {
            b->phase              = BOSS_PLANT_CLOSED;
            b->phase_tick         = 50;
            b->tick               = 0;
            g->block_hero_control = 0;
        }
        break;
    }
    case BOSS_PLANT_CLOSED: {
        if (100 <= b->phase_tick && 50 <= b->tentacle_pt_ticks_ended) {
            b->phase_tick      = 0;
            b->phase           = BOSS_PLANT_PREPARE_OPEN;
            b->open_up_action  = rngsr_i32(&b->seed, 0, 1);
            b->eye_hooked_tick = 0;
        }
        break;
    }
    case BOSS_PLANT_PREPARE_OPEN: {
        switch (b->open_up_action) {
        case BOSS_PLANT_OPEN_UP_SHOW: {
            if (50 <= b->phase_tick) {
                b->phase_tick = 0;
                b->phase      = BOSS_PLANT_OPENED;
                if (!o_eyefr && !o_eyefl) {
                    boss_plant_eye_show(g, o_eye);
                }
                boss_plant_eye_show(g, o_eyefl);
                boss_plant_eye_show(g, o_eyefr);
            }
            break;
        }
        case BOSS_PLANT_OPEN_UP_ATK: {
            if (70 <= b->phase_tick) {
                i32 i_pt = 1 + rngsr_i32(&b->seed, 0, NUM_BOSS_PLANT_TENTACLE_PT);
                boss_plant_start_tentacle_pt(g, i_pt);
                b->phase_tick = 0;
                b->phase      = BOSS_PLANT_CLOSED;
            }
            break;
        }
        }

        break;
    }
    case BOSS_PLANT_OPENED: {
        bool32 is_busy = boss_plant_eye_is_busy(o_eye) |
                         boss_plant_eye_is_busy(o_eyefl) |
                         boss_plant_eye_is_busy(o_eyefr);

        bool32 is_hooked = boss_plant_eye_is_hooked(o_eye) |
                           boss_plant_eye_is_hooked(o_eyefl) |
                           boss_plant_eye_is_hooked(o_eyefr);

        if (is_hooked) {
            b->eye_hooked_tick++;

            switch (n_eyes) {
            case 0: {
                if (25 <= b->tentacle_pt_ticks_ended) {
                    boss_plant_start_tentacle_pt(g, rngsr_i32(&b->seed, 1, 3));
                }
                break;
            }
            case 1:
            case 2: {
                if (((20 + b->eye_hooked_tick) % 100) == 0) {
                    boss_plant_tentacle_try_emerge(g, rngsr_sym_i32(&b->seed, 8));
                }
                break;
            }
            }
        } else {
            b->eye_hooked_tick = 0;
        }

        if (75 == b->phase_tick && !is_hooked) {
            i32 dir = rngsr_i32(&b->seed, 0, 1) * 2 - 1;

            switch (n_eyes) {
            case 0: {

                break;
            }
            case 1: {
                obj_s *o_eye_single = o_eyefl ? o_eyefl : o_eyefr;
                i32    slash_y      = rngsr_i32(&b->seed, 3, 6);

                boss_plant_eye_try_attack(g, o_eye_single, slash_y, dir, 1);
                break;
            }
            case 2: {
                static u8 ay[4][2] = {{7, 2},
                                      {6, 4}};
                i32       ypt      = rngsr_i32(&b->seed, 0, 1);

                boss_plant_eye_try_attack(g, o_eyefl, ay[ypt][0], dir, 0);
                boss_plant_eye_try_attack(g, o_eyefr, ay[ypt][1], dir, 0);
                break;
            }
            }
        }
        if (300 <= b->phase_tick) {
            if (is_busy) {

            } else {
                b->phase_tick = 0;
                b->phase      = BOSS_PLANT_CLOSED;
                boss_plant_eye_hide(g, o_eye);
                boss_plant_eye_hide(g, o_eyefl);
                boss_plant_eye_hide(g, o_eyefr);
            }
        }
        break;
    }
    case BOSS_PLANT_OPENED_TEARED: {
        if (60 <= b->phase_tick) {
            b->phase_tick = 0;
            b->phase      = BOSS_PLANT_CLOSED_TEARED;
        }
        break;
    }
    case BOSS_PLANT_CLOSED_TEARED: {
        if (BPLANT_CLOSED_TEARED_TICKS <= b->phase_tick) {
            b->phase_tick = 0;
            b->phase      = BOSS_PLANT_CLOSED;
        }
        break;
    }
    case BOSS_PLANT_OUTRO0: {
        if (BPLANT_OUTRO0_TICKS <= b->phase_tick) {
            b->phase_tick = 0;
            b->phase++;
        }
        break;
    }
    case BOSS_PLANT_OUTRO1: {
        if ((b->phase_tick & 3) == 0) {
            b->eye_teared.x += b->sx_teared;
            b->eye_teared.y++;
            b->eye_tearx += b->sx_teared;
            b->eye_teary++;
        }
        if (BPLANT_OUTRO1_TICKS <= b->phase_tick) {
            b->phase_tick = 0;
            b->phase++;
        }
        break;
    }
    case BOSS_PLANT_OUTRO2: {
        if (BPLANT_OUTRO2_TICKS <= b->phase_tick) {
            b->phase_tick = 0;
            b->phase++;
            game_darken_bg(g, -8);
        }
        break;
    }
    case BOSS_PLANT_OUTRO3: {
        if (BPLANT_DYING_TICKS <= b->phase_tick) {
            b->phase                 = BOSS_PLANT_DEAD;
            b->phase_tick            = 0;
            obj_s *o_attract         = obj_from_obj_handle(b->o_cam_attract);
            o_attract->cam_attract_r = BPLANT_CAM_ATTRACT_R;

            obj_delete(g, o_eye);
            obj_delete(g, o_eyefl);
            obj_delete(g, o_eyefr);
            obj_handle_delete(g, b->exitblocker[0]);
            obj_handle_delete(g, b->exitblocker[1]);
            boss_plant_barrier_poof(g);
            game_cue_area_music(g);
        }
        break;
    }
    }

    boss_plant_tentacle_pt_update(g);
}

void boss_plant_tentacle_try_emerge(g_s *g, i32 tile_x)
{
    boss_plant_s *b = &g->boss.plant;
    for (i32 n = 0; n < ARRLEN(b->tentacles); n++) {
        if (obj_handle_valid(b->tentacles[n])) continue;

        obj_s *ot       = boss_plant_tentacle_emerge(g, tile_x * 16, 240 - 16);
        b->tentacles[n] = obj_handle_from_obj(ot);
        break;
    }
}

void boss_plant_draw(g_s *g, v2_i32 cam)
{
    boss_plant_s *b       = &g->boss.plant;
    gfx_ctx_s     ctx     = gfx_ctx_display();
    obj_s        *o_eye   = obj_from_obj_handle(b->eye);
    obj_s        *o_eyefl = obj_from_obj_handle(b->eye_fake[0]);
    obj_s        *o_eyefr = obj_from_obj_handle(b->eye_fake[1]);
    tex_s         ttmp    = asset_tex(TEXID_DISPLAY_TMP_MASK);
    tex_clr(ttmp, GFX_COL_CLEAR);

    gfx_ctx_s ctxt = gfx_ctx_default(ttmp);
    i32       fr1  = 0;
    i32       fr2  = frame_from_ticks_pingpong(g->tick_animation >> 3, 4);

    v2_i32 fr_plant = {0, 0};
    v2_i32 fr_core  = {0, 0};
    v2_i32 fr_vine  = {0, 0};

    texrec_s trcol   = asset_texrec(TEXID_BOSSPLANT, fr1 * 64, 6 * 64, 64, 64);
    texrec_s trplant = asset_texrec(TEXID_BOSSPLANT, 0, 0, 288, 96);
    texrec_s trcore  = asset_texrec(TEXID_BOSSPLANT, 704, 0, 128, 128);
    texrec_s trvine  = asset_texrec(TEXID_BOSSPLANT, (1 + fr2) * 128, 7 * 64, 128, 128);

    bool32 draw_vines = 1;
    v2_i32 panchor    = {b->x + cam.x, b->y + cam.y};

    i32 plant_fr = 0;

    switch (b->phase) {
    default: break;
    case BOSS_PLANT_SLEEP:
    case BOSS_PLANT_DEAD:
    case BOSS_PLANT_INTRO0:
    case BOSS_PLANT_INTRO1:
    case BOSS_PLANT_INTRO2: {
        draw_vines = 0;
        break;
    }
    case BOSS_PLANT_INTRO3: {
        if (b->phase_tick < 12) {
            draw_vines = 0;
        }
        break;
    }
    }

    switch (b->phase) {
    default: break;
    case BOSS_PLANT_INTRO1:
    case BOSS_PLANT_CLOSED:
    case BOSS_PLANT_OPENED_TEARED:
    case BOSS_PLANT_OPENED: {
        boss_plant_eye_draw(g, o_eye, cam, ctxt);
        boss_plant_eye_draw(g, o_eyefl, cam, ctxt);
        boss_plant_eye_draw(g, o_eyefr, cam, ctxt);
        break;
    }
    }

    for (i32 y = 0; draw_vines && y < 8; y++) {
        v2_i32 pvinel = {panchor.x - 198 - (trcol.w >> 1), panchor.y + y * 64};
        v2_i32 pviner = {panchor.x + 198 - (trcol.w >> 1), panchor.y + y * 64};
        gfx_spr(ctxt, trcol, pvinel, 0, 0);
        gfx_spr(ctxt, trcol, pviner, 0, 0);
    }

    switch (b->phase) {
    default: break;
    case BOSS_PLANT_SLEEP: {
        if ((b->phase_tick % 100) < 10) {
            plant_fr = 1;
        }
        break;
    }
    case BOSS_PLANT_CLOSED:
        if (b->phase_tick < 8) {
            plant_fr = 1;
        }
        break;
    case BOSS_PLANT_DEAD:
        plant_fr = 3;
        break;
    case BOSS_PLANT_INTRO1:
    case BOSS_PLANT_OPENED_TEARED:
    case BOSS_PLANT_OPENED: {
        plant_fr = 2;
        break;
    case BOSS_PLANT_CLOSED_TEARED:
    case BOSS_PLANT_PREPARE_OPEN:
        plant_fr = ((b->phase_tick >> 3) & 1 ? 1 : 0);
        break;
    case BOSS_PLANT_OUTRO0:
    case BOSS_PLANT_OUTRO1:
    case BOSS_PLANT_OUTRO2: {
        plant_fr = 2;
        break;
    }
    case BOSS_PLANT_OUTRO3: {
        if ((b->phase_tick >> 3) & 1) {
            plant_fr = 2;
        } else {
            plant_fr = 3;
        }
        break;
    }
    }
    }

    for (i32 n = 0; n < ARRLEN(b->tentacles); n++) {
        obj_s *otentacle = obj_from_obj_handle(b->tentacles[n]);
        if (!otentacle) continue;

        boss_plant_tentacle_on_draw(g, otentacle, cam, ctxt);
    }

    trplant.y = plant_fr * 96;

    v2_i32 posplant = {panchor.x - (trplant.w >> 1), panchor.y + 16};
    gfx_spr(ctxt, trplant, posplant, 0, 0);

    tex_merge_to_opaque_outlined_white(ctx.dst, ttmp);
}

void boss_plant_on_eye_tear_off(g_s *g, obj_s *o)
{
    boss_plant_s *b       = &g->boss.plant;
    obj_s        *o_eye   = obj_from_obj_handle(b->eye);
    obj_s        *o_eyefl = obj_from_obj_handle(b->eye_fake[0]);
    obj_s        *o_eyefr = obj_from_obj_handle(b->eye_fake[1]);

    switch (o->ID) {
    case OBJID_BOSS_PLANT_EYE: {
        b->phase      = BOSS_PLANT_OUTRO0;
        b->phase_tick = 0;
        b->eye_teared = obj_pos_center(o_eye);
        save_event_register(g, SAVE_EV_BOSS_PLANT);
        mus_play_extv(0, 0, 0, 200, 0, 0);
        for (i32 n = 0; n < ARRLEN(b->tentacles); n++) {
            obj_handle_delete(g, b->tentacles[n]);
        }
        obj_handle_delete(g, b->exithurter[0]);
        obj_handle_delete(g, b->exithurter[1]);
        break;
    }
    case OBJID_BOSS_PLANT_EYE_FAKE_L:
    case OBJID_BOSS_PLANT_EYE_FAKE_R: {
        b->phase      = BOSS_PLANT_OPENED_TEARED;
        b->phase_tick = 0;
        healthdrop_spawn(g, obj_pos_center(o));
        boss_plant_eye_hide(g, o_eyefl);
        boss_plant_eye_hide(g, o_eyefr);
        break;
    }
    }
}

void boss_plant_draw_post_outro(g_s *g, v2_i32 cam)
{
    boss_plant_s *b = &g->boss.plant;

    gfx_ctx_s ctx = gfx_ctx_display();
    texrec_s  tr0 = asset_texrec(TEXID_BOSSPLANT, 4 * 96, 0 * 96, 96, 96);
    texrec_s  tr1 = asset_texrec(TEXID_BOSSPLANT, 4 * 96, 1 * 96, 96, 96);
    texrec_s  tr2 = asset_texrec(TEXID_BOSSPLANT, 6 * 96, 2 * 96, 96, 96);

    v2_i32 peye = v2_i32_add(b->eye_teared, cam);
    peye.x -= 48;
    peye.y -= 48 + 16;

    v2_i32 p0 = peye;
    v2_i32 p1 = peye;
    v2_i32 p2 = peye;

    p1.x += b->eye_tearx;
    p0.y += b->eye_teary;

    switch (b->phase) {
    default: break;
    case BOSS_PLANT_OUTRO0:
    case BOSS_PLANT_OUTRO1: {
        if (b->phase == BOSS_PLANT_OUTRO1 &&
            BPLANT_OUTRO1_TICKS / 2 <= b->phase_tick &&
            ((b->phase_tick >> 2) & 1)) {
            break;
        }

        gfx_spr(ctx, tr1, p1, 0, 0);
        gfx_spr(ctx, tr2, p2, 0, 0);
        gfx_spr(ctx, tr0, p0, 0, 0);
        break;
    }
    }
}

void boss_plant_draw_post(g_s *g, v2_i32 cam)
{
    boss_plant_s *b = &g->boss.plant;

    gfx_ctx_s ctx  = gfx_ctx_display();
    gfx_ctx_s ctxf = ctx;
    rec_i32   r    = {0, 0, 400, 240};

    switch (b->phase) {
    default: break;
    case BOSS_PLANT_OUTRO0: {
        ctxf.pat = gfx_pattern_interpolate(b->phase_tick, BPLANT_OUTRO0_TICKS);
        gfx_rec_fill_opaque(ctxf, r, PRIM_MODE_WHITE);
        break;
    }
    case BOSS_PLANT_OUTRO1: {
        gfx_rec_fill_opaque(ctx, r, PRIM_MODE_WHITE);
        break;
    }
    case BOSS_PLANT_OUTRO2: {
        ctxf.pat = gfx_pattern_interpolate(BPLANT_OUTRO2_TICKS - b->phase_tick, BPLANT_OUTRO2_TICKS);
        gfx_rec_fill_opaque(ctxf, r, PRIM_MODE_WHITE);
        break;
    }
    case BOSS_PLANT_OPENED_TEARED: {
        if (BPLANT_FLASH_TICKS <= b->phase_tick) break;

        i32 pt = 0;

        if (BPLANT_FLASH_TICKS_IN <= b->phase_tick) {
            i32 t = b->phase_tick - BPLANT_FLASH_TICKS_IN;
            pt    = lerp_i32(GFX_PATTERN_MAX, 0,
                             t, BPLANT_FLASH_TICKS_OUT);
        } else {
            i32 t = b->phase_tick;
            pt    = lerp_i32(0, GFX_PATTERN_MAX,
                             t, BPLANT_FLASH_TICKS_IN);
        }
        ctx.pat = gfx_pattern_bayer_4x4(pt);
        gfx_rec_fill_opaque(ctx, r, PRIM_MODE_WHITE);
        break;
    }
    }

    switch (b->phase) {
    default: break;
    case BOSS_PLANT_OUTRO0:
    case BOSS_PLANT_OUTRO1:
    case BOSS_PLANT_OUTRO2: {
        boss_plant_draw_post_outro(g, cam);
        break;
    }
    }
}

void boss_plant_hide_other_eye(g_s *g, obj_s *o)
{
    boss_plant_s *b       = &g->boss.plant;
    obj_s        *o_eyefl = obj_from_obj_handle(b->eye_fake[0]);
    obj_s        *o_eyefr = obj_from_obj_handle(b->eye_fake[1]);

    if (o == o_eyefl) {
        boss_plant_eye_hide(g, o_eyefr);
    } else {
        boss_plant_eye_hide(g, o_eyefl);
    }
}