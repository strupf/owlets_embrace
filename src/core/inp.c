// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "inp.h"
#include "util/mathfunc.h"

#define INP_ACC_K 0.35f // accelerometer smoothing: A * (1-K) + B * K

static struct {
    inp_state_s  curri;
    inp_state_s  previ;
    inp_config_s cfg;
    f32          acc_x;
    f32          acc_y;
    f32          acc_z;
} INP;

void inp_init()
{
    INP.cfg = inp_config_default();
}

inp_config_s inp_config_default()
{
    inp_config_s c    = {0};
    c.acc_sensitivity = 0.80f;
    return c;
}

inp_config_s inp_config_get()
{
    return INP.cfg;
}

void inp_config_set(inp_config_s c)
{
    INP.cfg = c;
}

void inp_update()
{
    INP.previ = INP.curri;
    INP.curri = (inp_state_s){0};
#ifdef PLTF_PD
    INP.curri.actions = pltf_pd_btn() & B8(00111111);

    if (pltf_pd_crank_docked()) INP.curri.actions |= INP_CRANK_DOCK;
    INP.curri.crank_q16 = (i32)(pltf_pd_crank_deg() * 182.0444f) & 0xFFFFU;
#else
    static SDL_GameController *gc;
    static i32                 once = 0;
    if (once == 0) {
        once = 1;
        gc   = SDL_GameControllerOpen(0);
    }
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
        SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) < 0) INP.curri.actions |= INP_DU;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
        SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) > 0) INP.curri.actions |= INP_DD;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
        SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) < 0) INP.curri.actions |= INP_DL;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
        SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) > 0) INP.curri.actions |= INP_DR;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A)) INP.curri.actions |= INP_B;
    if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B)) INP.curri.actions |= INP_A;

    if (pltf_sdl_key(SDL_SCANCODE_W)) INP.curri.actions |= INP_DU;
    if (pltf_sdl_key(SDL_SCANCODE_S)) INP.curri.actions |= INP_DD;
    if (pltf_sdl_key(SDL_SCANCODE_A)) INP.curri.actions |= INP_DL;
    if (pltf_sdl_key(SDL_SCANCODE_D)) INP.curri.actions |= INP_DR;
    if (pltf_sdl_key(SDL_SCANCODE_COMMA)) INP.curri.actions |= INP_B;
    if (pltf_sdl_key(SDL_SCANCODE_PERIOD)) INP.curri.actions |= INP_A;
    if (pltf_sdl_key(SDL_SCANCODE_N)) INP.curri.actions |= INP_B;
    if (pltf_sdl_key(SDL_SCANCODE_M)) INP.curri.actions |= INP_A;
    INP.curri.actions |= INP_CRANK_DOCK;
#endif

#if PLTF_ACCELEROMETER_SUPPORT
    f32 x, y, z;
    pltf_accelerometer(&x, &y, &z);

    INP.acc_x = INP.acc_x + (x - INP.acc_x) * INP_ACC_K;
    INP.acc_y = INP.acc_y + (y - INP.acc_y) * INP_ACC_K;
    INP.acc_z = INP.acc_z + (z - INP.acc_z) * INP_ACC_K;
    f32 dx    = x - INP.acc_x;
    f32 dy    = y - INP.acc_y;
    f32 dz    = z - INP.acc_z;
    if (INP.cfg.acc_sensitivity <= (dx * dx + dy * dy + dz * dz)) {
        INP.curri.actions |= INP_SHAKE;
    }
#endif
}

void inp_on_resume()
{
    inp_update();
}

i32 inp_x()
{
    if (inp_action(INP_DL)) return -1;
    if (inp_action(INP_DR)) return +1;
    return 0;
}

i32 inp_y()
{
    if (inp_action(INP_DU)) return -1;
    if (inp_action(INP_DD)) return +1;
    return 0;
}

i32 inp_xp()
{
    if (inp_actionp(INP_DL)) return -1;
    if (inp_actionp(INP_DR)) return +1;
    return 0;
}

