// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "boss/boss_a.h"
#include "game.h"

void boss_a_tendril_on_hooked(g_s *g, obj_s *o, i32 hooked);

obj_s *boss_a_tendril_create(g_s *g, boss_a_s *b, v2_i32 p_anchor, i32 x_sign)
{
    obj_s            *o  = obj_create(g);
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;
    o->ID                = OBJID_BOSS_A_TENDRIL;
    td->x_sign           = x_sign;
    td->b                = b;

    v2_i32 p_idle = {p_anchor.x - x_sign * 100, p_anchor.y + 50};
    td->p_anchor  = p_anchor;
    td->p_idle    = p_idle;

    for (i32 n = 0; n < BOSS_A_TENDRIL_NUM_SEGS; n++) {
        boss_a_segment_s *seg = &td->segs[n];

        v2_i32 p  = v2_i32_lerp(v2_i32_shl(td->p_anchor, 8), v2_i32_shl(td->p_idle, 8),
                                n, BOSS_A_TENDRIL_NUM_SEGS - 1);
        seg->p_q8 = p;
    }

    o->w     = 32;
    o->h     = 32;
    o->pos.x = p_idle.x - (o->w / 2);
    o->pos.y = p_idle.y - (o->h / 2);
    o->flags = OBJ_FLAG_ACTOR |
               // OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_HOOKABLE;
    o->on_hook = boss_a_tendril_on_hooked;
    return o;
}

