// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#define PROJ_SMEAR_LEN 16

typedef struct {
    u32    f;
    u16    n_hist;   // current pos in array
    u16    hist_len; // smear entries = length of smear
    u16    d_smear;
    v2_i32 pos_hist[PROJ_SMEAR_LEN];
} projectile_s;

static_assert(sizeof(projectile_s) < 512, "");

obj_s *projectile_create(g_s *g, v2_i32 pos, v2_i32 vel, i32 subID)
{
    obj_s        *o = obj_create(g);
    projectile_s *p = (projectile_s *)o->mem;

    o->ID    = OBJID_PROJECTILE;
    o->v_q8  = v2_i16_from_i32(vel);
    o->subID = subID;
    //
    o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
    o->moverflags = OBJ_MOVER_TERRAIN_COLLISIONS;

    switch (subID) {
    case PROJECTILE_ID_STALACTITE_BREAK:
        o->w     = 16;
        o->h     = 16;
        o->timer = 30;
        break;
    default:
        o->w        = 16;
        o->h        = 16;
        p->hist_len = 10;
        p->d_smear  = 10;
        break;
    }

    o->pos.x = pos.x - o->w / 2;
    o->pos.y = pos.y - o->h / 2;

    for (i32 n = 0; n < p->hist_len; n++) {
        p->pos_hist[n] = o->pos;
    }
    return o;
}

void projectile_on_update(g_s *g, obj_s *o)
{
    switch (o->subID) {
    case PROJECTILE_ID_STALACTITE_BREAK:
        break;
    default:
        o->v_q8.y += 80;
        break;
    }

    if (o->bumpflags) {
        projectile_on_collision(g, o);
        return;
    }

    if (o->timer) { // if timed projectile
        o->timer--;
        if (o->timer == 0) {
            // remove

            obj_delete(g, o);
            return;
        }
    }

    projectile_s *p = (projectile_s *)o->mem;
    if (p->hist_len) {
        v2_i32 pp              = p->pos_hist[p->n_hist];
        p->n_hist              = (p->n_hist + 1) % p->hist_len;
        p->pos_hist[p->n_hist] = v2_i32_shr(v2_i32_add(pp, o->pos), 1);
        p->n_hist              = (p->n_hist + 1) % p->hist_len;
        p->pos_hist[p->n_hist] = o->pos;
    }
    obj_move_by_v_q8(g, o);
}

void projectile_on_animate(g_s *g, obj_s *o)
{
    projectile_s *p   = (projectile_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    i32 frameID = 0;
    i32 animID  = 0;
}

// smear behind projectile
void projectile_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s     ctx = gfx_ctx_display();
    projectile_s *pr  = (projectile_s *)o->mem;

    switch (o->subID) {
    case PROJECTILE_ID_STALACTITE_BREAK: {
        v2_i32 pos = v2_i32_add(o->pos, cam);
        gfx_cir_fill(ctx, pos, 12, GFX_COL_BLACK);
        gfx_cir_fill(ctx, pos, 8, GFX_COL_WHITE);
    } break;
    }

    if (pr->hist_len) {
        v2_i32 vadd = {o->w / 2 + cam.x, o->h / 2 + cam.y};
        v2_i32 pos  = v2_i32_add(o->pos, vadd);

        for (u32 n = 0; n < pr->hist_len; n++) {
            gfx_ctx_s ctxp = ctx;
            ctxp.pat       = gfx_pattern_interpolate(n, pr->hist_len);
            u32    k       = (n + pr->n_hist) % pr->hist_len;
            v2_i32 p       = v2_i32_add(pr->pos_hist[k], vadd);
            u32    d       = (n * pr->d_smear) / pr->hist_len;
            gfx_cir_fill(ctxp, p, d, GFX_COL_BLACK);
        }

        gfx_cir_fill(ctx, pos, pr->d_smear, GFX_COL_BLACK);
        gfx_cir_fill(ctx, pos, pr->d_smear - 4, GFX_COL_WHITE);
    }
}

void projectile_on_collision(g_s *g, obj_s *o)
{
    switch (o->subID) {
    case 1: break;
    default: break;
    }

    f32 vol = cam_snd_scale(g, o->pos, 300) * 0.1f;
    snd_play(SNDID_PROJECTILE_WALL, vol, rngr_f32(0.9f, 1.1f));
    obj_delete(g, o);
}