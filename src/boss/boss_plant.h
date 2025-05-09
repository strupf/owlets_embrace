// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef BOSS_PLANT_H
#define BOSS_PLANT_H

#include "gamedef.h"
#include "map_loader.h"

enum {
    BOSS_PLANT_SLEEP,
    BOSS_PLANT_DEAD,
    //
    BOSS_PLANT_INTRO0,
    BOSS_PLANT_CLOSED,
    BOSS_PLANT_PREPARE_OPEN,
    BOSS_PLANT_OPENED,
    BOSS_PLANT_FINAL_TEARED,
    BOSS_PLANT_FINAL_PHASE,
    //
    BOSS_PLANT_OUTRO0,
};

enum {
    BOSS_PLANT_EYE_HIDDEN,
    BOSS_PLANT_EYE_SHOWN,
    BOSS_PLANT_EYE_GRAB_COMP,
    BOSS_PLANT_EYE_GRABBED_COMP,
    BOSS_PLANT_EYE_HOOKED,
    BOSS_PLANT_EYE_ATTACK,
    BOSS_PLANT_EYE_ATTACK_EXE,
    BOSS_PLANT_EYE_RIPPED,
};

typedef struct {
    v2_i32 pp_q8;
    v2_i32 p_q8;
} bplant_seg_s;

typedef struct boss_plant_s {
    u32          seed;
    i32          tick;
    i32          phase_tick;
    i32          phase;
    i32          x;
    i32          y;
    i32          snd_rumble_iID;
    v2_i32       eye_teared; // position of teared eye in outro
    i16          open_up_action;
    i16          tick_tentacle_pt;
    i16          tentacle_pt;
    i16          tentacle_pt_n;
    i16          tentacle_pt_ticks_ended;
    i32          eye_ripped_tick;
    i32          just_teared_flash_tick;
    i16          eyes_killed;
    i16          ripped_intensify;
    i16          ripped_timer;
    i16          n_ripped;
    i16          tentacle_pt_spare_x;
    i16          n_pt_back_to_back;
    i32          time_of_slash_sfx;
    bool32       draw_vines;
    i32          plant_frame;
    obj_handle_s o_cam_attract;
    obj_handle_s eye;
    obj_handle_s eye_fake[2];
    obj_handle_s exitblocker[2];
    obj_handle_s exithurter[2];
    obj_handle_s tentacles[16];
} boss_plant_s;

void   boss_plant_load(g_s *g, map_obj_s *mo);
void   boss_plant_update(g_s *g);
void   boss_plant_draw(g_s *g, v2_i32 cam);
void   boss_plant_draw_post(g_s *g, v2_i32 cam);
void   boss_plant_wake_up(g_s *g);
void   boss_plant_update_seg(bplant_seg_s *segs, i32 num, i32 l);
void   boss_plant_on_eye_tear_off(g_s *g, obj_s *o);
obj_s *boss_plant_other_eye(g_s *g, obj_s *o);
void   boss_plant_hideshow_other_eye(g_s *g, obj_s *o, b32 show);
void   boss_plant_barrier_poof(g_s *g);
void   boss_plant_tentacle_pt_update(g_s *g);
void   boss_plant_tentacle_try_emerge(g_s *g, i32 tile_x);
obj_s *boss_plant_tentacle_emerge(g_s *g, i32 x, i32 y, i32 t_emerge, i32 t_active);
void   boss_plant_tentacle_on_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
obj_s *boss_plant_eye_create(g_s *g, i32 ID);
void   boss_plant_eye_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
void   boss_plant_eye_hide(g_s *g, obj_s *o);
void   boss_plant_eye_show(g_s *g, obj_s *o);
bool32 boss_plant_eye_is_hooked(obj_s *o);
bool32 boss_plant_eye_is_busy(obj_s *o);
bool32 boss_plant_eye_is_teared(obj_s *o);
bool32 boss_plant_eye_try_attack(g_s *g, obj_s *o, i32 slash_y_slot, i32 slash_sig, b32 x_slash);
void   boss_plant_on_killed_eye(g_s *g, obj_s *o);
void   boss_plant_eye_move_to_centerpx(g_s *g, obj_s *o, i32 x, i32 y);

#endif