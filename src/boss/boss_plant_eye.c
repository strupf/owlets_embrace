// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss_plant.h"
#include "game.h"

#define BPLANT_EYE_ANCHOR_Y     48
#define BPLANT_EYE_OFFS_IDLE_Y  60
#define BPLANT_EYE_ATTACK_TICKS 50
#define BPLANT_EYE_SLASH_X      140 // from - to + horizontally

enum {
    BOSS_PLANT_EYE_HIDDEN,
    BOSS_PLANT_EYE_SHOWN,
    BOSS_PLANT_EYE_HOOKED,
    BOSS_PLANT_EYE_ATTACK,
    BOSS_PLANT_EYE_ATTACK_EXE,
};

typedef struct boss_plant_eye_s {
    b8     active_tear;
    v2_i32 pos_src;
    v2_i32 jank_pos;
    i16    anchor_x;
    i16    anchor_y;
    i16    shown_lerp_q8;
    u16    hooked_cd;
    u16    n_segs;
    u16    tear_off_tick;
    u16    tear_off_tick_needed;
    i16    slash_y_slot;
    i16    slash_sig;
    i16    jank_tick;

    bplant_seg_s segs[20];
} boss_plant_eye_s;

static_assert(sizeof(boss_plant_eye_s) <= OBJ_MEM_BYTES, "Size");

void boss_plant_eye_hide(g_s *g, obj_s *o);
void boss_plant_eye_show(g_s *g, obj_s *o);
void boss_plant_eye_show_nohook(g_s *g, obj_s *o);
void boss_plant_eye_on_update(g_s *g, obj_s *o);
void boss_plant_eye_on_animate(g_s *g, obj_s *o);
void boss_plant_eye_on_hook(g_s *g, obj_s *o, b32 hooked);
void boss_plant_eye_on_update(g_s *g, obj_s *o);
void boss_plant_eye_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
i32  boss_plant_eye_slash_y(obj_s *o);

i32 boss_plant_eye_slash_y(obj_s *o)
{
    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    return (48 + e->slash_y_slot * 16);
}

static i32 boss_plant_eye_dist_seg(obj_s *o)
{
    return (o->ID == OBJID_BOSS_PLANT_EYE ? 8 : 6);
}

v2_i32 boss_plant_eye_idle_bop_offs(obj_s *o)
{
    v2_i32 p = {((sin_q15(o->animation * 1150) * 4) / 32769),
                ((cos_q15(o->animation * 2000) * 4) / 32769)};
    return p;
}

obj_s *boss_plant_eye_create(g_s *g, i32 ID)
{
    boss_plant_s     *bp = &g->boss.plant;
    obj_s            *o  = obj_create(g);
    boss_plant_eye_s *e  = (boss_plant_eye_s *)o->mem;
    o->heap              = bp;
    o->ID                = ID;
    o->animation         = rngr_i32(0, 100);
    o->on_update         = boss_plant_eye_on_update;
    o->on_animate        = boss_plant_eye_on_animate;
    o->on_hook           = boss_plant_eye_on_hook;

    v2_i32 panchor = {bp->x, bp->y};

    switch (ID) {
    case OBJID_BOSS_PLANT_EYE: {
        panchor.y += BPLANT_EYE_ANCHOR_Y;
        e->anchor_y             = 40;
        e->n_segs               = 10;
        e->tear_off_tick_needed = 100;
        o->w                    = 48;
        o->h                    = 48;
        break;
    }
    case OBJID_BOSS_PLANT_EYE_FAKE_L:
    case OBJID_BOSS_PLANT_EYE_FAKE_R: {
        e->n_segs               = 20;
        e->anchor_x             = ID == OBJID_BOSS_PLANT_EYE_FAKE_R ? +70 : -70;
        e->anchor_y             = 60;
        e->tear_off_tick_needed = 200;
        o->w                    = 28;
        o->h                    = 28;
        break;
    }
    }

    o->pos.x  = panchor.x - o->w / 2 + e->anchor_x;
    o->pos.y  = panchor.y - o->h / 2 + e->anchor_y;
    v2_i32 p1 = v2_i32_shl(v2_i32_sub(obj_pos_center(o), panchor), 8);

    for (i32 n = 0; n < e->n_segs; n++) {
        bplant_seg_s *seg = &e->segs[n];
        seg->p_q8         = v2_i32_lerp((v2_i32){0}, p1,
                                        n, e->n_segs - 1);
    }
    return o;
}

