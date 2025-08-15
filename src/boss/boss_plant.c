// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss/boss_plant.h"
#include "app.h"
#include "game.h"

#define BPLANT_FAST_INTRO          1
//
#define BPLANT_INTRO_TICKS         160
#define BPLANT_DYING_TICKS         150
#define BPLANT_FLASH_TICKS_IN      4
#define BPLANT_FLASH_TICKS_OUT     40
#define BPLANT_FLASH_TICKS         (BPLANT_FLASH_TICKS_IN + BPLANT_FLASH_TICKS_OUT)
#define BPLANT_CAM_ATTRACT_R       300
#define BPLANT_CLOSED_TEARED_TICKS 200

enum {
    BOSS_PLANT_OPEN_UP_SHOW,
    BOSS_PLANT_OPEN_UP_ATK,
};

enum {
    BOSS_PLANT_TENTACLE_PT_NONE,
    //
    BOSS_PLANT_TENTACLE_PT_BURST_0,
    BOSS_PLANT_TENTACLE_PT_BURST_1,
    BOSS_PLANT_TENTACLE_PT_LE_RI,
    BOSS_PLANT_TENTACLE_PT_RI_LE,
    //
    NUM_BOSS_PLANT_TENTACLE_PT = 4
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
void   boss_plant_tentacle_try_emerge_ext(g_s *g, i32 tile_x, i32 t_emerge, i32 t_active);
obj_s *boss_plant_tentacle_emerge(g_s *g, i32 x, i32 y, i32 t_emerge, i32 t_active);
void   boss_plant_tentacle_on_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
obj_s *boss_plant_eye_create(g_s *g, i32 ID);
void   boss_plant_eye_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
void   boss_plant_eye_hide(g_s *g, obj_s *o);
void   boss_plant_eye_show(g_s *g, obj_s *o);
bool32 boss_plant_eye_is_hooked(obj_s *o);
bool32 boss_plant_eye_is_busy(obj_s *o);
bool32 boss_plant_eye_try_attack(g_s *g, obj_s *o, i32 slash_y_slot, i32 slash_sig, b32 x_slash);

bool32 boss_plant_any_tentacle_still_emerging(g_s *g)
{
    boss_plant_s *b = &g->boss.plant;
    for (i32 n = 0; n < ARRLEN(b->tentacles); n++) {
        obj_s *o = obj_from_obj_handle(b->tentacles[n]);
        if (o && o->state == 0) return 1;
    }
    return 0;
}

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
    case BOSS_PLANT_TENTACLE_PT_BURST_0: {
        boss_plant_tentacle_try_emerge_ext(g, -7, 50, 30);
        boss_plant_tentacle_try_emerge_ext(g, -5, 50, 30);
        boss_plant_tentacle_try_emerge_ext(g, -2, 50, 30);
        boss_plant_tentacle_try_emerge_ext(g, +2, 50, 30);
        boss_plant_tentacle_try_emerge_ext(g, +5, 50, 30);
        boss_plant_tentacle_try_emerge_ext(g, +7, 50, 30);
        b->tentacle_pt = 0;
        break;
    }
    case BOSS_PLANT_TENTACLE_PT_BURST_1: {
        for (i32 n = -6; n <= +6; n += 3) {
            boss_plant_tentacle_try_emerge_ext(g, n, 50, 30);
        }
        b->tentacle_pt = 0;
        break;
    }
    case BOSS_PLANT_TENTACLE_PT_RI_LE:
    case BOSS_PLANT_TENTACLE_PT_LE_RI: {
        i32 d = (b->tentacle_pt == BOSS_PLANT_TENTACLE_PT_LE_RI ? +1 : -1);

        if (0 <= b->tick_tentacle_pt) {
            b->tick_tentacle_pt -= 15;
            i32 posx = -d * 8 + 2 * d * b->tentacle_pt_n;
            if (!(b->tentacle_pt_spare_x - 1 <= posx &&
                  posx <= b->tentacle_pt_spare_x + 1)) {
                boss_plant_tentacle_try_emerge_ext(g, posx, 35, 35);
            }
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

    if (i_pt == BOSS_PLANT_TENTACLE_PT_RI_LE ||
        i_pt == BOSS_PLANT_TENTACLE_PT_LE_RI) {
        b->tentacle_pt_spare_x = rngr_i32(-4, +4) << 1;
    }
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
    snd_play(SNDID_STOMP_LAND, 1.2f, 1.f);
    snd_play(SNDID_LANDING, 1.2f, 1.f);
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

    tex_from_wad_ID(TEXID_BOSSPLANT, "T_BOSSPLANT", game_allocator(g));

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
    obj_s *o_eye             = boss_plant_eye_create(g, OBJID_BOSS_PLANT_EYE);
    b->eye                   = obj_handle_from_obj(o_eye);
    b->eye_fake[0]           = obj_handle_from_obj(boss_plant_eye_create(g, OBJID_BOSS_PLANT_EYE_FAKE_L));
    b->eye_fake[1]           = obj_handle_from_obj(boss_plant_eye_create(g, OBJID_BOSS_PLANT_EYE_FAKE_R));

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
        if (1) {
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
    g->cam.trg.x        = b->x;
    g->cam.trg.y        = b->y + 120;
    g->cam.has_trg      = 1;
    g->cam.trg_fade_spd = 50;
    cs_bossplant_intro_enter(g);
}

void boss_plant_update(g_s *g)
{
    boss_plant_s *b           = &g->boss.plant;
    v2_i32        panchor     = {b->x, b->y};
    obj_s        *owl         = obj_get_owl(g);
    v2_i32        powl        = obj_pos_center(owl);
    obj_s        *o_eye       = obj_from_obj_handle(b->eye);
    obj_s        *o_eyefl     = obj_from_obj_handle(b->eye_fake[0]);
    obj_s        *o_eyefr     = obj_from_obj_handle(b->eye_fake[1]);
    i32           n_eyes_fake = (o_eyefl != 0) + (o_eyefr != 0);
    i32           n_eyes_fake_ripped =
        boss_plant_eye_is_teared(o_eyefl) +
        boss_plant_eye_is_teared(o_eyefr);
    bool32 any_eye_attacking = boss_plant_eye_is_busy(o_eyefl) |
                               boss_plant_eye_is_busy(o_eyefr);
    bool32 any_eye_hooked = boss_plant_eye_is_hooked(o_eye) |
                            boss_plant_eye_is_hooked(o_eyefl) |
                            boss_plant_eye_is_hooked(o_eyefr);
    b->phase_tick++;
    b->tick++;
    if (n_eyes_fake_ripped || b->n_ripped == 3) {
        b->eye_ripped_tick++;
    } else {
        b->eye_ripped_tick  = 0;
        b->ripped_intensify = 0;
        b->ripped_timer     = 0;
    }

    if (b->just_teared_flash_tick) {
        b->just_teared_flash_tick++;
        if (BPLANT_FLASH_TICKS <= b->just_teared_flash_tick) {
            b->just_teared_flash_tick = 0;
        }
    }

    if (b->phase != BOSS_PLANT_DEAD &&
        b->phase != BOSS_PLANT_SLEEP) {

        if (b->snd_rumble_iID && !boss_plant_any_tentacle_still_emerging(g)) {
            snd_instance_stop_fade(b->snd_rumble_iID, 50, 0);
            b->snd_rumble_iID = 0;
        }
    } else {
        // g->cam.has_trg = 0;
    }

    switch (b->phase) {
    case BOSS_PLANT_SLEEP: {
        if ((b->phase_tick % 100) == 89) {
            snd_play(SNDID_PLANTPULSE, 0.5f, rngr_f32(0.85f, 1.15f));
        }
        break;
    }
    case BOSS_PLANT_INTRO0: break;
    case BOSS_PLANT_CLOSED: {
        if (100 <= b->phase_tick && 50 <= b->tentacle_pt_ticks_ended) {
            b->phase_tick = 0;
            b->phase      = BOSS_PLANT_PREPARE_OPEN;
            if (b->n_pt_back_to_back == 3) {
                b->open_up_action = BOSS_PLANT_OPEN_UP_SHOW;
            } else if (b->n_pt_back_to_back == 0) {
                b->open_up_action = BOSS_PLANT_OPEN_UP_ATK;
            } else {
                b->open_up_action = rngsr_i32(&b->seed, 0, 1);
            }
        }
        break;
    }
    case BOSS_PLANT_PREPARE_OPEN: {
        if ((b->phase_tick % 20) == 1) {
            snd_play(SNDID_PLANTPULSE, 0.75f, rngr_f32(0.85f, 1.15f));
        }
        switch (b->open_up_action) {
        case BOSS_PLANT_OPEN_UP_SHOW: {
            if (50 <= b->phase_tick) {
                b->n_pt_back_to_back = 0;
                b->phase_tick        = 0;
                b->phase             = BOSS_PLANT_OPENED;
                snd_play(SNDID_BPLANT_SHOW, 1.5f, rngr_f32(0.9f, 1.1f));

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
                b->n_pt_back_to_back++;
                i32 i_pt = 1 + rngsr_i32(&b->seed, 0, NUM_BOSS_PLANT_TENTACLE_PT - 1);
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
        if (any_eye_hooked) {
            i32 bt = n_eyes_fake <= 1 ? 60 : 80;
            if ((b->phase_tick % bt) == 0) {
                i32 herox_rel = (powl.x - panchor.x) >> 4;
                herox_rel += rngr_i32(-1, +1);
                herox_rel = clamp_i32(herox_rel, -9, +9);
                boss_plant_tentacle_try_emerge_ext(g, herox_rel, 55, 10);
            }
        }

        if (b->n_ripped == 3) {
            b->ripped_timer++;

            if (100 <= b->tentacle_pt_ticks_ended) {
                i32 i_pt = 1 + rngsr_i32(&b->seed, 0, NUM_BOSS_PLANT_TENTACLE_PT - 1);
                boss_plant_start_tentacle_pt(g, i_pt);
            }
        } else if (n_eyes_fake_ripped) {
            b->ripped_timer++;

            obj_s *o_ripped = boss_plant_eye_is_teared(o_eyefl) ? o_eyefl : o_eyefr;
            obj_s *o_other  = o_ripped == o_eyefl ? o_eyefr : o_eyefl;
            if (b->ripped_timer == 20) {
                boss_plant_eye_show(g, o_other);
            }

            if (!boss_plant_eye_is_hooked(o_other) &&
                !boss_plant_eye_is_teared(o_other) &&
                ((b->eye_ripped_tick + 150) % 300) == 0) {
                i32 dir     = rngsr_i32(&b->seed, 0, 1) * 2 - 1;
                i32 slash_y = rngsr_i32(&b->seed, 3, 7);
                boss_plant_eye_try_attack(g, o_other, slash_y, dir, 0);
            }

            i32 nt_max   = 140;
            i32 nt_min   = 80;
            i32 t_show   = 60;
            i32 t_emerge = 50;
            if (b->n_ripped == 2) {
                nt_max   = 90;
                nt_max   = 60;
                t_show   = 40;
                t_emerge = 40;
            }
            i32 nt = max_i32(nt_max - b->ripped_intensify * 15, nt_min);
            if (b->ripped_timer == nt ||
                (b->ripped_intensify == 0 && b->ripped_timer == 50)) {
                b->ripped_timer = 0;
                b->ripped_intensify++;
                i32 herox_rel = (powl.x - panchor.x) >> 4;
                herox_rel += rngr_i32(-1, +1);
                herox_rel = clamp_i32(herox_rel, -9, +9);
                boss_plant_tentacle_try_emerge_ext(g, herox_rel, t_emerge, t_show);
            }
        } else if (!any_eye_hooked && 35 == b->phase_tick) {
            i32 dir = rngsr_i32(&b->seed, 0, 1) * 2 - 1;

            switch (n_eyes_fake) {
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
                                      {6, 4},
                                      {7, 5}};
                i32       ypt      = rngsr_i32(&b->seed, 0, 2);

                boss_plant_eye_try_attack(g, o_eyefl, ay[ypt][0], dir, 0);
                boss_plant_eye_try_attack(g, o_eyefr, ay[ypt][1], dir, 0);
                break;
            }
            }
        } else if ((320 <= b->phase_tick && !any_eye_attacking) ||
                   (200 <= b->phase_tick && n_eyes_fake == 0 &&
                    !any_eye_hooked)) {
            b->phase_tick = 0;
            b->phase      = BOSS_PLANT_CLOSED;
            snd_play(SNDID_BPLANT_HIDE, 1.f, rngr_f32(0.9f, 1.1f));
            boss_plant_eye_hide(g, o_eye);
            boss_plant_eye_hide(g, o_eyefl);
            boss_plant_eye_hide(g, o_eyefr);
        }
        break;
    }
    case BOSS_PLANT_FINAL_TEARED: {

        break;
    }
    case BOSS_PLANT_FINAL_PHASE: {
        break;
    }
    case BOSS_PLANT_OUTRO0: {

        break;
    }
    }

    boss_plant_tentacle_pt_update(g);
}

void boss_plant_tentacle_try_emerge_ext(g_s *g, i32 tile_x, i32 t_emerge, i32 t_active)
{
    boss_plant_s *b = &g->boss.plant;
    for (i32 n = 0; n < ARRLEN(b->tentacles); n++) {
        if (obj_handle_valid(b->tentacles[n])) continue;

        obj_s *ot       = boss_plant_tentacle_emerge(g, tile_x * 24, 240 - 16, t_emerge, t_active);
        b->tentacles[n] = obj_handle_from_obj(ot);

        // snd_play_ext(SNDID_RUMBLE, 2.5f, 1.f, 0);
        if (!b->snd_rumble_iID) {
            b->snd_rumble_iID = snd_play_ext(SNDID_RUMBLE, 2.f, 1.f, 1);
        }
        break;
    }
}

void boss_plant_tentacle_try_emerge(g_s *g, i32 tile_x)
{
    boss_plant_tentacle_try_emerge_ext(g, tile_x, 30, 50);
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
    i32       fr2  = frame_from_ticks_pingpong(g->tick_gameplay >> 3, 4);

    v2_i32 fr_plant = {0, 0};
    v2_i32 fr_core  = {0, 0};
    v2_i32 fr_vine  = {0, 0};

    texrec_s trcol   = asset_texrec(TEXID_BOSSPLANT, fr1 * 64, 6 * 64, 64, 64);
    texrec_s trplant = asset_texrec(TEXID_BOSSPLANT, 0, 0, 288, 96);
    texrec_s trcore  = asset_texrec(TEXID_BOSSPLANT, 704, 0, 128, 128);
    texrec_s trvine  = asset_texrec(TEXID_BOSSPLANT, (1 + fr2) * 128, 7 * 64, 128, 128);

    bool32 draw_vines = b->draw_vines;
    v2_i32 panchor    = {b->x + cam.x, b->y + cam.y};

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
            b->plant_frame = 1;
        } else {
            b->plant_frame = 0;
        }
        break;
    }
    case BOSS_PLANT_CLOSED:
        if (b->phase_tick < 8) {
            b->plant_frame = 1;
        }
        break;
    case BOSS_PLANT_DEAD:
        b->plant_frame = 3;
        break;
    case BOSS_PLANT_OPENED:
        b->plant_frame = 2;
        break;
    case BOSS_PLANT_PREPARE_OPEN:
        b->plant_frame = ((b->phase_tick >> 3) & 1 ? 1 : 0);
        break;
    case BOSS_PLANT_OUTRO0: break;
    }

    for (i32 n = 0; n < ARRLEN(b->tentacles); n++) {
        obj_s *otentacle = obj_from_obj_handle(b->tentacles[n]);
        if (!otentacle) continue;

        boss_plant_tentacle_on_draw(g, otentacle, cam, ctxt);
    }

    i32 n_eyes_fake = (o_eyefl != 0) + (o_eyefr != 0);
    if (b->phase == BOSS_PLANT_OPENED && n_eyes_fake < 2) {
        texrec_s tcomp = asset_texrec(TEXID_BOSSPLANT, 64 * 5, 64 * 1, 64, 64);
        v2_i32   pcomp = {panchor.x + 25 - 32, panchor.y + 70 - 32};
        if ((b->phase_tick >> 3) & 1) {
            tcomp.y += 64;
            pcomp.y += 2;
        }
        gfx_spr(ctxt, tcomp, pcomp, 0, 0);
    }

    boss_plant_eye_draw(g, o_eye, cam, ctxt);
    boss_plant_eye_draw(g, o_eyefl, cam, ctxt);
    boss_plant_eye_draw(g, o_eyefr, cam, ctxt);

    trplant.y = b->plant_frame * 96;

    v2_i32 posplant = {panchor.x - (trplant.w >> 1), panchor.y + 16};
    gfx_spr(ctxt, trplant, posplant, 0, 0);

    tex_merge_to_opaque_outlined_white(ctx.dst, ttmp);
}

void boss_plant_on_eye_tear_off(g_s *g, obj_s *o)
{
    boss_plant_s *b           = &g->boss.plant;
    obj_s        *o_eye       = obj_from_obj_handle(b->eye);
    obj_s        *o_eyefl     = obj_from_obj_handle(b->eye_fake[0]);
    obj_s        *o_eyefr     = obj_from_obj_handle(b->eye_fake[1]);
    b->just_teared_flash_tick = 1;
    b->n_ripped++;

    switch (o->ID) {
    case OBJID_BOSS_PLANT_EYE: {
        break;
    }
    case OBJID_BOSS_PLANT_EYE_FAKE_L:
    case OBJID_BOSS_PLANT_EYE_FAKE_R: {
        healthdrop_spawn(g, obj_pos_center(o));
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

    if (b->just_teared_flash_tick) {
        i32 pt     = 0;
        i32 patmax = GFX_PATTERN_MAX;
        i32 t1     = BPLANT_FLASH_TICKS_IN;
        i32 t2     = BPLANT_FLASH_TICKS_OUT;
        if (pltf_reduce_flashing()) {
            patmax = 2;
            t1 += 16;
            t2 -= 16;
        }

        if (t1 <= b->just_teared_flash_tick) {
            i32 t = b->just_teared_flash_tick - t1;
            pt    = lerp_i32(patmax, 0, t, t2);
        } else {
            i32 t = b->just_teared_flash_tick;
            pt    = lerp_i32(0, patmax, t, t1);
        }
        ctx.pat = gfx_pattern_bayer_4x4(pt);
        gfx_rec_fill_opaque(ctx, r, PRIM_MODE_WHITE);
    }
}

obj_s *boss_plant_other_eye(g_s *g, obj_s *o)
{
    boss_plant_s *b       = &g->boss.plant;
    obj_s        *o_eyefl = obj_from_obj_handle(b->eye_fake[0]);
    obj_s        *o_eyefr = obj_from_obj_handle(b->eye_fake[1]);
    if (o == o_eyefl) return o_eyefr;
    if (o == o_eyefr) return o_eyefl;
    return 0;
}

void boss_plant_hideshow_other_eye(g_s *g, obj_s *o, b32 show)
{
    boss_plant_s *b       = &g->boss.plant;
    obj_s        *o_other = boss_plant_other_eye(g, o);
    if (show) {
        boss_plant_eye_show(g, o_other);
    } else {
        boss_plant_eye_hide(g, o_other);
    }
}

void boss_plant_on_killed_eye(g_s *g, obj_s *o)
{
    boss_plant_s *b = &g->boss.plant;

    switch (o->ID) {
    case OBJID_BOSS_PLANT_EYE: {
        b->phase       = BOSS_PLANT_OUTRO0;
        b->phase_tick  = 0;
        b->tentacle_pt = 0;
        b->eye_teared  = obj_pos_center(o);
        save_event_register(g, SAVE_EV_BOSS_PLANT);
        mus_play_extv(0, 0, 0, 200, 0, 0);
        for (i32 n = 0; n < ARRLEN(b->tentacles); n++) {
            obj_handle_delete(g, b->tentacles[n]);
        }
        obj_handle_delete(g, b->exithurter[0]);
        obj_handle_delete(g, b->exithurter[1]);
        obj_handle_delete(g, b->exitblocker[0]);
        obj_handle_delete(g, b->exitblocker[1]);
        objs_cull_to_delete(g);
        aud_stop_all_snd_instances();
        cs_bossplant_outro_enter(g);
        snd_play(SNDID_BOSSWIN, 1.f, 1.f);
        break;
    }
    case OBJID_BOSS_PLANT_EYE_FAKE_L:
    case OBJID_BOSS_PLANT_EYE_FAKE_R: {

        break;
    }
    }
}