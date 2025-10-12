// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss/boss_a.h"
#include "app.h"
#include "game.h"

obj_s *boss_a_tendril_create(g_s *g);
void   boss_a_tendril_update(g_s *g, obj_s *o);
void   boss_a_tendril_animate(g_s *g, obj_s *o);
void   boss_a_tendril_draw(g_s *g, obj_s *o, v2_i32 cam);
void   boss_a_tendril_setup(obj_s *o, v2_i32 p_anchor, v2_i32 p_head);
void   boss_a_tendril_move_to_head_pos(g_s *g, obj_s *o);

void boss_a_segments_constrain(boss_a_segment_s *segs, i32 num, i32 loops, i32 l);

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
            // cam_clamp_rec_set(g, obj_aabb(o_cam_rec));
        }
        // g->cam.handler_f = boss_a_cam_handler;
    }
    tex_from_wad_ID(TEXID_BOSS, "T_BOSS_A", game_allocator_room(g));
}

void boss_a_update(g_s *g, boss_a_s *b)
{
    b->phase_tick++;

    switch (b->phase) {
    default: break;
    case BOSS_A_INTRO_0: {
        obj_s *o_tendril = obj_from_handle(b->o_tendrils[0]);
        if (o_tendril) {
            boss_a_tendril_update(g, o_tendril);
            boss_a_tendril_animate(g, o_tendril);
        }

        if (50 == b->phase_tick) {
            game_on_trigger(g, 10);
        }

        i32 tt = b->phase_tick % 200;
        if (tt == 200 / 2) {
            boss_a_core_hide(b->o_core);
        }
        if (tt == 0) {
            boss_a_core_show(b->o_core);
        }
        if (1000 <= b->phase_tick) {
            b->phase      = BOSS_A_P1_IDLE;
            b->phase_tick = 0;
#if 0
            b->phase      = BOSS_A_P1_DEFEATED;
            b->phase_tick = 0;
            game_on_trigger(g, 1);
            saveID_put(g, SAVEID_BOSS_PLANT);
            cam_clamp_rec_unset(g);
            background_fade_to(g, 0, 150);
#endif
        }
        break;
    }
    }

    boss_a_core_on_update(g, b->o_core);
}

void boss_a_animate(g_s *g, boss_a_s *b)
{
    obj_s *o_tendril = obj_from_handle(b->o_tendrils[0]);
    if (o_tendril) {
        boss_a_tendril_animate(g, o_tendril);
    }

    boss_a_core_on_animate(g, b->o_core);
    boss_a_plant_animate(g, b);
}

void boss_a_draw(g_s *g, boss_a_s *b, v2_i32 cam)
{
    gfx_ctx_s ctx   = gfx_ctx_from_tex(asset_tex(TEXID_DISPLAY_WHITE_OUTLINED));
    tex_s     tboss = asset_tex(TEXID_BOSS);

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

    boss_a_s *b   = &g->boss.boss_a;
    b->phase      = BOSS_A_INTRO_0;
    b->phase_tick = 0;
    game_on_trigger(g, 1);
    background_fade_to(g, -64, 150);
    obj_s *o_cam_rec = obj_find_ID_subID(g, OBJID_CAM_REC, 2, 0);
    if (o_cam_rec) {
        // cam_clamp_rec_set(g, obj_aabb(o_cam_rec));
    }
    obj_s *o_tendril = boss_a_tendril_create(g);
    boss_a_tendril_setup(o_tendril, (v2_i32){b->x_anchor, b->y_anchor}, (v2_i32){b->x_anchor - 100, b->y_anchor + 100});
    b->o_tendrils[0] = handle_from_obj(o_tendril);

    b->o_core = boss_a_core_create(g, (v2_i32){b->x_anchor, b->y_anchor});
}

void boss_a_p2_init(g_s *g, boss_a_s *b)
{
    b->phase      = BOSS_A_P1_TO_P2;
    b->phase_tick = 0;
    game_on_trigger(g, 10);
}

