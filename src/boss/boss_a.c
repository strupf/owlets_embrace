// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss/boss_a.h"
#include "app.h"
#include "game.h"

void boss_a_intro_cb_puppet(g_s *g, obj_s *o, void *ctx);

void boss_a_intro_cb_puppet(g_s *g, obj_s *o, void *ctx)
{
    boss_a_s *b = (boss_a_s *)ctx;
    if (o == b->puppet_comp) {
        b->puppet_comp_flag = 1;
    }
    if (o == b->puppet_owl) {
        b->puppet_owl_flag = 1;
    }
}

v2_i32 boss_a_cam_handler(g_s *g)
{
    boss_a_s *b     = &g->boss.boss_a;
    obj_s    *o_owl = owl_if_present_and_alive(g);

    i32 dx   = o_owl->pos.x - b->x_anchor;
    dx       = clamp_sym_i32(dx / 2, 200);
    v2_i32 p = {b->x_anchor + dx, 120};
    return p;
}

// initialize upon level loading
void boss_a_load(g_s *g, boss_a_s *b)
{
    obj_s *o_spawn = obj_find_ID_subID(g, OBJID_MISC, 1000, 0);
    if (!o_spawn) return;

    b->x_anchor = o_spawn->pos.x + o_spawn->w / 2;
    b->y_anchor = o_spawn->pos.y;

    if (saveID_has(g, SAVEID_BOSS_PLANT)) {
        b->phase = BOSS_A_DEFEATED;
    } else {
        b->phase         = BOSS_A_ASLEEP;
        obj_s *o_cam_rec = obj_find_ID_subID(g, OBJID_CAM_REC, 1, 0);
        if (o_cam_rec) {
            cam_clamp_setr_hard(g, obj_aabb(o_cam_rec));
        }
    }
    tex_from_wad_ID(TEXID_BOSS, "T_BOSS_A", game_allocator_room(g));
}

