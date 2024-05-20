// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

#define CAM_W  SYS_DISPLAY_W
#define CAM_H  SYS_DISPLAY_H
#define CAM_WH (SYS_DISPLAY_W >> 1)
#define CAM_HH (SYS_DISPLAY_H >> 1)

static v2_i32 cam_constrain_to_room(game_s *g, v2_i32 p_center);

void cam_screenshake(cam_s *c, int ticks, int str)
{
    c->shake_ticks     = ticks;
    c->shake_ticks_max = ticks;
    c->shake_str       = str;
}

v2_i32 cam_pos_px(game_s *g, cam_s *c)
{
    v2_f32 pprev = c->pos;
    v2_f32 p     = c->pos;

    p = v2f_add(p, c->look_ahead);
    p = v2f_add(p, c->camattr);
    if (c->locked_x) {
        p.x = pprev.x;
    }
    if (c->locked_y) {
        p.y = pprev.y;
    }
    p = v2f_add(p, c->offs_shake);

    v2_i32 v = {(i32)(p.x + .5f), (i32)(p.y + .5f)};
    v2_i32 r = cam_constrain_to_room(g, v);
    return r;
}

rec_i32 cam_rec_px(game_s *g, cam_s *c)
{
    v2_i32  p = cam_pos_px(g, c);
    rec_i32 r = {p.x - CAM_WH, p.y - CAM_HH, CAM_W, CAM_H};
    return r;
}

void cam_set_pos_px(cam_s *c, int x, int y)
{
    c->pos.x = (f32)x;
    c->pos.y = (f32)y;
}

void cam_init_level(game_s *g, cam_s *c)
{
    for (i32 n = 0; n < 128; n++) {
        cam_update(g, c);
    }
}

void cam_update(game_s *g, cam_s *c)
{
    obj_s *hero = obj_get_tagged(g, OBJ_TAG_HERO);

    v2_f32 ppos   = c->pos;
    v2_f32 padd   = {0};
    v2_f32 lahead = {0};
    i32    dpad_y = inp_dpad_y();

    const i32 look_tickp = c->look_tick;
    if (c->mode == CAM_MODE_FOLLOW_HERO && hero) {
        hero_s *h     = (hero_s *)hero->mem;
        v2_i32  herop = obj_pos_bottom_center(hero);

        i32    py_bot       = herop.y - 60;
        i32    py_top       = herop.y + 20;
        i32    target_x     = herop.x;
        i32    target_y     = py_bot;
        bool32 herogrounded = obj_grounded(g, hero);

        padd.x = (f32)(target_x - c->pos.x) * .1f;

        if (herogrounded && 0 <= hero->vel_q8.y) {
            // move camera upwards if hero landed on new platform
            // "new base height"
            padd.y = (f32)(target_y - c->pos.y) * .05f;
            if (abs_i(hero->vel_q8.x) < 64 && dpad_y) {
                if (20 <= ++c->look_tick) {
                    lahead.y = (f32)(dpad_y * 50);
                }
            }
        }

        if (0.05f <= v2f_lensq(padd)) {
            c->pos = v2f_add(c->pos, padd);
        }

        c->pos.y = clamp_f(c->pos.y, (f32)py_bot, (f32)py_top);
        lahead.x = (f32)hero->facing * 50.f + (f32)hero->vel_q8.x * 0.05f;
    }

    if (c->look_tick == look_tickp) {
        c->look_tick = 0;
    }

    if (g->substate == SUBSTATE_TEXTBOX || shop_active(g)) {
        lahead.y = 60.f;
    }

    if (abs_f(c->look_ahead.x - lahead.x) < 0.75f) {
        c->look_ahead.x = lahead.x;
    } else {
        c->look_ahead.x = lerp_f32(c->look_ahead.x, lahead.x, 0.025f);
    }

    if (abs_f(c->look_ahead.y - lahead.y) < 0.75f) {
        c->look_ahead.y = lahead.y;
    } else {
        c->look_ahead.y = lerp_f32(c->look_ahead.y, lahead.y, 0.125f);
    }

    v2_f32 dtca = {0};
    i32    nc   = 0;
    for (i32 n = 0; n < c->n_attractors; n++) {
        v2_i32 ca  = c->attractors[n];
        v2_f32 dc  = v2f_sub((v2_f32){(f32)ca.x, (f32)ca.y}, c->pos);
        f32    lsq = v2f_lensq(dc);
        if (POW2(CAM_ATTRACTOR_RADIUS) < lsq) continue;
        dtca = v2f_add(dtca, dc);
        nc++;
    }
    if (nc) {
        dtca      = v2f_mul(dtca, 0.5f / (f32)nc);
        f32 dtlen = v2f_len(dtca);
        if (CAM_ATTRACTOR_MAX_OFFS < dtlen) {
            dtca = v2f_setlen(dtca, CAM_ATTRACTOR_MAX_OFFS);
        }

        c->camattr = v2f_lerp(c->camattr, dtca, 0.025f);
    } else {
        c->camattr = v2f_lerp(c->camattr, (v2_f32){0}, 0.025f);
    }

    c->offs_shake.x = 0.f;
    c->offs_shake.y = 0.f;
    if (0 < c->shake_ticks) {
        i32 s = (c->shake_str * c->shake_ticks--) / c->shake_ticks_max;

        c->offs_shake.x = (f32)rngr_sym_i32(s);
        c->offs_shake.y = (f32)rngr_sym_i32(s);
    }

    if (c->locked_x) {
        c->pos.x = ppos.x;
    }
    if (c->locked_y) {
        c->pos.y = ppos.y;
    }
}

static v2_i32 cam_constrain_to_room(game_s *g, v2_i32 p_center)
{
    v2_i32 v = {clamp_i(p_center.x, CAM_WH, g->pixel_x - CAM_WH),
                clamp_i(p_center.y, CAM_HH, g->pixel_y - CAM_HH)};
    return v;
}

v2_i32 cam_offset_max(game_s *g, cam_s *c)
{
    v2_i32 o = {g->pixel_x - CAM_W,
                g->pixel_y - CAM_H};
    return o;
}