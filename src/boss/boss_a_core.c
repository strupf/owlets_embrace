// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "boss/boss_a.h"
#include "game.h"

void boss_a_core_on_hit(g_s *g, obj_s *o, hitbox_res_s res);

void boss_a_core_on_hit(g_s *g, obj_s *o, hitbox_res_s res)
{
    boss_a_core_s *c = (boss_a_core_s *)o->heap;

    o->health = max_i32(o->health - res.damage, 0);

    if (o->health) {
        pltf_log("Boss health: %i\n", o->health);
    } else {
        // killed
        o->flags              = ~OBJ_FLAG_HURT_ON_TOUCH;
        o->on_hitbox          = 0;
        o->hitbox_flags_group = 0;
        o->state              = BOSS_A_CORE_ST_DYING;
        o->timer              = 0;
        boss_a_on_killed_core(g, &g->boss.boss_a);
    }
    c->hurt_tick = 1;
}

obj_s *boss_a_core_create(g_s *g, boss_a_s *b, v2_i32 p_anchor)
{
    obj_s *o              = obj_create(g);
    o->heap               = game_alloc_roomt(g, boss_a_core_s);
    boss_a_core_s *c      = (boss_a_core_s *)o->heap;
    c->b                  = b;
    o->ID                 = 4000;
    o->w                  = 32;
    o->h                  = 32;
    // o->flags              = OBJ_FLAG_HURT_ON_TOUCH;
    o->hitbox_flags_group = HITBOX_FLAG_GROUP_ENEMY | HITBOX_FLAG_GROUP_TRIGGERS_CALLBACK;
    o->on_hitbox          = boss_a_core_on_hit;
    o->health_max         = 2;
    o->health             = o->health_max;
    v2_i32 p_center       = {p_anchor.x, p_anchor.y + 100};
    o->pos.x              = p_center.x - (o->w >> 1);
    o->pos.y              = p_center.y - (o->h >> 1);
    c->p_anchor           = p_anchor;
    c->p_head             = p_center;
    c->p_idle             = p_center;
    c->p_dst              = p_center;

    i32 x_leg_attachments[3] = {-60, +20, +60};
    for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
        boss_a_core_leg_s *leg = &c->legs[n];

        v2_i32 p_attach_ceil = {p_anchor.x + x_leg_attachments[n], p_anchor.y};
        v2_i32 p0_q8         = v2_i32_shl(p_attach_ceil, 8);
        v2_i32 p1_q8         = v2_i32_shl(p_center, 8);

        for (i32 i = 0; i < BOSS_A_CORE_NUM_SEGS_PER_LEG; i++) {
            boss_a_segment_s *seg = &leg->segs[i];

            v2_i32 p_q8 = v2_i32_lerp(p0_q8, p1_q8, i, BOSS_A_CORE_NUM_SEGS_PER_LEG - 1);
            seg->p_q8   = p_q8;
            seg->pp_q8  = p_q8;
        }
    }
    boss_a_core_constrain_legs(g, o);
    return o;
}

