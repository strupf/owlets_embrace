// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "cam.h"
#include "game.h"

#define CAM_TICKS_TB 30
#define CAM_TICKS_X  200
#define CAM_FACING_X 50

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

    f32    hlerp = (f32)c->hookticks / 100.f;
    v2_f32 hoffs = {(c->hookpos.x - p.x) * hlerp, (c->hookpos.y - p.y) * hlerp};
    if (30.f < v2f_len(hoffs)) {
        v2f_setlen(hoffs, 30.f);
    }
    p = v2f_add(p, hoffs);
    p = v2f_add(p, c->offs);
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
    for (int n = 0; n < 64; n++) {
        cam_update(g, c);
    }
}

void cam_update(game_s *g, cam_s *c)
{
    obj_s *hero = obj_get_tagged(g, OBJ_TAG_HERO);

    c->look_down = 0;
    v2_f32 ppos  = c->pos;

    const int addxticks_prev = c->addxticks;

    v2_f32 padd = {0};
    if (c->mode == CAM_MODE_FOLLOW_HERO && hero) {
        hero_s *h     = (hero_s *)hero->mem;
        v2_i32  herop = obj_pos_bottom_center(hero);

        int    py_bot       = herop.y - 65;
        int    py_top       = herop.y + 20;
        int    target_x     = herop.x;
        int    target_y     = py_bot;
        bool32 herogrounded = obj_grounded(g, hero);

        padd.x = (f32)(target_x - c->pos.x) * .1f;

        if (herogrounded && 0 <= hero->vel_q8.y) {
            // move camera upwards if hero landed on new platform
            // "new base height"
            padd.y = (f32)(target_y - c->pos.y) * .05f;

            // look down when pressing down and standing still
            c->look_down = (inp_pressed(INP_DPAD_D) &&
                            hero->vel_q8.x == 0 &&
                            !h->sliding);
        }

        if (0.05f <= v2f_lensq(padd)) {
            c->pos = v2f_add(c->pos, padd);
        }

        c->pos.y = clamp_f(c->pos.y, (f32)py_bot, (f32)py_top);

        c->addxticks += hero->facing * (hero->vel_q8.x == 0 ? 2 : 3);
        c->addxticks = clamp_i(c->addxticks, -CAM_TICKS_X, +CAM_TICKS_X);

        if (g->herodata.rope_active) {
            v2_i32 rpos = g->herodata.rope.tail->p;
            c->hookticks += 2;
            c->hookticks = min_i(c->hookticks, 30);
            c->hookpos   = (v2f){(f32)rpos.x, (f32)rpos.y};
        }
    }

    static int lookdowntick = 0;

    if (g->substate.state == SUBSTATE_TEXTBOX || shop_active(g)) {
        c->addyticks  = min_i(c->addyticks + 1, CAM_TICKS_TB);
        c->addyoffset = 60;
    } else if (c->look_down) {
        lookdowntick++;
        if (25 <= lookdowntick) {
            c->addyticks  = min_i(c->addyticks + 1, CAM_TICKS_TB);
            c->addyoffset = 100;
        }
    } else if (0 < c->addyticks) {
        c->addyticks--;
    }
    if (0 < c->hookticks) {
        c->hookticks--;
    }

    if (!c->look_down) lookdowntick = 0;

    if (addxticks_prev == c->addxticks && c->addxticks != 0) {
        c->addxticks -= sgn_i(c->addxticks);
    }

    c->offs.x       = (f32)(sgn_i(c->addxticks) * ease_out_quad(0, CAM_FACING_X, abs_i(c->addxticks), CAM_TICKS_X));
    c->offs.y       = (f32)ease_in_out_quad(0, c->addyoffset, c->addyticks, CAM_TICKS_TB);
    c->offs_shake.x = 0.f;
    c->offs_shake.y = 0.f;
    if (0 < c->shake_ticks) {
        int s = (c->shake_str * c->shake_ticks--) / c->shake_ticks_max;

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