void boss_a_tendril_update(g_s *g, obj_s *o)
{
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;

    switch (o->state) {
    case BOSS_A_TENDRIL_ST_IDLE: {
        o->timer++;

        td->p_head = v2_i32_lerp(td->p_head, td->p_idle, 1, 8);

        if (50 <= o->timer) {
            o->timer           = 0;
            o->state           = BOSS_A_TENDRIL_ST_SLASH_INIT;
            td->p_slash_from.x = td->p_idle.x - 50;
            td->p_slash_from.y = td->p_idle.y + 50;
            td->p_slash_to.x   = td->p_slash_from.x + 200;
            td->p_slash_to.y   = td->p_slash_from.y;
        }
        break;
    }
    case BOSS_A_TENDRIL_ST_SLASH_INIT: {
        o->timer++;

        td->p_head = v2_i32_lerp(td->p_head, td->p_slash_from, 1, 2);
        if (BOSS_A_TENDRIL_TICKS_SLASH_INIT <= o->timer) {
            td->p_head = td->p_slash_from;
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

        v2_i32 p_headp = td->p_head;
        td->p_head.x   = ease_out_quad(td->p_slash_from.x, td->p_slash_to.x, o->timer, BOSS_A_TENDRIL_TICKS_SLASH_EXE);
        td->p_head.y   = ease_out_quad(td->p_slash_from.y, td->p_slash_to.y, o->timer, BOSS_A_TENDRIL_TICKS_SLASH_EXE);

        // hitbox_s hb = hitbox_gen(g);
        //  hitbox_set_lin(&hb, p_headp.x, p_headp.y, td->p_head.x, td->p_head.y);
        //   hitbox_to_this_frame(g, hb);

        if (BOSS_A_TENDRIL_TICKS_SLASH_EXE <= o->timer) {
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_A_TENDRIL_ST_SLASH_WAIT: {
        o->timer++;

        if (BOSS_A_TENDRIL_TICKS_SLASH_WAIT <= o->timer) {
            o->state = BOSS_A_TENDRIL_ST_IDLE;
            o->timer = 0;
        }
        break;
    }
    }

    boss_a_tendril_move_to_head_pos(g, o);
}

void boss_a_tendril_animate(g_s *g, obj_s *o)
{
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;
    td->head_bop_tick++;

    boss_a_segment_s *seg_tail = &td->segs[0];
    boss_a_segment_s *seg_head = &td->segs[BOSS_A_TENDRIL_NUM_SEGS - 1];

    v2_i32 p_head_q8 = v2_i32_shl(td->p_head, 8);
    p_head_q8.x += ((1000 * sin_q15(td->head_bop_tick * 1000)) / 32768);
    p_head_q8.y += ((1000 * sin_q15(td->head_bop_tick * 800)) / 32768);
    seg_head->p_q8 = p_head_q8;

    for (i32 n = 1; n < BOSS_A_TENDRIL_NUM_SEGS - 1; n++) {
        boss_a_segment_s *seg = &td->segs[n];
        seg->p_q8.y -= 50;
        seg->p_q8.y += rngr_sym_i32(20);
        seg->p_q8.x -= 10;
    }

    i32 seg_len = 3000;
    if (o->state) {
        seg_len >>= 1;
    }
    // boss_a_tendril_segs_constrain(td->segs, BOSS_A_TENDRIL_NUM_SEGS, seg_len);
}

void boss_a_tendril_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    boss_a_tendril_s *td  = (boss_a_tendril_s *)o->mem;
    gfx_ctx_s         ctx = gfx_ctx_from_tex(asset_tex(TEXID_DISPLAY_WHITE_OUTLINED));

    for (i32 n = 1; n < BOSS_A_TENDRIL_NUM_SEGS; n++) {
        boss_a_segment_s *s0 = &td->segs[n - 1];
        boss_a_segment_s *s1 = &td->segs[n];

        v2_i32 p0 = v2_i32_add(cam, v2_i32_shr(s0->p_q8, 8));
        v2_i32 p1 = v2_i32_add(cam, v2_i32_shr(s1->p_q8, 8));

        gfx_lin_thick(ctx, p0, p1, PRIM_MODE_BLACK, 8);
        gfx_cir_fill(ctx, p1, 12, PRIM_MODE_BLACK);
    }
    rec_i32 rr = obj_aabb(o);
    rr.x += cam.x;
    rr.y += cam.y;

    gfx_rec_fill(ctx, rr, PRIM_MODE_BLACK);
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

obj_s *boss_a_tendril_create(g_s *g)
{
    obj_s            *o  = obj_create(g);
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;

    o->ID    = 300;
    o->w     = 32;
    o->h     = 32;
    o->flags = OBJ_FLAG_ACTOR |
               OBJ_FLAG_HURT_ON_TOUCH |
               OBJ_FLAG_HOOKABLE;
    o->on_hook = boss_a_tendril_on_hooked;
    return o;
}

void boss_a_tendril_setup(obj_s *o, v2_i32 p_anchor, v2_i32 p_head)
{
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;

    td->p_anchor.x = p_anchor.x;
    td->p_anchor.y = p_anchor.y;
    td->p_head.x   = p_head.x;
    td->p_head.y   = p_head.y;
    td->p_idle     = td->p_head;

    for (i32 n = 0; n < BOSS_A_TENDRIL_NUM_SEGS; n++) {
        boss_a_segment_s *seg = &td->segs[n];

        v2_i32 p  = v2_i32_lerp(v2_i32_shl(td->p_anchor, 8), v2_i32_shl(td->p_head, 8),
                                n, BOSS_A_TENDRIL_NUM_SEGS - 1);
        seg->p_q8 = p;
    }
    o->pos.x = p_head.x - (o->w / 2);
    o->pos.y = p_head.y - (o->h / 2);
}

void boss_a_tendril_move_to_head_pos(g_s *g, obj_s *o)
{
    boss_a_tendril_s *td = (boss_a_tendril_s *)o->mem;
    v2_i32            dt = v2_i32_sub(td->p_head, obj_pos_center(o));
    obj_move(g, o, dt.x, dt.y);
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
    gfx_spr(ctx, tr, pos, 0, 0);
}

obj_s *boss_a_core_create(g_s *g, v2_i32 p_anchor)
{
    obj_s *o         = obj_create(g);
    o->heap          = game_alloc_roomt(g, boss_a_core_s);
    boss_a_core_s *c = (boss_a_core_s *)o->heap;
    o->ID            = 4000;
    o->w             = 32;
    o->h             = 32;
    v2_i32 p_center  = {p_anchor.x, p_anchor.y + 100};
    o->pos.x         = p_center.x - (o->w >> 1);
    o->pos.y         = p_center.y - (o->h >> 1);
    c->p_anchor      = p_anchor;
    c->p_head        = p_center;
    c->p_idle        = p_center;
    c->p_dst         = p_center;

    i32 x_leg_attachments[3] = {-100, +20, +100};
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

    if (o->state != BOSS_A_CORE_ST_HIDDEN) {
        for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
            boss_a_core_leg_s *leg = &c->legs[n];

            leg->l_q8 = min_i32(leg->l_q8 + 256, BOSS_A_CORE_LEG_LEN_Q8);
        }
    }

    switch (o->state) {
    case BOSS_A_CORE_ST_HIDDEN: {
        for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
            boss_a_core_leg_s *leg = &c->legs[n];

            leg->l_q8 = max_i32(512, leg->l_q8 - 256);
        }

        c->p_idle = v2_i32_lerp(c->p_idle, c->p_anchor, 1, 8);

        c->p_head.x = c->p_idle.x + (5 * sin_q15(c->bop_tick * 1000)) / 65536;
        c->p_head.y = c->p_idle.y + (5 * sin_q15(c->bop_tick * 1200)) / 65536;
        boss_a_core_move_to_center(g, o);
        break;
    }
    case BOSS_A_CORE_ST_IDLE: {
        o->timer++;
        c->bop_tick++;

        if (o->timer == 100) {
            o->timer   = 0;
            c->p_dst.x = c->p_anchor.x + rngr_i32(-100, +100);
            c->p_dst.y = c->p_anchor.y + rngr_i32(+30, +150);
        }

        c->p_idle = v2_i32_lerp(c->p_idle, c->p_dst, 1, 8);

        c->p_head.x = c->p_idle.x + (5 * sin_q15(c->bop_tick * 1000)) / 65536;
        c->p_head.y = c->p_idle.y + (5 * sin_q15(c->bop_tick * 1200)) / 65536;
        boss_a_core_move_to_center(g, o);
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

        for (i32 i = 0; i < 5; i++) {
            for (i32 k = 1; k < BOSS_A_CORE_NUM_SEGS_PER_LEG; k++) {
                boss_a_segment_s *s0 = &leg->segs[k - 1];
                boss_a_segment_s *s1 = &leg->segs[k];

                v2_i32 p0  = s0->p_q8;
                v2_i32 p1  = s1->p_q8;
                v2_i32 dt  = v2_i32_sub(p1, p0);
                i32    len = v2_i32_len_appr(dt);
                if (len <= l_seg) continue;

                i32    new_l = l_seg + ((len - l_seg) >> 1);
                v2_i32 vadd  = v2_i32_setlenl(dt, len, new_l);

                if (1 < k) {
                    s0->p_q8 = v2_i32_sub(p1, vadd);
                }
                if (k < BOSS_A_CORE_NUM_SEGS_PER_LEG - 1) {
                    s1->p_q8 = v2_i32_add(p0, vadd);
                }
            }
        }
    }
}

void boss_a_core_on_animate(g_s *g, obj_s *o)
{
    if (!o) return;
    boss_a_core_s *c = (boss_a_core_s *)o->heap;
    boss_a_core_constrain_legs(g, o);
}

void boss_a_core_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    if (!o) return;
    boss_a_core_s *c      = (boss_a_core_s *)o->heap;
    v2_i32         p_core = v2_i32_add(obj_pos_center(o), cam);
    gfx_ctx_s      ctx    = gfx_ctx_from_tex(asset_tex(TEXID_DISPLAY_WHITE_OUTLINED));
    texrec_s       trcore = asset_texrec(TEXID_BOSS, 544, 32 * 4, 32 * 4, 32 * 4);

    for (i32 n = 0; n < BOSS_A_CORE_NUM_LEGS; n++) {
        boss_a_core_leg_s *leg = &c->legs[n];

        for (i32 i = 1; i < BOSS_A_CORE_NUM_SEGS_PER_LEG; i++) {
            boss_a_segment_s *s0 = &leg->segs[i - 1];
            boss_a_segment_s *s1 = &leg->segs[i];

            v2_i32 p0 = v2_i32_add(v2_i32_shr(s0->p_q8, 8), cam);
            v2_i32 p1 = v2_i32_add(v2_i32_shr(s1->p_q8, 8), cam);
            gfx_lin_thick(ctx, p0, p1, PRIM_MODE_BLACK, 16);
            gfx_cir_fill(ctx, p1, 18, PRIM_MODE_BLACK);
        }
    }

    // gfx_cir_fill(ctx, p_core, 40, PRIM_MODE_BLACK);
    v2_i32 p_core_spr = {
        p_core.x - trcore.w / 2,
        p_core.y - trcore.h / 2 + 32};
    gfx_spr(ctx, trcore, p_core_spr, 0, 0);
}

void boss_a_core_show(obj_s *o)
{
    boss_a_core_s *c = (boss_a_core_s *)o->heap;
    assert(o->state == BOSS_A_CORE_ST_HIDDEN);
    o->state = BOSS_A_CORE_ST_IDLE;
    o->flags |= OBJ_FLAG_HURT_ON_TOUCH |
                OBJ_FLAG_ACTOR |
                OBJ_FLAG_HOOKABLE;
    pltf_log("SHOW\n");
}

void boss_a_core_hide(obj_s *o)
{
    boss_a_core_s *c = (boss_a_core_s *)o->heap;
    assert(o->state != BOSS_A_CORE_ST_HIDDEN);
    o->state = BOSS_A_CORE_ST_HIDDEN;
    o->flags &= ~(OBJ_FLAG_HURT_ON_TOUCH |
                  OBJ_FLAG_ACTOR |
                  OBJ_FLAG_HOOKABLE);
    pltf_log("HIDE\n");
}