void boss_a_update(g_s *g, boss_a_s *b)
{
    b->phase_tick++;
    obj_s *owl = obj_get_owl(g);

    for (i32 i = 0; i < ARRLEN(b->o_tendrils); i++) {
        obj_s *o_tendril = obj_from_handle(b->o_tendrils[i]);
        boss_a_tendril_update(g, o_tendril);
    }

    switch (b->phase) {
    default: break;
    case BOSS_A_INTRO_0: {
        if (!owl_wait_for_idle(g)) break;

        b->puppet_owl = puppet_owl_put(g, owl);
        puppet_set_anim(b->puppet_owl, PUPPET_OWL_ANIMID_IDLE, 0);
        if (b->o_comp) {
            if (g->owl.stance == OWL_STANCE_ATTACK) {
                owl_set_stance(g, owl, OWL_STANCE_GRAPPLE);
            }
            b->puppet_comp = puppet_companion_put(g, b->o_comp);

            v2_i32 hpos = {b->x_anchor - 100, b->y_anchor + 30};

            puppet_set_anim(b->puppet_comp, PUPPET_COMPANION_ANIMID_FLY, +1);
            puppet_move_ext(b->puppet_comp, hpos, 40, 0, 0, boss_a_intro_cb_puppet, b);
        }
        b->phase      = BOSS_A_INTRO_10;
        b->phase_tick = 0;
        break;
    }
    case BOSS_A_INTRO_10: {
        if (b->puppet_comp_flag) {
            b->puppet_comp_flag = 0;
            b->phase            = BOSS_A_INTRO_20;
            b->phase_tick       = 0;
            puppet_set_anim(b->puppet_comp, PUPPET_COMPANION_ANIMID_HUH, 0);
            dia_load_from_wad(g, "D_BPLANT_0");
        }
        break;
    }
    case BOSS_A_INTRO_20: {
        break;
    }
    case BOSS_A_INTRO_30: {
        bool32 dobump = 0;

        switch (b->phase_tick) {
        case 20:
            puppet_move_ext(b->puppet_comp, (v2_i32){+20, 0}, 30, 0, 1, 0, 0);
            break;
        case 60:
            dobump = 1;
            break;
        case 70:
            puppet_move_ext(b->puppet_comp, (v2_i32){-20, 0}, 30, 0, 1, 0, 0);
            break;
        case 110:
            puppet_move_ext(b->puppet_comp, (v2_i32){+20, 0}, 30, 0, 1, 0, 0);
            break;
        case 130:
            dobump = 1;
            break;
        case 180:
            dobump = 1;
            break;
        case 200:
            puppet_move_ext(b->puppet_comp, (v2_i32){-20, 0}, 30, 0, 1, 0, 0);
            break;
        case 250:
            b->phase      = BOSS_A_INTRO_40;
            b->phase_tick = 0;
            break;
        }

        if (dobump) {
            sfx_cuef(SFXID_STOMP_LAND, 0.8f, 1.f);
            puppet_set_anim(b->puppet_comp, PUPPET_COMPANION_ANIMID_BUMP_ONCE, 0);
        }
        break;
    }
    case BOSS_A_INTRO_40: {
        b->phase      = BOSS_A_INTRO_END;
        b->phase_tick = 0;
        break;
    }
    case BOSS_A_INTRO_END: {
        // could be function call, but this way it's more obvious from a control flow
        if (b->puppet_comp) {
            puppet_companion_replace_and_del(g, b->o_comp, b->puppet_comp);
        }
        if (b->puppet_owl) {
            puppet_owl_replace_and_del(g, obj_get_owl(g), b->puppet_owl);
        }
        b->puppet_comp = 0;
        b->puppet_owl  = 0;
        b->o_comp      = 0;
        g->flags &= ~GAME_FLAG_BLOCK_PLAYER_INPUT;
        saveID_put(g, SAVEID_BOSS_PLANT_INTRO_SEEN);
        b->phase      = BOSS_A_P1_IDLE;
        b->phase_tick = 0;
        b->counter    = 0;

        for (i32 i = 0; i < BOSS_A_NUM_TENDRILS; i++) {
            obj_s *o_tendril = obj_from_handle(b->o_tendrils[i]);
            boss_a_tendril_set_show(o_tendril, 1);
        }
        break;
    }
    case BOSS_A_P1_IDLE: {
        i32 tt = b->phase_tick % 200;
#if 1
        if ((b->phase_tick % 200) == 0) {
            for (i32 i = 0; i < BOSS_A_NUM_TENDRILS; i++) {
                obj_s *o_tendril = obj_from_handle(b->o_tendrils[i]);

                v2_i32 slash0 = {b->x_anchor - 180, b->y_anchor + 100 + i * 30};
                v2_i32 slash1 = {b->x_anchor + 180, slash0.y};
                boss_a_tendril_slash(o_tendril, slash0, slash1);
            }
        }
#else
        if (tt == 0) {
            for (i32 i = 0; i < BOSS_A_NUM_TENDRILS; i++) {
                obj_s *o_tendril = obj_from_handle(b->o_tendrils[i]);
                // boss_a_tendril_set_show(o_tendril, 1);
            }
            //  boss_a_core_set_show(b->o_core, 1);
            // boss_a_plant_set(g, b, BOSS_A_PLANT_ST_OPEN);
        }
        if (tt == 100) {
            for (i32 i = 0; i < BOSS_A_NUM_TENDRILS; i++) {
                obj_s *o_tendril = obj_from_handle(b->o_tendrils[i]);
                //  boss_a_tendril_set_show(o_tendril, 0);
            }
            // boss_a_core_set_show(b->o_core, 0);
            // boss_a_plant_set(g, b, BOSS_A_PLANT_ST_CLOSED);
        }
#endif

        break;
    }
    case BOSS_A_P1_TO_P2: {
        if (50 == b->phase_tick) {
            cam_clamp_y2(g, 0);
            game_on_trigger(g, 10);
            b->phase      = BOSS_A_P2_IDLE;
            b->phase_tick = 0;
        }
        break;
    }
    case BOSS_A_P2_IDLE: {
        i32 tt = b->phase_tick % 200;
        if (tt == 200 / 2) {
            boss_a_core_set_show(b->o_core, 0);
        }
        if (tt == 0) {
            boss_a_core_set_show(b->o_core, 1);
        }
        break;
    }
    case BOSS_A_OUTRO_0: {
        if (20 <= b->phase_tick) {
            b->phase      = BOSS_A_DEFEATED;
            b->phase_tick = 0;
            cam_clamp_clr(g);
            game_on_trigger(g, 1);
            background_fade_to(g, 0, 150);
            cam_clamp_clr(g);
        }
        break;
    }
    }

    boss_a_core_on_update(g, b->o_core);
}

void boss_a_on_trigger(g_s *g, i32 trigger, boss_a_s *b)
{
    switch (b->phase) {
    case BOSS_A_INTRO_0: {

        break;
    }
    case BOSS_A_INTRO_20: {
        if (trigger == TRIGGER_DIA_END) {
            b->phase      = BOSS_A_INTRO_30;
            b->phase_tick = 0;
        }

        break;
    }
    }
}

void boss_a_animate(g_s *g, boss_a_s *b)
{
    for (i32 i = 0; i < ARRLEN(b->o_tendrils); i++) {
        obj_s *o_tendril = obj_from_handle(b->o_tendrils[i]);
        boss_a_tendril_animate(g, o_tendril);
    }

    boss_a_core_on_animate(g, b->o_core);
    boss_a_plant_animate(g, b);
}

