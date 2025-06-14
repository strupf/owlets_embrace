// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// gets triggered by touching it and falls down

#include "game.h"

enum {
    CRACKBLOCK_ST_IDLE,
    CRACKBLOCK_ST_SHAKE,
    CRACKBLOCK_ST_FALL
};

#define CRACKBLOCK_SHAKE_T 20

void crackblock_on_update(g_s *g, obj_s *o);
void crackblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);

void crackblock_load(g_s *g, map_obj_s *mo)
{
    obj_s *o = obj_create(g);

    o->ID              = OBJID_CRACKBLOCK;
    o->flags           = OBJ_FLAG_SOLID | OBJ_FLAG_CLIMBABLE;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->on_update       = crackblock_on_update;
    o->on_draw         = crackblock_on_draw;
    o->render_priority = RENDER_PRIO_INFRONT_FLUID_AREA - 1;
}

void crackblock_on_update(g_s *g, obj_s *o)
{
    o->timer++;

    switch (o->state) {
    case CRACKBLOCK_ST_IDLE: {
        obj_s *ohero = obj_get_hero(g);
        if (!ohero) break;

        if (overlap_rec(obj_rec_bottom(ohero), obj_aabb(o)) ||
            obj_from_obj_handle(ohero->linked_solid) == o) {
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case CRACKBLOCK_ST_SHAKE: {
        if (CRACKBLOCK_SHAKE_T <= o->timer) {
            o->timer = 0;
            o->state++;
        }
        break;
    }
    case CRACKBLOCK_ST_FALL: {
        o->v_q12.y += 70;
        o->subpos_q12.y += o->v_q12.y;
        i32 dy = o->subpos_q12.y >> 8;
        o->subpos_q12.y &= 255;

        for (i32 k = 0; k < dy; k++) {
            if (!map_blocked(g, obj_rec_bottom(o))) {
                obj_move(g, o, 0, 1);
                continue;
            }

            // landed on the floor
            // spawn dust explosions
            o->v_q12.y   = 0;
            o->on_update = 0;
            cam_screenshake_xy(&g->cam, 15, 0, 4);
            snd_play(SNDID_EXPLO1, 0.3f, 1.5f);

            i32 w = o->w >> 4;
            for (i32 n = 0; n < w; n++) {
                v2_i32 panim = {o->pos.x + n * 16 + 8, o->pos.y + o->h};
                obj_s *oanim = objanim_create(g, panim, OBJANIM_BOULDER_POOF);

                oanim->render_priority = RENDER_PRIO_INFRONT_TERRAIN_LAYER;
            }
            break;
        }
        break;
    }
    }
}

void crackblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    gfx_ctx_s ctx = gfx_ctx_display();
    v2_i32    p   = v2_i32_add(o->pos, cam);
    texrec_s  tr  = asset_texrec(0, 0, 0, 64, 64);

    if (o->state == CRACKBLOCK_ST_SHAKE) {
        p.x += rngr_i32(0, 2) * 2 - 2;
    }

    render_tile_terrain_block(ctx, p, o->w >> 4, o->h >> 4,
                              TILE_TYPE_BRIGHT_STONE);
}