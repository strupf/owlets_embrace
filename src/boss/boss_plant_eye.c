// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "boss/boss_plant.h"
#include "game.h"

#define BPLANT_EYE_ANCHOR_Y     48
#define BPLANT_EYE_OFFS_IDLE_Y  60
#define BPLANT_EYE_ATTACK_TICKS 50
#define BPLANT_EYE_SLASH_X      140 // from - to + horizontally

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

void boss_plant_eye_on_stomped_on(g_s *g, obj_s *o);
void boss_plant_eye_on_stomped_on_fake(g_s *g, obj_s *o);
void boss_plant_eye_hide(g_s *g, obj_s *o);
void boss_plant_eye_show(g_s *g, obj_s *o);
void boss_plant_eye_show_nohook(g_s *g, obj_s *o);
void boss_plant_eye_on_update(g_s *g, obj_s *o);
void boss_plant_eye_on_animate(g_s *g, obj_s *o);
void boss_plant_eye_on_hook(g_s *g, obj_s *o, i32 hooked);
void boss_plant_eye_on_update(g_s *g, obj_s *o);
void boss_plant_eye_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
i32  boss_plant_eye_slash_y(obj_s *o);
void boss_plant_eye_on_update_attached(g_s *g, obj_s *o);
void boss_plant_eye_on_update_ripped(g_s *g, obj_s *o);
void boss_plant_eye_draw_attached(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);
void boss_plant_eye_draw_ripped(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx);

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
        e->tear_off_tick_needed = 200;
        o->w                    = 48;
        o->h                    = 48;
        o->health               = 3;
        break;
    }
    case OBJID_BOSS_PLANT_EYE_FAKE_L:
    case OBJID_BOSS_PLANT_EYE_FAKE_R: {
        e->n_segs               = 20;
        e->anchor_x             = ID == OBJID_BOSS_PLANT_EYE_FAKE_R ? +70 : -70;
        e->anchor_y             = 60;
        e->tear_off_tick_needed = 200;
        o->w                    = 32;
        o->h                    = 28;
        o->health               = 2;
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
    o->subpos_q12.x = 0;
    o->subpos_q12.y = 0;
    o->v_q12.x      = 0;
    o->v_q12.y      = 0;
}

void boss_plant_eye_move_to_centerpx_keepv(g_s *g, obj_s *o, i32 x, i32 y)
{
    v2_i32 pcenter = obj_pos_center(o);
    i32    dx      = (x - o->w / 2) - o->pos.x;
    i32    dy      = (y - o->h / 2) - o->pos.y;
    obj_move(g, o, dx, dy);
}

void boss_plant_eye_on_update(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_plant_eye_s *e             = (boss_plant_eye_s *)o->mem;
    boss_plant_s     *bp            = (boss_plant_s *)o->heap;
    v2_i32            panchor       = {bp->x, bp->y + BPLANT_EYE_ANCHOR_Y};
    i32               pulling_force = 0;
    i32               dseg          = boss_plant_eye_dist_seg(o);
    obj_s            *owl           = obj_get_owl(g);
    v2_i32            hcenter       = obj_pos_center(owl);
    v2_i32            pcenter       = obj_pos_center(o);
    o->timer++;

    switch (o->state) {
    case BOSS_PLANT_EYE_RIPPED: {
        boss_plant_eye_on_update_ripped(g, o);
        break;
    }
    default: {
        boss_plant_eye_on_update_attached(g, o);
        break;
    }
    }

    switch (o->state) {
    case BOSS_PLANT_EYE_ATTACK_EXE:
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        break;
    default:
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        break;
    }
}