void boss_a_core_on_update(g_s *g, obj_s *o)
{
    if (!o) return;

    obj_s *o_owl = obj_get_owl(g);
    if (o_owl) {
        v2_i32 pc_owl = obj_pos_center(o_owl);
    }

    boss_a_core_s *c = (boss_a_core_s *)o->heap;

    if (o->state == BOSS_A_CORE_ST_HIDDEN) {
        c->show_q8 = max_i32(c->show_q8 - 10, 0);
    } else {
        c->show_q8 = min_i32(c->show_q8 + 10, 256);
    }

    if (o->state != BOSS_A_CORE_ST_HIDDEN) {
        for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
            boss_a_core_leg_s *leg = &c->legs[n];

            // leg->l_q8 = min_i32(leg->l_q8 + 256, BOSS_A_CORE_LEG_LEN_Q8);
        }
    }

    switch (o->state) {
    case BOSS_A_CORE_ST_HIDDEN: {
        for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
            boss_a_core_leg_s *leg = &c->legs[n];

            //  leg->l_q8 = max_i32(512, leg->l_q8 - 256);
        }

        // c->p_idle = v2_i32_lerp(c->p_idle, c->p_anchor, 1, 8);

        // c->p_head.x = c->p_idle.x + (5 * sin_q15(c->bop_tick * 1000)) / 65536;
        //  c->p_head.y = c->p_idle.y + (5 * sin_q15(c->bop_tick * 1200)) / 65536;
        //  boss_a_core_move_to_center(g, o);
        break;
    }
    case BOSS_A_CORE_ST_IDLE: {
        o->timer++;
        c->bop_tick++;

        if (o->timer == 100) {
            o->timer   = 0;
            c->p_dst.x = c->p_anchor.x + rngr_i32(-100, +100);
            c->p_dst.y = c->p_anchor.y + rngr_i32(+80, +180);
        }

        c->p_idle = v2_i32_lerp(c->p_idle, c->p_dst, 1, 8);

        c->p_head.x = c->p_idle.x + (5 * sin_q15(c->bop_tick * 1000)) / 65536;
        c->p_head.y = c->p_idle.y + (5 * sin_q15(c->bop_tick * 1200)) / 65536;
        boss_a_core_move_to_center(g, o);
        break;
    }
    case BOSS_A_CORE_ST_STOMP_INIT: {
        o->timer++;
        if (BOSS_A_CORE_TICKS_STOMP_INIT <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_CORE_ST_STOMP_TARGETING: {
        o->timer++;
        if (BOSS_A_CORE_TICKS_STOMP_TARGETING <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_CORE_ST_STOMP_ANTICIPATE: {
        o->timer++;
        if (BOSS_A_CORE_TICKS_STOMP_ANTICIPATE <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_CORE_ST_STOMP_EXE: {
        o->timer++;
        if (BOSS_A_CORE_TICKS_STOMP_EXE <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_CORE_ST_STOMP_CRASHED: {
        o->timer++;
        if (BOSS_A_CORE_TICKS_STOMP_CRASHED <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_CORE_ST_STOMP_RECOVER: {
        o->timer++;
        if (BOSS_A_CORE_TICKS_STOMP_RECOVER <= o->timer) {
            o->state = BOSS_A_CORE_ST_IDLE;
            o->timer = 0;
        }
        break;
    }
    }
}

void boss_a_core_move_to_center(g_s *g, obj_s *o)
{
    boss_a_core_s *c  = (boss_a_core_s *)o->heap;
    v2_i32         pc = obj_pos_center(o);
    v2_i32         dt = v2_i32_sub(c->p_head, pc);
    obj_move(g, o, dt.x, dt.y);
}

void boss_a_core_constrain_legs(g_s *g, obj_s *o)
{
    boss_a_core_s *c  = (boss_a_core_s *)o->heap;
    v2_i32         pc = obj_pos_center(o);

    for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
        boss_a_core_leg_s *leg = &c->legs[n];

        i32               l_seg    = leg->l_q8 / (BOSS_A_CORE_NUM_SEGS_PER_LEG - 1);
        boss_a_segment_s *leg_head = &leg->segs[BOSS_A_CORE_NUM_SEGS_PER_LEG - 1];
        leg_head->p_q8.x           = pc.x << 8;
        leg_head->p_q8.y           = pc.y << 8;

        for (i32 k = 1; k < BOSS_A_CORE_NUM_SEGS_PER_LEG - 1; k++) {
            boss_a_segment_s *s = &leg->segs[k];
            s->p_q8.y += 500;
        }
        boss_a_segments_constrain(leg->segs, BOSS_A_CORE_NUM_SEGS_PER_LEG, 5, l_seg);
    }
}

void boss_a_core_on_animate(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_a_core_s *c = (boss_a_core_s *)o->heap;

    if (c->hurt_tick) {
        c->hurt_tick++;
        if (ENEMY_HIT_FREEZE_TICKS <= c->hurt_tick) {
            c->hurt_tick = 0;
        }
    }
    boss_a_core_constrain_legs(g, o);
}

void boss_a_core_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    if (!o) return;

    boss_a_core_s *c = (boss_a_core_s *)o->heap;
    if (c->show_q8 == 0) return;

    v2_i32    p_core = v2_i32_add(obj_pos_center(o), cam);
    gfx_ctx_s ctx    = gfx_ctx_from_tex(asset_tex(TEXID_DISPLAY_WHITE_OUTLINED));
    texrec_s  trcore = asset_texrec(TEXID_BOSS, 544, 32 * 4, 32 * 4, 32 * 4);
    v2_i32    pzero  = {g->boss.boss_a.x_anchor + cam.x, g->boss.boss_a.y_anchor + cam.y};

    for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
        boss_a_core_leg_s *leg = &c->legs[n];

        for (i32 i = 1; i < BOSS_A_CORE_NUM_SEGS_PER_LEG; i++) {
            boss_a_segment_s *s0 = &leg->segs[i - 1];
            boss_a_segment_s *s1 = &leg->segs[i];

            v2_i32 p0 = v2_i32_add(v2_i32_shr(s0->p_q8, 8), cam);
            v2_i32 p1 = v2_i32_add(v2_i32_shr(s1->p_q8, 8), cam);

            p0.y = lerp_i32(pzero.y, p0.y, c->show_q8, 256);
            p1.y = lerp_i32(pzero.y, p1.y, c->show_q8, 256);

            gfx_lin_thick(ctx, p0, p1, PRIM_MODE_BLACK, 16);
            gfx_cir_fill(ctx, p1, 18, PRIM_MODE_BLACK);
        }
    }

    p_core            = v2_i32_lerp(pzero, p_core, c->show_q8, 256);
    // gfx_cir_fill(ctx, p_core, 40, PRIM_MODE_BLACK);
    v2_i32 p_core_spr = {
        p_core.x - trcore.w / 2,
        p_core.y - trcore.h / 2 + 32};
    // gfx_spr(ctx, trcore, p_core_spr, 0, 0);

    i32 k = (c->hurt_tick && c->hurt_tick < ENEMY_HIT_FLASH_TICKS + 1 ? PRIM_MODE_BLACK : PRIM_MODE_WHITE);
    gfx_cir_fill(ctx, p_core, o->w, k);
}

void boss_a_core_set_show(obj_s *o, i32 show)
{
    boss_a_core_s *c = (boss_a_core_s *)o->heap;

    switch (show) {
    case 0: {
        if (o->state == BOSS_A_CORE_ST_HIDDEN) break;

        o->state = BOSS_A_CORE_ST_HIDDEN;
        o->timer = 0;
        o->flags &= ~OBJ_FLAG_HOOKABLE;
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        break;
    }
    default:
    case 1: {
        if (o->state != BOSS_A_CORE_ST_HIDDEN) break;

        o->state = BOSS_A_CORE_ST_HIDDEN;
        o->timer = 0;
        o->flags |= OBJ_FLAG_HOOKABLE;
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        break;
    }
    }
}

void boss_a_core_move_to_pos(g_s *g, obj_s *o, v2_i32 pos)
{
    v2_i32 dt = v2_i32_sub(pos, obj_pos_center(o));
    obj_move(g, o, dt.x, dt.y);
}