// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "core/inp.h"
#include "settings.h"
#include "util/mathfunc.h"

static struct {
    inp_state_s curri;
    inp_state_s previ;
#if PLTF_SDL
    SDL_GameController *gc;
#endif
} INP;

void inp_update()
{
    INP.previ      = INP.curri;
    inp_state_s *i = &INP.curri;
    mclr(i, sizeof(inp_state_s));
#if PLTF_PD
    i->actions = pltf_pd_btn() & B8(00111111);
    if (SETTINGS.swap_a_b_buttons) {
        u32 actions = i->actions;
        i->actions &= ~(INP_A | INP_B);
        if (actions & INP_A) {
            i->actions |= INP_B;
        }
        if (actions & INP_B) {
            i->actions |= INP_A;
        }
    }

    if (pltf_pd_crank_docked()) i->actions |= INP_CRANK_DOCK;
    i->crank_q16 = (i32)(pltf_pd_crank_deg() * 182.0444f) & 0xFFFFU;
#else
#if 1
    // makeshift controller support for now
    static i32 once = 0;
    if (once == 0) {
        once   = 1;
        INP.gc = SDL_GameControllerOpen(0);
    }

    if (SDL_GameControllerGetButton(INP.gc, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
        SDL_GameControllerGetAxis(INP.gc, SDL_CONTROLLER_AXIS_LEFTY) < -4096) i->actions |= INP_DU;
    if (SDL_GameControllerGetButton(INP.gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
        SDL_GameControllerGetAxis(INP.gc, SDL_CONTROLLER_AXIS_LEFTY) > +4096) i->actions |= INP_DD;
    if (SDL_GameControllerGetButton(INP.gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
        SDL_GameControllerGetAxis(INP.gc, SDL_CONTROLLER_AXIS_LEFTX) < -4096) i->actions |= INP_DL;
    if (SDL_GameControllerGetButton(INP.gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
        SDL_GameControllerGetAxis(INP.gc, SDL_CONTROLLER_AXIS_LEFTX) > +4096) i->actions |= INP_DR;
    if (SDL_GameControllerGetButton(INP.gc, SDL_CONTROLLER_BUTTON_A)) i->actions |= INP_B;
    if (SDL_GameControllerGetButton(INP.gc, SDL_CONTROLLER_BUTTON_B)) i->actions |= INP_A;
#endif
    if (pltf_sdl_key(SDL_SCANCODE_W)) i->actions |= INP_DU;
    if (pltf_sdl_key(SDL_SCANCODE_S)) i->actions |= INP_DD;
    if (pltf_sdl_key(SDL_SCANCODE_A)) i->actions |= INP_DL;
    if (pltf_sdl_key(SDL_SCANCODE_D)) i->actions |= INP_DR;
    if (pltf_sdl_key(SDL_SCANCODE_COMMA)) i->actions |= INP_B;
    if (pltf_sdl_key(SDL_SCANCODE_PERIOD)) i->actions |= INP_A;
    if (pltf_sdl_key(SDL_SCANCODE_N)) i->actions |= INP_B;
    if (pltf_sdl_key(SDL_SCANCODE_M)) i->actions |= INP_A;
    i->actions |= INP_CRANK_DOCK;
#endif
}

void inp_on_resume()
{
    inp_update();
}

i32 inp_x()
{
    if (inp_btn(INP_DL)) return -1;
    if (inp_btn(INP_DR)) return +1;
    return 0;
}

i32 inp_y()
{
    if (inp_btn(INP_DU)) return -1;
    if (inp_btn(INP_DD)) return +1;
    return 0;
}

i32 inp_xp()
{
    if (inp_btnp(INP_DL)) return -1;
    if (inp_btnp(INP_DR)) return +1;
    return 0;
}

i32 inp_yp()
{
    if (inp_btnp(INP_DU)) return -1;
    if (inp_btnp(INP_DD)) return +1;
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

bool32 inp_btn(i32 b)
{
    return (INP.curri.actions & b);
}

bool32 inp_btnp(i32 b)
{
    return (INP.previ.actions & b);
}

bool32 inp_btn_jp(i32 b)
{
    return (inp_btn(b) && !inp_btnp(b));
}

bool32 inp_btn_jr(i32 b)
{
    return (!inp_btn(b) && inp_btnp(b));
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
    mclr(c, sizeof(inp_crank_click_s));
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

static inline i32 inpst_x(inp_state_s i)
{
    if (i.actions & INP_DR) return +1;
    if (i.actions & INP_DL) return -1;
    return 0;
}

static inline i32 inpst_y(inp_state_s i)
{
    if (i.actions & INP_DD) return +1;
    if (i.actions & INP_DU) return -1;
    return 0;
}

static inline i32 inpst_btn(inp_state_s i, i32 b)
{
    return (i.actions & b);
}

static inline i32 inpst_crank_q16(inp_state_s i)
{
    return i.crank_q16;
}

inp_s inp_cur()
{
    inp_s i = {INP.previ, INP.curri};
    return i;
}

i32 inps_x(inp_s i)
{
    return inpst_x(i.c);
}

i32 inps_y(inp_s i)
{
    return inpst_y(i.c);
}

i32 inps_xp(inp_s i)
{
    return inpst_x(i.p);
}

i32 inps_yp(inp_s i)
{
    return inpst_y(i.p);
}

i32 inps_btn(inp_s i, i32 b)
{
    return inpst_btn(i.c, b);
}

i32 inps_btnp(inp_s i, i32 b)
{
    return inpst_btn(i.p, b);
}

i32 inps_btn_jp(inp_s i, i32 b)
{
    return (inps_btn(i, b) && !inps_btnp(i, b));
}

i32 inps_btn_jr(inp_s i, i32 b)
{
    return (!inps_btn(i, b) && inps_btnp(i, b));
}

i32 inps_crank_q16(inp_s i)
{
    return inpst_crank_q16(i.c);
}

i32 inps_crankp_q16(inp_s i)
{
    return inpst_crank_q16(i.p);
}

i32 inps_crankdt_q16(inp_s i)
{
    i32 ac = inps_crank_q16(i);
    i32 ap = inps_crankp_q16(i);
    return inp_crank_calc_dt_q16(ap, ac);
}