void boss_plant_eye_move_to_centerpx(g_s *g, obj_s *o, i32 x, i32 y)
{
    v2_i32 pcenter = obj_pos_center(o);
    i32    dx      = (x - o->w / 2) - o->pos.x;
    i32    dy      = (y - o->h / 2) - o->pos.y;
    obj_move(g, o, dx, dy);
    o->subpos_q8.x = 0;
    o->subpos_q8.y = 0;
    o->v_q8.x      = 0;
    o->v_q8.y      = 0;
}

void boss_plant_eye_on_update(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_plant_eye_s *e             = (boss_plant_eye_s *)o->mem;
    boss_plant_s     *bp            = (boss_plant_s *)o->heap;
    v2_i32            panchor       = {bp->x, bp->y + BPLANT_EYE_ANCHOR_Y};
    i32               pulling_force = 0;
    i32               dseg          = boss_plant_eye_dist_seg(o);
    obj_s            *ohero         = obj_get_hero(g);
    v2_i32            hcenter       = obj_pos_center(ohero);
    v2_i32            pcenter       = obj_pos_center(o);
    o->timer++;

    if (o->state == BOSS_PLANT_EYE_HIDDEN) {
        e->shown_lerp_q8 = max_i32(e->shown_lerp_q8 - 20, 0);
    } else {
        e->shown_lerp_q8 = min_i32(e->shown_lerp_q8 + 30, 256);
    }

    if (o->state != BOSS_PLANT_EYE_HOOKED) {
        e->hooked_cd     = 0;
        e->tear_off_tick = 0;
    }

    e->active_tear        = 0;
    bool32 back_to_anchor = 1;
    bool32 truncate_len   = 0;

    switch (o->state) {
    case BOSS_PLANT_EYE_SHOWN: {
        break;
    }
    case BOSS_PLANT_EYE_HIDDEN: {
        break;
    }
    case BOSS_PLANT_EYE_ATTACK: {
        back_to_anchor    = 0;
        truncate_len      = 0;
        v2_i32 attack_pos = {panchor.x + e->slash_sig * BPLANT_EYE_SLASH_X,
                             panchor.y + boss_plant_eye_slash_y(o)};
        i32    st         = min_i32(o->timer, o->subtimer - 15);
        v2_i32 plerp      = v2_i32_lerp(pcenter, attack_pos,
                                        st, o->subtimer - 15);
        obj_move(g, o, plerp.x - pcenter.x, plerp.y - pcenter.y);

        if (o->subtimer <= o->timer) {
            e->pos_src = attack_pos;
            o->state++;
            o->timer = 0;
        }
        break;
    }
    case BOSS_PLANT_EYE_ATTACK_EXE: {
        back_to_anchor    = 0;
        truncate_len      = 0;
        v2_i32 attack_pos = {panchor.x - e->slash_sig * BPLANT_EYE_SLASH_X,
                             panchor.y + boss_plant_eye_slash_y(o)};
        i32    st         = min_i32(o->timer, 30);
        v2_i32 plerp      = v2_i32_lerp(pcenter, attack_pos, o->timer, 30);
        obj_move(g, o, plerp.x - pcenter.x, plerp.y - pcenter.y);

        if (o->timer == 40) {
            if (o->substate == 1) {
                boss_plant_eye_try_attack(g, o, e->slash_y_slot + 2, -e->slash_sig, 0);
                o->substate = 2;
                o->subtimer = 25;
            } else {
                o->state = BOSS_PLANT_EYE_SHOWN;
                o->timer = 0;
            }
        }
        break;
    }
    case BOSS_PLANT_EYE_HOOKED: {
        truncate_len  = 1;
        pulling_force = grapplinghook_pulling_force_hero(g);

        if (e->jank_tick) {
            e->jank_tick++;
            v2_i32 plerp = v2_i32_lerp(pcenter, e->jank_pos, e->jank_tick, 30);
            obj_move(g, o, plerp.x - pcenter.x, plerp.y - pcenter.y);
            if (e->jank_tick == 30) {
                e->jank_tick = 0;
                o->subtimer  = 0;
            }
        } else {
            o->subtimer++;
            if (o->subtimer == 110) {
                e->pos_src = pcenter;

                e->jank_pos.y = panchor.y + 40;
                e->jank_tick  = 1;
                o->subtimer   = 0;

                if (panchor.x < pcenter.x) {
                    e->jank_pos.x = panchor.x - 100;
                } else {
                    e->jank_pos.x = panchor.x + 100;
                }
            }
        }

        if (pulling_force) {
            back_to_anchor = 0;
            e->tear_off_tick++;
            if (e->tear_off_tick_needed <= e->tear_off_tick) {
                e->tear_off_tick = e->tear_off_tick_needed;
                boss_plant_on_eye_tear_off(g, o);
                bp->sx_teared = pcenter.x < bp->x ? -1 : +1;
                grapplinghook_destroy(g, &g->ghook);
                obj_delete(g, o);
            }

            e->active_tear = 1;
            e->hooked_cd   = 20;
            v2_i32 dtr     = v2_i32_sub(hcenter, pcenter);
            v2_i32 steer   = steer_seek(pcenter, v2_i32_from_i16(o->v_q8), hcenter, 1200);
            o->v_q8.x += steer.x / 4;
            o->v_q8.y += steer.y / 4;
        } else if (e->hooked_cd) {
            back_to_anchor = 0;
            e->hooked_cd--;
        }
        break;
    }
    }

    if (back_to_anchor) {
        if (e->tear_off_tick) {
            e->tear_off_tick--;
        }
        v2_i32 goalpos = {panchor.x + e->anchor_x,
                          panchor.y + e->anchor_y};
        v2_i32 dtr     = v2_i32_sub(goalpos, pcenter);
        if (v2_i32_lensq(dtr) < 10) {
            o->v_q8.x = 0;
            o->v_q8.y = 0;
            boss_plant_eye_move_to_centerpx(g, o, goalpos.x, goalpos.y);
        } else {
            v2_i32 steer = steer_seek(pcenter, v2_i32_from_i16(o->v_q8), goalpos, 850);
            o->v_q8.x += steer.x / 4;
            o->v_q8.y += steer.y / 4;
        }
    }

    obj_move_by_v_q8(g, o);

    pcenter       = obj_pos_center(o);
    v2_i32 dt     = v2_i32_sub(pcenter, panchor);
    i32    dtl    = v2_i32_len(dt);
    i32    lenmax = (e->n_segs - 1) * dseg;
    if (o->state == BOSS_PLANT_EYE_HOOKED) {
        lenmax = (lenmax * lerp_i32(256, 128, min_i32(o->timer, 16), 16)) >> 8;
    }

    bool32 upd_seg = 1;
    if (lenmax < dtl) {
        if (truncate_len) {
            dt = v2_i32_setlenl(dt, dtl, lenmax);
            boss_plant_eye_move_to_centerpx(g, o, panchor.x + dt.x, panchor.y + dt.y);
            pcenter = obj_pos_center(o);
        } else {
            upd_seg = 0;
        }
        v2_i32 ps1 = {0};
        v2_i32 ps2 = v2_i32_shl(v2_i32_sub(pcenter, panchor), 8);
        for (i32 n = 1; n < e->n_segs - 1; n++) {
            bplant_seg_s *seg      = &e->segs[n];
            v2_i32        seggoalp = v2_i32_lerp(ps1, ps2, n, e->n_segs - 1);
            seg->p_q8              = v2_i32_lerp(seg->p_q8, seggoalp, 1, 2);
            seg->pp_q8             = seg->p_q8;
        }
    } else {
        for (i32 n = 1; n < e->n_segs - 1; n++) {
            bplant_seg_s *seg = &e->segs[n];
            seg->p_q8.y -= 50;
        }
    }

    {
        bplant_seg_s *seg  = &e->segs[e->n_segs - 1];
        v2_i32        offs = boss_plant_eye_idle_bop_offs(o);
        if (o->state == BOSS_PLANT_EYE_ATTACK ||
            o->state == BOSS_PLANT_EYE_ATTACK_EXE) {
            offs.x = 0;
            offs.y = 0;
        }
        seg->p_q8.x = (pcenter.x - panchor.x + offs.x) << 8;
        seg->p_q8.y = (pcenter.y - panchor.y + offs.y) << 8;
    }

    if (upd_seg) {
        boss_plant_update_seg(e->segs, e->n_segs, dseg << 8);
    }

    switch (o->state) {
    case BOSS_PLANT_EYE_ATTACK:
    case BOSS_PLANT_EYE_ATTACK_EXE:
    case BOSS_PLANT_EYE_SHOWN:
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        break;
    case BOSS_PLANT_EYE_HOOKED:
    case BOSS_PLANT_EYE_HIDDEN:
        o->flags &= OBJ_FLAG_HURT_ON_TOUCH;
        break;
    }
}