void boss_plant_eye_on_update_attached(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_plant_eye_s *e              = (boss_plant_eye_s *)o->mem;
    boss_plant_s     *bp             = (boss_plant_s *)o->heap;
    v2_i32            panchor        = {bp->x, bp->y + BPLANT_EYE_ANCHOR_Y};
    i32               pulling_force  = 0;
    i32               dseg           = boss_plant_eye_dist_seg(o);
    bool32            back_to_anchor = 1;
    bool32            truncate_len   = 0;
    obj_s            *owl            = obj_get_owl(g);
    v2_i32            owlcenter      = obj_pos_center(owl);
    v2_i32            pcenter        = obj_pos_center(o);

    e->active_tear = 0;
    if (o->state == BOSS_PLANT_EYE_HIDDEN) {
        e->shown_lerp_q8 = max_i32(e->shown_lerp_q8 - 20, 0);
    } else {
        e->shown_lerp_q8 = min_i32(e->shown_lerp_q8 + 30, 256);
    }

    if (o->state != BOSS_PLANT_EYE_HOOKED) {
        e->hooked_cd     = 0;
        e->tear_off_tick = 0;
    }

    switch (o->state) {
    default: {
        o->facing = owlcenter.x < pcenter.x ? -1 : +1;
        break;
    }
    case BOSS_PLANT_EYE_GRABBED_COMP:
    case BOSS_PLANT_EYE_GRAB_COMP: {
        back_to_anchor = 0;
        break;
    }
    case BOSS_PLANT_EYE_ATTACK: {
        o->facing         = -e->slash_sig;
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
            if (bp->time_of_slash_sfx != g->tick_gameplay) {
                bp->time_of_slash_sfx = g->tick_gameplay;
                snd_play(SNDID_BPLANT_SWOOSH, 1.2f, 1.f);
            }
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

        if (o->timer == 38) {
            if (o->substate == 1) {
                boss_plant_eye_try_attack(g, o, e->slash_y_slot + 2, -e->slash_sig, 0);
                o->substate = 2;
                o->subtimer = 20;
            } else {
                o->state = BOSS_PLANT_EYE_SHOWN;
                o->timer = 0;
            }
        }
        break;
    }
    case BOSS_PLANT_EYE_HOOKED: {
        truncate_len = 1;

        if (e->jank_tick) {
            e->jank_tick++;
            v2_i32 plerp = v2_i32_lerp(pcenter, e->jank_pos, e->jank_tick, 40);
            obj_move(g, o, plerp.x - pcenter.x, plerp.y - pcenter.y);
            o->v_q12.x = 0;
            o->v_q12.y = 0;
            if (e->jank_tick == 40) {
                e->jank_tick = 0;
                o->subtimer  = 0;
            }
        } else {
            o->subtimer++;
            if (o->subtimer == 120) {
                e->pos_src = pcenter;

                e->jank_pos.y = panchor.y + 30;
                e->jank_tick  = 1;
                o->subtimer   = 0;

                if (panchor.x < pcenter.x) {
                    e->jank_pos.x = pcenter.x - 20;
                } else {
                    e->jank_pos.x = pcenter.x + 20;
                }
            }
        }
        v2_i32 vpull = {0};
        i32    fpull = grapplinghook_f_at_obj_proj_v(&g->ghook, o, (v2_i32){0}, &vpull);

        if (fpull) {
            back_to_anchor = 0;
            e->tear_off_tick++;
            e->active_tear = 1;
            e->hooked_cd   = 20;
            o->v_q12.x += vpull.x >> 0;
            o->v_q12.y += vpull.y >> 2;
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
            o->v_q12.x = 0;
            o->v_q12.y = 0;
            boss_plant_eye_move_to_centerpx(g, o, goalpos.x, goalpos.y);
        } else {
            v2_i32 steer = steer_seek(pcenter, o->v_q12, goalpos, Q_VOBJ(3.0));
            o->v_q12.x += steer.x / 4;
            o->v_q12.y += steer.y / 4;
        }
    }

    if (e->tear_off_tick_needed <= e->tear_off_tick) {
        grapplinghook_destroy(g, &g->ghook);
        owl->v_q12.x = 0;
        owl->v_q12.y >>= 1;

        e->tear_off_tick      = 0;
        e->n_segs             = 0;
        o->state              = BOSS_PLANT_EYE_RIPPED;
        o->moverflags         = OBJ_MOVER_TERRAIN_COLLISIONS;
        o->v_q12.x            = Q_VOBJ(1.6) * (rngr_i32(0, 1) * 2 - 1);
        o->v_q12.y            = 0;
        o->flags              = OBJ_FLAG_ACTOR;
        o->enemy              = enemy_default();
        o->enemy.hurt_on_jump = 1;
        i32 w_old             = o->w;
        i32 h_old             = o->h;
        o->w                  = 28;
        o->h                  = 28;
        o->pos.x += (w_old - o->w) >> 1;
        o->pos.y += (h_old - o->h) >> 1;

        switch (o->ID) {
        case OBJID_BOSS_PLANT_EYE:
            o->enemy.on_hurt = boss_plant_eye_on_stomped_on;
            break;
        case OBJID_BOSS_PLANT_EYE_FAKE_R:
        case OBJID_BOSS_PLANT_EYE_FAKE_L:
            o->enemy.on_hurt = boss_plant_eye_on_stomped_on_fake;
            break;
        }

        boss_plant_on_eye_tear_off(g, o);
    } else {
        o->v_q12.x = clamp_sym_i32(o->v_q12.x, 20000);
        o->v_q12.y = clamp_sym_i32(o->v_q12.y, 20000);
        obj_move_by_v_q12(g, o);

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
                boss_plant_eye_move_to_centerpx_keepv(g, o, panchor.x + dt.x, panchor.y + dt.y);
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

        bplant_seg_s *headseg = &e->segs[e->n_segs - 1];
        v2_i32        offs    = boss_plant_eye_idle_bop_offs(o);
        if (o->state == BOSS_PLANT_EYE_ATTACK ||
            o->state == BOSS_PLANT_EYE_ATTACK_EXE) {
            offs.x = 0;
            offs.y = 0;
        }
        headseg->p_q8.x = (pcenter.x - panchor.x + offs.x) << 8;
        headseg->p_q8.y = (pcenter.y - panchor.y + offs.y) << 8;

        if (upd_seg) {
            boss_plant_update_seg(e->segs, e->n_segs, dseg << 8);
        }
    }
}

void boss_plant_eye_on_update_ripped(g_s *g, obj_s *o)
{
    boss_plant_eye_s *e        = (boss_plant_eye_s *)o->mem;
    boss_plant_s     *bp       = (boss_plant_s *)o->heap;
    v2_i32            panchor  = {bp->x, bp->y + BPLANT_EYE_ANCHOR_Y};
    i32               dseg     = boss_plant_eye_dist_seg(o);
    v2_i32            pcenter  = obj_pos_center(o);
    bool32            grounded = obj_grounded(g, o);

    o->v_q12.y += Q_VOBJ(0.2);
    if (o->bumpflags & OBJ_BUMP_Y) {
        o->v_q12.y = 0;
        o->flags |=
            OBJ_FLAG_HURT_ON_TOUCH |
            OBJ_FLAG_ENEMY |
            OBJ_FLAG_OWL_JUMPSTOMPABLE;
    }
    if (o->bumpflags & OBJ_BUMP_X) {
        o->v_q12.x = -o->v_q12.x;
    }
    o->bumpflags = 0;

    if (grounded) {
        o->v_q12.x = 0;
        o->v_q12.y = 0;
        o->subtimer++;

        i32 jumpticks = o->ID == OBJID_BOSS_PLANT_EYE ? 10 : 25;

        if (jumpticks <= o->subtimer) {
            o->subtimer  = 0;
            i32 dx       = panchor.x - o->pos.x;
            i32 jx       = 0;
            o->animation = 0;

            switch (o->ID) {
            case OBJID_BOSS_PLANT_EYE: {
                o->v_q12.y = -rngr_i32(Q_VOBJ(4.0), Q_VOBJ(5.0));
                jx         = rngr_i32(Q_VOBJ(0.8), Q_VOBJ(3.0));
                break;
            }
            case OBJID_BOSS_PLANT_EYE_FAKE_R:
            case OBJID_BOSS_PLANT_EYE_FAKE_L: {
                o->v_q12.y = -rngr_i32(Q_VOBJ(4.0), Q_VOBJ(5.0));
                jx         = rngr_i32(Q_VOBJ(0.8), Q_VOBJ(3.0));
                break;
            }
            }

            if (0) {
            } else if (dx <= -120) {
                o->v_q12.x = +jx;
            } else if (dx >= +120) {
                o->v_q12.x = -jx;
            } else {
                o->v_q12.x = jx * (rngr_i32(0, 1) * 2 - 1);
            }
        }
    } else {
        o->subtimer = 0;
    }
    obj_move_by_v_q12(g, o);
}

void boss_plant_eye_on_animate(g_s *g, obj_s *o)
{
    if (!o) return;

    boss_plant_eye_s *e  = (boss_plant_eye_s *)o->mem;
    boss_plant_s     *bp = (boss_plant_s *)o->heap;
    o->animation++;

    if (o->state == BOSS_PLANT_EYE_RIPPED) {
        if (o->v_q12.x < 0) {
            o->facing = -1;
        }
        if (o->v_q12.x > 0) {
            o->facing = +1;
        }
    }
}

void boss_plant_eye_draw(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx)
{
    if (!o) {
        return;
    }

    switch (o->state) {
    case BOSS_PLANT_EYE_RIPPED: {
        boss_plant_eye_draw_ripped(g, o, cam, ctx);
        break;
    }
    default: {
        boss_plant_eye_draw_attached(g, o, cam, ctx);
        break;
    }
    }
}

void boss_plant_eye_draw_attached(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx)
{
    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    if (e->shown_lerp_q8 == 0) return;

    boss_plant_s *bp      = (boss_plant_s *)o->heap;
    v2_i32        panchor = {bp->x, bp->y + BPLANT_EYE_ANCHOR_Y};
    v2_i32        pcenter = v2_i32_lerp(panchor, obj_pos_center(o),
                                        e->shown_lerp_q8, 256);

    texrec_s trx    = asset_texrec(TEXID_BOSSPLANT, 2 * 64, 6 * 64, 64, 64);
    v2_i32   posg   = v2_i32_add(v2_i32_add(pcenter, cam),
                                 boss_plant_eye_idle_bop_offs(o));
    i32      dseg   = boss_plant_eye_dist_seg(o);
    i32      n_tear = lerp_i32(0, e->n_segs, e->tear_off_tick, e->tear_off_tick_needed);

    for (i32 n = 0; n < e->n_segs - 1; n++) {
        bplant_seg_s *seg   = &e->segs[n];
        i32           i_seg = e->n_segs - n - 1;

        v2_i32 spos = v2_i32_lerp((v2_i32){0}, seg->p_q8, e->shown_lerp_q8, 256);
        v2_i32 pseg = {panchor.x + cam.x + (spos.x >> 8),
                       panchor.y + cam.y + (spos.y >> 8)};

        gfx_ctx_s ctxcir = ctx;
        i32       col    = PRIM_MODE_BLACK;
        i32       cd     = 16;
        if (o->ID != OBJID_BOSS_PLANT_EYE) {
            cd = i_seg == 0 ? 18 : 8;

            if (o->state == BOSS_PLANT_EYE_ATTACK) {
                col = PRIM_MODE_BLACK_WHITE;

                if (i_seg == 0) {
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
            if (((i_seg + 3) % 5) == 0) {
                i32 smode = n < n_tear ? SPR_MODE_WHITE : 0;
                gfx_spr(ctx, trx, pseg, 0, smode);
            }
        }
    }

    if (o->ID == OBJID_BOSS_PLANT_EYE) {
        i32 flip = 0 < o->facing ? SPR_FLIP_X : 0;

        v2_i32   pos2 = {posg.x - 32,
                         posg.y - 8 + ((sin_q15(g->tick * 1300) * 2) / 32769)};
        v2_i32   pos0 = {posg.x - 48 + ((sin_q15(g->tick << 11) * 2) / 32769),
                         posg.y - 48};
        v2_i32   pos1 = {posg.x - 48 + ((sin_q15(g->tick << 10) * 2) / 32769),
                         posg.y - 48 + ((cos_q15(g->tick << 9) * 2) / 32769)};
        texrec_s tr0  = asset_texrec(TEXID_BOSSPLANT, 4 * 96, 0 * 96, 96, 96);
        texrec_s tr1  = asset_texrec(TEXID_BOSSPLANT, 4 * 96, 1 * 96, 96, 96);
        texrec_s tr2  = asset_texrec(TEXID_BOSSPLANT, 6 * 64, 3 * 64, 64, 64);
        if (o->state == BOSS_PLANT_EYE_HOOKED) {
            tr0.x += 96;
            tr1.x += 96;
            tr2.x += 64;
        }

        gfx_spr(ctx, tr1, pos1, flip, 0);
        gfx_spr(ctx, tr2, pos2, flip, 0);
        gfx_spr(ctx, tr0, pos0, flip, 0);
    } else {
        i32      flip = 0 < o->facing ? 0 : SPR_FLIP_X;
        texrec_s trk  = asset_texrec(TEXID_BOSSPLANT, 5 * 64, 3 * 64, 64, 64);
        trk.y += 64 * ((o->animation >> 3) & 1);
        v2_i32 posk = {posg.x - 32, posg.y - 32};
        if (o->state == BOSS_PLANT_EYE_GRABBED_COMP) {
            trk.x = 64 * 4;
            trk.y = 64 * 6;
        }
        gfx_spr(ctx, trk, posk, flip, 0);
    }
}

void boss_plant_eye_draw_ripped(g_s *g, obj_s *o, v2_i32 cam, gfx_ctx_s ctx)
{
    boss_plant_eye_s *e       = (boss_plant_eye_s *)o->mem;
    boss_plant_s     *bp      = (boss_plant_s *)o->heap;
    v2_i32            pcenter = v2_i32_add(obj_pos_center(o), cam);
    pcenter.x -= 32;
    pcenter.y -= 32;

    if (o->enemy.hurt_tick) {
        pcenter.x += rngr_sym_i32(3);
        pcenter.y += rngr_sym_i32(3);
    }

    switch (o->ID) {
    case OBJID_BOSS_PLANT_EYE: {
        i32      fl = 0 < o->facing ? SPR_FLIP_X : 0;
        texrec_s tr = asset_texrec(TEXID_BOSSPLANT, 8 * 64, 3 * 64, 64, 64);
        tr.x += 64 * ((o->animation >> 2) & 1);
        if (o->enemy.hurt_tick &&
            o->enemy.hurt_tick_max / 2 <= o->enemy.hurt_tick) {
            tr.x = 64 * 7;
        }

        gfx_spr(ctx, tr, pcenter, fl, 0);
        break;
    }
    case OBJID_BOSS_PLANT_EYE_FAKE_R:
    case OBJID_BOSS_PLANT_EYE_FAKE_L: {
        i32      mode = 0;
        i32      fl   = 0 < o->facing ? 0 : SPR_FLIP_X;
        texrec_s trk  = asset_texrec(TEXID_BOSSPLANT, 5 * 64, 3 * 64, 64, 64);

        if (obj_grounded(g, o)) {
            trk.y = 64 * (3 + ((o->animation >> 2) & 1));
        } else {
            i32 fr = ani_frame_loop(ANIID_BPLANT_HOP, o->animation);
            trk.y  = 64 * (3 + fr);
        }

        if (o->enemy.hurt_tick < 0) {
            if (-o->enemy.hurt_tick > o->enemy.die_tick_max / 2) {
                mode = SPR_MODE_WHITE;
            }
        } else {
            if (o->enemy.hurt_tick > 0 &&
                o->enemy.hurt_tick > o->enemy.hurt_tick_max / 2) {
                mode = SPR_MODE_WHITE;
            }
        }

        gfx_spr(ctx, trk, pcenter, fl, mode);
        break;
    }
    }

    i32      kk      = (o->animation >> 1) % 10;
    texrec_s trarrow = asset_texrec(TEXID_BOSSPLANT, kk * 32, 448 + 32, 32, 32);
    pcenter.x += 16;
    pcenter.y -= 8;
    gfx_spr(ctx, trarrow, pcenter, 0, 0);
}

void boss_plant_eye_on_hook(g_s *g, obj_s *o, i32 hooked)
{
    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    wire_s           *r = &g->ghook.wire;
    if (o->state == BOSS_PLANT_EYE_RIPPED) return;
    if (hooked) {
        boss_plant_hideshow_other_eye(g, o, 0);
        o->flags &= ~OBJ_FLAG_HURT_ON_TOUCH;
        o->state            = BOSS_PLANT_EYE_HOOKED;
        g->ghook.len_max_q4 = (g->ghook.len_max_q4 * 200) >> 8;
    } else {
        boss_plant_hideshow_other_eye(g, o, 1);
        o->state = BOSS_PLANT_EYE_SHOWN;
    }
    o->timer    = 0;
    o->subtimer = 0;
}

void boss_plant_eye_hide(g_s *g, obj_s *o)
{
    if (!o) return;
    if (o->state == BOSS_PLANT_EYE_RIPPED) return;

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
    if (o->state != BOSS_PLANT_EYE_HIDDEN) return;

    boss_plant_eye_show_nohook(g, o);
    o->flags |= OBJ_FLAG_HOOKABLE;
}

void boss_plant_eye_show_nohook(g_s *g, obj_s *o)
{
    if (!o) return;
    if (o->state == BOSS_PLANT_EYE_RIPPED) return;

    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    o->state            = BOSS_PLANT_EYE_SHOWN;
    o->flags |= OBJ_FLAG_HOOKABLE;
    o->timer = 0;
}

bool32 boss_plant_eye_try_attack(g_s *g, obj_s *o, i32 slash_y_slot, i32 slash_sig,
                                 b32 x_slash)
{
    if (!o) return 0;
    if (o->state == BOSS_PLANT_EYE_HOOKED ||
        o->state == BOSS_PLANT_EYE_HIDDEN ||
        o->state == BOSS_PLANT_EYE_RIPPED) return 0;

    boss_plant_eye_s *e = (boss_plant_eye_s *)o->mem;
    e->pos_src          = obj_pos_center(o);
    o->state            = BOSS_PLANT_EYE_ATTACK;
    o->timer            = 0;
    o->v_q12.x          = 0;
    o->v_q12.y          = 0;
    e->slash_y_slot     = slash_y_slot;
    e->slash_sig        = slash_sig;
    o->substate         = x_slash != 0;
    o->subtimer         = 50;
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

void boss_plant_eye_on_stomped_on_fake(g_s *g, obj_s *o)
{
    if (o->health) return;

    boss_plant_s     *bp = (boss_plant_s *)o->heap;
    boss_plant_eye_s *e  = (boss_plant_eye_s *)o->mem;

    boss_plant_on_killed_eye(g, o);
    // obj_delete(g, o);
}

void boss_plant_eye_on_stomped_on(g_s *g, obj_s *o)
{
    if (o->health) return;

    boss_plant_s     *bp = (boss_plant_s *)o->heap;
    boss_plant_eye_s *e  = (boss_plant_eye_s *)o->mem;

    boss_plant_on_killed_eye(g, o);
    obj_delete(g, o);
}

bool32 boss_plant_eye_is_teared(obj_s *o)
{
    if (!o) return 0;
    return (o->state == BOSS_PLANT_EYE_RIPPED);
}