void boss_a_tendril_update(g_s *g, obj_s *o)
{
    if (!o) return;
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;

    if (o->state == BOSS_A_TENDRIL_ST_HIDDEN) {
        td->show_q8 = max_i32(td->show_q8 - 10, 0);
    } else {
        td->show_q8 = min_i32(td->show_q8 + 10, 256);
    }

    switch (o->state) {
    case 0: break;
    case BOSS_A_TENDRIL_ST_HOOKED: {
        obj_v_q8_mul(o, 250, 250);
        v2_i32 fr = {0};
        i32    f  = grapplinghook_f_at_obj_proj_v(&g->ghook, o, (v2_i32){0}, &fr);
        v2_i32 pr = wirenode_vec(o->wire, o->wirenode);

        v2_i32 pc   = obj_pos_center(o);
        pc          = v2_i32_shl(pc, 4);
        v2_i32 vdt  = v2_i32_sub(v2_i32_shl(td->p_idle, 4), pc);
        i32    vlen = v2_i32_len_appr(vdt);
        i32    ff   = (vlen * 800) >> 8;
        if (10 <= ff) {
            v2_i32 f2 = v2_i32_setlen(vdt, ff);
            pltf_log("%i | %i %i\n", ff, f2.x, f2.y);
            o->v_q12.x += f2.x;
            o->v_q12.y += f2.y;
        }
        v2_i32 pc_q12 = v2_i32_shl(v2_i32_sub(td->p_idle, obj_pos_center(o)), 12);
        v2_i32 d_q12  = v2_i32_sub(pc_q12, o->v_q12);
        d_q12         = v2_i32_shr(d_q12, 7);
        // o->v_q12.x += d_q12.x;
        // o->v_q12.y += d_q12.y;
        o->v_q12.x += fr.x >> 0;
        o->v_q12.y += fr.y >> 0;

        obj_move_by_v_q12(g, o);

        break;
    }
    case BOSS_A_TENDRIL_ST_IDLE: {
        o->timer++;
        v2_i32 pc     = obj_pos_center(o);
        v2_i32 pc_q12 = v2_i32_shl(pc, 12);
        pc_q12.x += o->subpos_q12.x;
        pc_q12.y += o->subpos_q12.y;
        v2_i32 pt = {td->p_idle.x << 12, td->p_idle.y << 12};

        i32    mul    = 30;
        v2_i32 dt_q12 = v2_i32_sub(pt, pc_q12);
        v2_i32 dt_mov = v2_i32_mul_ratio(dt_q12, mul, 256);
        dt_mov        = v2_i32_truncatel(dt_mov, 10000);

        if (v2_i32_len_appr(dt_mov) < 8) {
            obj_move(g, o, td->p_idle.x - pc.x, td->p_idle.y - pc.y);
            o->subpos_q12.x = 0;
            o->subpos_q12.y = 0;
        } else {
            obj_move_by_q12(g, o, dt_mov.x, dt_mov.y);
        }
        break;
    }
    case BOSS_A_TENDRIL_ST_SLASH_INIT: {
        o->timer++;
        i32    ticks_total = BOSS_A_TENDRIL_TICKS_SLASH_INIT;
        v2_i32 p_trg       = v2_i32_lerp(td->p_slash0, td->p_slash1, o->timer, ticks_total);
        boss_a_tendril_move_to_pos(g, o, p_trg);
        if (ticks_total <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_TENDRIL_ST_SLASH_ANTICIPATE: {
        o->timer++;

        if (BOSS_A_TENDRIL_TICKS_SLASH_ANTICIPATE <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_TENDRIL_ST_SLASH_EXE: {
        o->timer++;

        i32    ticks_total = BOSS_A_TENDRIL_TICKS_SLASH_EXE;
        v2_i32 p_trg       = v2_i32_lerp(td->p_slash1, td->p_slash2, o->timer, ticks_total);
        boss_a_tendril_move_to_pos(g, o, p_trg);
        if (ticks_total <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_TENDRIL_ST_SLASH_WAIT: {
        o->timer++;

        if (BOSS_A_TENDRIL_TICKS_SLASH_WAIT <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_TENDRIL_ST_SLASH_RETURN: {
        o->timer++;

        i32 ticks_total = BOSS_A_TENDRIL_TICKS_SLASH_RETURN;
        // v2_i32 p_trg       = v2_i32_lerp(td->p_slash2, td->p_idle, o->timer, ticks_total);
        // boss_a_tendril_move_to_pos(g, o, p_trg);
        if (ticks_total <= o->timer) {
            o->state = BOSS_A_TENDRIL_ST_IDLE;
            o->timer = 0;
        }
        break;
    }
    }
}

void boss_a_tendril_animate(g_s *g, obj_s *o)
{
    if (!o) return;
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;
    td->head_bop_tick++;

    boss_a_segment_s *seg_tail = &td->segs[0];
    boss_a_segment_s *seg_head = &td->segs[BOSS_A_TENDRIL_NUM_SEGS - 1];

    v2_i32 p_head    = obj_pos_center(o);
    v2_i32 p_head_q8 = v2_i32_shl(p_head, 8);
    p_head_q8.x += ((1000 * sin_q15(td->head_bop_tick * 1000)) / 32768);
    p_head_q8.y += ((1000 * sin_q15(td->head_bop_tick * 800)) / 32768);
    seg_head->p_q8 = p_head_q8;

    i32 seg_len = 3000;
    if (o->state) {
        seg_len >>= 1;
    }
    for (i32 n = 1; n < BOSS_A_TENDRIL_NUM_SEGS - 1; n++) {
        boss_a_segment_s *seg = &td->segs[n];
        seg->p_q8.y -= 50;
        seg->p_q8.y += rngr_sym_i32(20);
        seg->p_q8.x += td->x_sign * 10;
    }

    boss_a_segments_constrain(td->segs, BOSS_A_TENDRIL_NUM_SEGS, 3, seg_len);
}

void boss_a_tendril_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    if (!o) return;

    boss_a_tendril_s *td  = (boss_a_tendril_s *)o->mem;
    gfx_ctx_s         ctx = gfx_ctx_from_tex(asset_tex(TEXID_DISPLAY_WHITE_OUTLINED));
    if (td->show_q8 == 0) return;

    v2_i32 pzero = {g->boss.boss_a.x_anchor + cam.x, g->boss.boss_a.y_anchor + cam.y};

    for (i32 n = 1; n < BOSS_A_TENDRIL_NUM_SEGS; n++) {
        boss_a_segment_s *s0 = &td->segs[n - 1];
        boss_a_segment_s *s1 = &td->segs[n];

        v2_i32 p0 = v2_i32_add(cam, v2_i32_shr(s0->p_q8, 8));
        v2_i32 p1 = v2_i32_add(cam, v2_i32_shr(s1->p_q8, 8));

        p0 = v2_i32_lerp(pzero, p0, td->show_q8, 256);
        p1 = v2_i32_lerp(pzero, p1, td->show_q8, 256);

        gfx_lin_thick(ctx, p0, p1, PRIM_MODE_BLACK, 8);
        gfx_cir_fill(ctx, p1, 12, PRIM_MODE_BLACK);
    }
    rec_i32 rr = obj_aabb(o);
    rr.x += cam.x;
    rr.y += cam.y;
    v2_i32 cp = obj_pos_center(o);
    cp        = v2_i32_add(cp, cam);

    cp = v2_i32_lerp(pzero, cp, td->show_q8, 256);
    gfx_cir_fill(ctx, cp, rr.w, PRIM_MODE_BLACK);
}

void boss_a_tendril_on_hooked(g_s *g, obj_s *o, i32 hooked)
{
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;

    switch (hooked) {
    case 0: {
        o->state = BOSS_A_TENDRIL_ST_IDLE;
        o->timer = 0;
        break;
    }
    default:
    case 1: {
        o->state = BOSS_A_TENDRIL_ST_HOOKED;
        o->timer = 0;
        break;
    }
    }
}

void boss_a_tendril_move_to_pos(g_s *g, obj_s *o, v2_i32 pos)
{
    v2_i32 dt = v2_i32_sub(pos, obj_pos_center(o));
    obj_move(g, o, dt.x, dt.y);
}

void boss_a_tendril_set_show(obj_s *o, i32 show)
{
    if (!o) return;

    switch (show) {
    case 0: {
        if (o->state == BOSS_A_TENDRIL_ST_HIDDEN) break;

        o->state = BOSS_A_TENDRIL_ST_HIDDEN;
        o->timer = 0;
        o->flags &= ~OBJ_FLAG_HOOKABLE;
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        break;
    }
    default:
    case 1: {
        if (o->state != BOSS_A_TENDRIL_ST_HIDDEN) break;

        o->state = BOSS_A_TENDRIL_ST_IDLE;
        o->timer = 0;
        o->flags |= OBJ_FLAG_HOOKABLE;
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        break;
    }
    }
}

void boss_a_tendril_slash(obj_s *o, v2_i32 p_from, v2_i32 p_to)
{

    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;

    o->timer        = 0;
    o->state        = BOSS_A_TENDRIL_ST_SLASH_INIT;
    o->v_q12.x      = 0;
    o->v_q12.y      = 0;
    o->subpos_q12.x = 0;
    o->subpos_q12.y = 0;
    td->p_slash0    = obj_pos_center(o);
    td->p_slash1    = p_from;
    td->p_slash2    = p_to;
}