void boss_plant_eye_on_animate(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_plant_eye_s *e  = (boss_plant_eye_s *)o->mem;
    boss_plant_s     *bp = (boss_plant_s *)o->heap;
    o->animation++;
    obj_s *ohero = obj_get_hero(g);

    if (obj_pos_center(o).x < obj_pos_center(ohero).x) {
        o->facing = +1;
    } else {
        o->facing = -1;
    }
}

void boss_plant_eye_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx)
{
    if (!o) return;

    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    if (e->shown_lerp_q8 == 0) return;

    boss_plant_s *bp      = (boss_plant_s *)o->heap;
    v2_i32        panchor = {bp->x, bp->y + BPLANT_EYE_ANCHOR_Y};
    v2_i32        pcenter = obj_pos_center(o);
    pcenter               = v2_i32_lerp(panchor, pcenter, e->shown_lerp_q8, 256);

    v2_i32 peye = v2_i32_add(pcenter, cam);
    peye.x -= 48;
    peye.y -= 48 + 16;
    texrec_s tr0 = asset_texrec(TEXID_BOSSPLANT, 4 * 96, 0 * 96, 96, 96);
    texrec_s tr1 = asset_texrec(TEXID_BOSSPLANT, 4 * 96, 1 * 96, 96, 96);
    texrec_s tr2 = asset_texrec(TEXID_BOSSPLANT, 4 * 96, 2 * 96, 96, 96);
    texrec_s trx = asset_texrec(TEXID_BOSSPLANT, 2 * 64, 6 * 64, 64, 64);

    if (o->state == BOSS_PLANT_EYE_HOOKED) {
        tr0.x += 96;
        tr1.x += 96;
        tr2.x += 96;
    }

    v2_i32 posg = v2_i32_add(peye, boss_plant_eye_idle_bop_offs(o));
    v2_i32 pos2 = {posg.x,
                   posg.y + ((sin_q15(g->tick * 1300) * 2) / 32769)};
    v2_i32 pos0 = {posg.x + ((sin_q15(g->tick << 11) * 2) / 32769),
                   posg.y};
    v2_i32 pos1 = {posg.x + ((sin_q15(g->tick << 10) * 2) / 32769),
                   posg.y + ((cos_q15(g->tick << 9) * 2) / 32769)};

    i32 dseg   = boss_plant_eye_dist_seg(o);
    i32 n_tear = lerp_i32(0, e->n_segs, e->tear_off_tick, e->tear_off_tick_needed);

    for (i32 n = 0; n < e->n_segs; n++) {
        bplant_seg_s *seg = &e->segs[n];

        v2_i32 spos = v2_i32_lerp((v2_i32){0}, seg->p_q8, e->shown_lerp_q8, 256);
        v2_i32 pseg = {panchor.x + cam.x + (spos.x >> 8),
                       panchor.y + cam.y + (spos.y >> 8)};

        gfx_ctx_s ctxcir = ctx;
        i32       col    = PRIM_MODE_BLACK;
        i32       cd     = 16;
        if (o->ID != OBJID_BOSS_PLANT_EYE) {
            cd = n == e->n_segs - 1 ? 18 : 8;

            if (o->state == BOSS_PLANT_EYE_ATTACK) {
                col = PRIM_MODE_BLACK_WHITE;

                if (n == e->n_segs - 1) {
                    ctxcir.pat = gfx_pattern_50();
                } else if (((o->timer >> 2) & 1)) {
                    ctxcir.pat = gfx_pattern_25();
                }
            }
        }

        if (n < n_tear) {
            col = PRIM_MODE_WHITE;
        } else if (e->active_tear && ((o->animation >> 2) & 1)) {
            col        = PRIM_MODE_BLACK_WHITE;
            ctxcir.pat = gfx_pattern_50();
        }

        gfx_cir_fill(ctxcir, pseg, cd, col);

        if (o->ID != OBJID_BOSS_PLANT_EYE) {
            pseg.x -= 32;
            pseg.y -= 32;
            if (((e->n_segs - 3 - n) % 5) == 0) {
                i32 smode = n < n_tear ? SPR_MODE_WHITE : 0;
                gfx_spr(ctx, trx, pseg, 0, smode);
            }
        }
    }

    if (o->ID == OBJID_BOSS_PLANT_EYE) {
        i32 flip = 0 < o->facing ? SPR_FLIP_X : 0;
        gfx_spr(ctx, tr1, pos1, flip, 0);
        gfx_spr(ctx, tr2, pos2, flip, 0);
        gfx_spr(ctx, tr0, pos0, flip, 0);
    }
}

