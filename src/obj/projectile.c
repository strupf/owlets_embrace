// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
    PROJECTILE_ST_IDLE,
};

#define PROJ_SMEAR_LEN 32

typedef struct {
    u16    n_hist;   // current pos in array
    u16    hist_len; // smear entries = length of smear
    u16    d_smear;
    v2_i32 pos_hist[PROJ_SMEAR_LEN];
} projectile_s;

static_assert(sizeof(projectile_s) < 512, "");

void projectile_on_update(game_s *g, obj_s *o);
void projectile_on_animate(game_s *g, obj_s *o);
void projectile_on_draw(game_s *g, obj_s *o, v2_i32 cam);

obj_s *projectile_create(game_s *g, v2_i32 pos, v2_i32 vel, i32 subID)
{
    obj_s        *o = obj_create(g);
    projectile_s *p = (projectile_s *)o->mem;

    o->ID    = OBJ_ID_PROJECTILE;
    o->flags = OBJ_FLAG_SPRITE |
               // OBJ_FLAG_RENDER_AABB |
               OBJ_FLAG_MOVER;
    o->w          = 16;
    o->h          = 16;
    o->pos        = pos;
    o->vel_q8     = vel;
    o->on_update  = projectile_on_update;
    o->on_animate = projectile_on_animate;
    o->on_draw    = projectile_on_draw;
    o->subID      = subID;

    switch (subID) {
    case 0:
        o->flags |= OBJ_FLAG_HURT_ON_TOUCH;
        p->hist_len     = 10;
        p->d_smear      = 10;
        o->gravity_q8.y = 80;
        o->drag_q8.x    = 255;
        o->drag_q8.y    = 255;
        break;
    }

    for (i32 n = 0; n < p->hist_len; n++) {
        p->pos_hist[n] = o->pos;
    }

    obj_sprite_s *spr = &o->sprites[0];
    o->n_sprites      = 1;
    return o;
}

void projectile_on_update(game_s *g, obj_s *o)
{
    if (o->bumpflags) {
        projectile_on_collision(g, o);
        return;
    }

    projectile_s *p        = (projectile_s *)o->mem;
    v2_i32        pp       = p->pos_hist[p->n_hist];
    p->n_hist              = (p->n_hist + 1) % p->hist_len;
    p->pos_hist[p->n_hist] = v2_shr(v2_add(pp, o->pos), 1);
    p->n_hist              = (p->n_hist + 1) % p->hist_len;
    p->pos_hist[p->n_hist] = o->pos;

    switch (o->state) {
    case PROJECTILE_ST_IDLE: {

        break;
    }
    default: break;
    }
}

void projectile_on_animate(game_s *g, obj_s *o)
{
    projectile_s *p   = (projectile_s *)o->mem;
    obj_sprite_s *spr = &o->sprites[0];

    i32 frameID = 0;
    i32 animID  = 0;

    switch (o->state) {
    case PROJECTILE_ST_IDLE: {

        break;
    }
    default: break;
    }
}

// smear behind projectile
void projectile_on_draw(game_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s     ctx = gfx_ctx_display();
    projectile_s *pr  = (projectile_s *)o->mem;

    v2_i32 vadd = {o->w / 2 + cam.x, o->h / 2 + cam.y};

    for (u32 n = 0; n < pr->hist_len; n++) {
        u32    k = (n + pr->n_hist) % pr->hist_len;
        v2_i32 p = v2_add(pr->pos_hist[k], vadd);
        u32    d = (n * pr->d_smear) / pr->hist_len;
        gfx_cir_fill(ctx, p, d, GFX_COL_BLACK);
    }
    v2_i32 pos = v2_add(o->pos, vadd);
    gfx_cir_fill(ctx, pos, pr->d_smear, GFX_COL_BLACK);
}

void projectile_on_collision(game_s *g, obj_s *o)
{
    switch (o->subID) {
    default: break;
    }

    obj_delete(g, o);
}