void boss_a_on_killed_core(g_s *g, boss_a_s *b)
{
    saveID_put(g, SAVEID_BOSS_PLANT);
    b->phase      = BOSS_A_OUTRO_0;
    b->phase_tick = 0;
}

void boss_a_draw(g_s *g, boss_a_s *b, v2_i32 cam)
{
    gfx_ctx_s ctx   = gfx_ctx_from_tex(asset_tex(TEXID_DISPLAY_WHITE_OUTLINED));
    tex_s     tboss = asset_tex(TEXID_BOSS);

    for (i32 i = 0; i < ARRLEN(b->o_tendrils); i++) {
        obj_s *o_tendril = obj_from_handle(b->o_tendrils[i]);
        boss_a_tendril_draw(g, o_tendril, cam);
    }
    boss_a_core_on_draw(g, b->o_core, cam);
    boss_a_plant_draw(g, b, cam);
}

void boss_a_draw_post(g_s *g, boss_a_s *b, v2_i32 cam)
{
    switch (b->phase) {
    default: break;
    }
}

void boss_a_awake(g_s *g)
{
    pltf_log("Wake up!\n");

    boss_a_s *b = &g->boss.boss_a;
    game_on_trigger(g, 1);
    background_fade_to(g, -64, 150);
    obj_s *o_cam_rec = obj_find_ID_subID(g, OBJID_CAM_REC, 2, 0);
    if (o_cam_rec) {
        cam_clamp_x1(g, o_cam_rec->pos.x);
        cam_clamp_x2(g, o_cam_rec->pos.x + o_cam_rec->w);
    }

    v2_i32 p_anchor_boss = {b->x_anchor, b->y_anchor};

    for (i32 n = 0; n < BOSS_A_NUM_TENDRILS; n++) {
        obj_s *o_tendril = boss_a_tendril_create(g, b, p_anchor_boss, n * 2 - 1);
        b->o_tendrils[n] = handle_from_obj(o_tendril);
    }

    b->o_core = boss_a_core_create(g, b, p_anchor_boss);
    b->o_comp = obj_get_comp(g);

    g->flags |= GAME_FLAG_BLOCK_PLAYER_INPUT;
    if (b->o_comp) {
    }

    if (1) {
        b->phase = BOSS_A_INTRO_END;
    } else {
        b->phase = BOSS_A_INTRO_0;
    }
    b->phase_tick = 0;
}

void boss_a_p2_init(g_s *g, boss_a_s *b)
{
    b->phase      = BOSS_A_P1_TO_P2;
    b->phase_tick = 0;
    game_on_trigger(g, 10);
}

void boss_a_segments_constrain(boss_a_segment_s *segs, i32 num, i32 loops, i32 l)
{
    for (i32 i = 0; i < loops; i++) {
        for (i32 k = 1; k < num; k++) {
            boss_a_segment_s *s0 = &segs[k - 1];
            boss_a_segment_s *s1 = &segs[k];

            v2_i32 p0  = s0->p_q8;
            v2_i32 p1  = s1->p_q8;
            v2_i32 dt  = v2_i32_sub(p1, p0);
            i32    len = v2_i32_len_appr(dt);
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

void boss_a_plant_set(g_s *g, boss_a_s *b, i32 state)
{
    b->plant_state = state;
    b->plant_tick  = 0;
}

void boss_a_plant_animate(g_s *g, boss_a_s *b)
{
    b->plant_tick++;
}

void boss_a_plant_draw(g_s *g, boss_a_s *b, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_from_tex(asset_tex(TEXID_DISPLAY_WHITE_OUTLINED));

    i32 fx = 0;
    i32 fy = 0;
    i32 fw = 288;
    i32 fh = 96;

    switch (b->plant_state) {
    case BOSS_A_PLANT_ST_CLOSED_ASLEEP: {
        i32 fcurr = 3 <= (((b->plant_tick >> 4) & 3));
        i32 fprev = 3 <= (((b->plant_tick - 1) >> 4) & 3);
        fy        = fcurr;
        break;
    }
    case BOSS_A_PLANT_ST_CLOSED: {
        fy = 0;
        break;
    }
    case BOSS_A_PLANT_ST_OPEN: {
        fy = 2;
        break;
    }
    case BOSS_A_PLANT_ST_DEAD: {
        fy = 3;
        break;
    }
    }

    texrec_s tr  = asset_texrec(TEXID_BOSS, fx * fw, fy * fh, fw, fh);
    v2_i32   pos = {b->x_anchor - fw / 2, b->y_anchor};
    pos          = v2_i32_add(pos, cam);
    // pltf_log("draw plant\n");
    gfx_spr(ctx, tr, pos, 0, 0);
}