void boss_plant_eye_on_hook(g_s *g, obj_s *o, b32 hooked)
{
    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    rope_s           *r = &g->ghook.rope;
    if (hooked) {
        boss_plant_hide_other_eye(g, o);
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        o->state      = BOSS_PLANT_EYE_HOOKED;
        r->len_max_q4 = (r->len_max_q4 * 200) >> 8;
    } else {
        o->state = BOSS_PLANT_EYE_SHOWN;
    }
    o->timer    = 0;
    o->subtimer = 0;
}

void boss_plant_eye_hide(g_s *g, obj_s *o)
{
    if (!o) return;

    if (o->state == BOSS_PLANT_EYE_HOOKED) {
        grapplinghook_destroy(g, &g->ghook);
    }

    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    o->state            = BOSS_PLANT_EYE_HIDDEN;
    o->flags &= ~OBJ_FLAG_HOOKABLE;
    o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
    o->timer = 0;
}

void boss_plant_eye_show(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_plant_eye_show_nohook(g, o);
    o->flags |= OBJ_FLAG_HOOKABLE;
}

void boss_plant_eye_show_nohook(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    o->state            = BOSS_PLANT_EYE_SHOWN;
    o->flags |= OBJ_FLAG_HOOKABLE;
    o->timer = 0;
}

bool32 boss_plant_eye_try_attack(g_s *g, obj_s *o, i32 slash_y_slot, i32 slash_sig,
                                 b32 x_slash)
{
    if (!o) return 0;
    if (o->state == BOSS_PLANT_EYE_HOOKED) return 0;

    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    e->pos_src          = obj_pos_center(o);
    o->state            = BOSS_PLANT_EYE_ATTACK;
    o->timer            = 0;
    o->v_q8.x           = 0;
    o->v_q8.y           = 0;
    e->slash_y_slot     = slash_y_slot;
    e->slash_sig        = slash_sig;
    o->substate         = x_slash != 0;
    o->subtimer         = 40;
    return 1;
}

bool32 boss_plant_eye_is_busy(obj_s *o)
{
    if (!o) return 0;

    return (o->state == BOSS_PLANT_EYE_HOOKED ||
            o->state == BOSS_PLANT_EYE_ATTACK ||
            o->state == BOSS_PLANT_EYE_ATTACK_EXE);
}

bool32 boss_plant_eye_is_hooked(obj_s *o)
{
    if (!o) return 0;

    return (o->state == BOSS_PLANT_EYE_HOOKED);
}