i32 inp_yp()
{
    if (inp_actionp(INP_DU)) return -1;
    if (inp_actionp(INP_DD)) return +1;
    return 0;
}

v2_i32 inp_dir()
{
    v2_i32 v = {inp_x(), inp_y()};
    return v;
}

v2_i32 inp_dirp()
{
    v2_i32 v = {inp_xp(), inp_yp()};
    return v;
}

bool32 inp_action(i32 b)
{
    return (INP.curri.actions & b);
}

bool32 inp_actionp(i32 b)
{
    return (INP.previ.actions & b);
}

bool32 inp_action_jp(i32 b)
{
    return (inp_action(b) && !inp_actionp(b));
}

bool32 inp_action_jr(i32 b)
{
    return (!inp_action(b) && inp_actionp(b));
}

i32 inp_crank_q16()
{
    return INP.curri.crank_q16;
}

i32 inp_crankp_q16()
{
    return INP.previ.crank_q16;
}

i32 inp_crank_qx(i32 q)
{
    return q_convert_i32(inp_crank_q16(), 16, q);
}

i32 inp_crankp_qx(i32 q)
{
    return q_convert_i32(inp_crankp_q16(), 16, q);
}

i32 inp_crank_dt_q16()
{
    return inp_crank_calc_dt_qx(16, inp_crankp_q16(), inp_crank_q16());
}

i32 inp_crank_calc_dt_q16(i32 ang_from, i32 ang_to)
{
    return inp_crank_calc_dt_qx(16, ang_from, ang_to);
}

i32 inp_crank_calc_dt_qx(i32 q, i32 ang_from, i32 ang_to)
{
    i32 dt = ang_to - ang_from;
    if (dt <= -(1 << (q - 1))) return (dt + (1 << q));
    if (dt >= +(1 << (q - 1))) return (dt - (1 << q));
    return dt;
}

i32 inp_dpad_dir()
{
    i32 x = inp_x();
    i32 y = inp_y();
    i32 d = ((y + 1) << 2) | (x + 1);
    switch (d) {
    case 1: return INP_DPAD_DIR_N;
    case 9: return INP_DPAD_DIR_S;
    case 6: return INP_DPAD_DIR_E;
    case 4: return INP_DPAD_DIR_W;
    case 10: return INP_DPAD_DIR_SE;
    case 8: return INP_DPAD_DIR_SW;
    case 0: return INP_DPAD_DIR_NW;
    case 2: return INP_DPAD_DIR_NE;
    }
    return INP_DPAD_DIR_NONE;
}

void inp_crank_click_init(inp_crank_click_s *c, i32 n_seg, i32 offs)
{
    *c         = (inp_crank_click_s){0};
    c->n_segs  = n_seg;
    c->ang_off = (offs * 0x10000) / (c->n_segs << 1);
}

i32 inp_crank_click_turn_by(inp_crank_click_s *c, i32 dt_q16)
{
    if (!dt_q16) return 0;
    assert(c->n_segs);
    i32 cprev = (i32)c->ang;
    i32 ccurr = (i32)c->ang + dt_q16;
    c->ang    = ccurr & 0xFFFF;

    i32 cp = (cprev + c->ang_off) & 0xFFFF;
    i32 cc = (ccurr + c->ang_off) & 0xFFFF;
    i32 dt = 0;
    for (i32 ccx = -1; ccx <= +1; ccx += 2) {
        if (0 >= ccx * dt_q16) continue;

        while (1) {
            i32 i    = (c->n + ccx + c->n_segs) % c->n_segs;
            i32 iang = (i * 0x10000) / c->n_segs;
            i32 dta  = inp_crank_calc_dt_q16(cp, iang);
            i32 dtb  = inp_crank_calc_dt_q16(iang, cc);

            if (0 > ccx * dta || 0 > ccx * dtb) break;
            c->n = i;
            dt += ccx;
        }
    }
    return dt;
}