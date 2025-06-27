// ============ =================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

typedef struct {
    i32 hit_tick;
    i32 l_line;
    i32 hitID;
    i32 saveID;
} hangingblock_s;

enum {
    HANGINGBLOCK_IDLE,
    HANGINGBLOCK_FALLING,
    HANGINGBLOCK_SET,
};

#define HANGINGBLOCK_HIT_TICKS     70
#define HANGINGBLOCK_RECOVER_TICKS 50 // ticks to restore a health point

void hangingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void hangingblock_on_update(g_s *g, obj_s *o);
void hangingblock_on_hit(g_s *g, obj_s *o);

void hangingblock_load(g_s *g, map_obj_s *mo)
{
    i32    saveID      = map_obj_i32(mo, "SaveID");
    obj_s *o           = obj_create(g);
    o->ID              = OBJID_HANGINGBLOCK;
    o->on_update       = hangingblock_on_update;
    o->on_draw         = hangingblock_on_draw;
    o->render_priority = RENDER_PRIO_HERO + 1;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->health_max      = 3;
    o->health          = o->health_max;
    o->flags =
        OBJ_FLAG_SOLID |
        OBJ_FLAG_CLIMBABLE;
    o->moverflags     = OBJ_MOVER_TERRAIN_COLLISIONS | OBJ_MOVER_ONE_WAY_PLAT;
    hangingblock_s *b = (hangingblock_s *)o->mem;

    if (save_event_exists(g, saveID)) {
        o->state = HANGINGBLOCK_SET;

        rec_i32 rb = obj_rec_bottom(o);
        for (i32 steps = 1024; steps && !map_blocked(g, rb); steps--) {
            o->pos.y++;
            rb.y++;
        }
    } else {
        b->saveID = saveID;
        b->l_line = map_obj_i32(mo, "L_Rope") << 4;
    }
}

void hangingblock_on_update(g_s *g, obj_s *o)
{
    hangingblock_s *b = (hangingblock_s *)o->mem;
    o->animation++;

    switch (o->state) {
    case HANGINGBLOCK_IDLE:
        if (b->hit_tick) {
            b->hit_tick--;
        }
        if (o->timer) {
            o->timer--;
            if (o->timer == 0) {
                o->health++;
                if (o->health < o->health_max) {
                    o->timer = HANGINGBLOCK_RECOVER_TICKS;
                }
            }
        }
        break;
    case HANGINGBLOCK_FALLING:
        o->timer++;
        if (o->timer < 10) {
            break;
        }
        o->v_q12.y += Q_VOBJ(0.35);
        o->v_q12.y      = min_i32(o->v_q12.y, Q_VOBJ(4.0));
        o->subpos_q12.y = o->subpos_q12.y + o->v_q12.y;
        i32 dy          = o->subpos_q12.y >> 12;
        o->subpos_q12.y &= 0xFFF;

        for (i32 y = 0; y < dy; y++) {
            rec_i32 rb = obj_rec_bottom(o);
            if (map_blocked(g, rb)) {
                o->state   = HANGINGBLOCK_SET;
                o->v_q12.y = 0;
                break;
            }
            obj_move(g, o, 0, 1);
        }
        break;
    case HANGINGBLOCK_SET:
        break;
    }
}

void hangingblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    hangingblock_s *b       = (hangingblock_s *)o->mem;
    gfx_ctx_s       ctx     = gfx_ctx_display();
    v2_i32          p       = v2_i32_add(o->pos, cam);
    v2_i32          ps      = p;
    i32             terrain = TILE_TYPE_BRIGHT_STONE;

    if (o->state == HANGINGBLOCK_IDLE) {
        i32 px_line = p.x + (o->w >> 1);
        i32 w_line  = 3 + o->health;

        i32 k = (o->animation >> 4) & 3;
        if (k <= 1) {
            ps.y += 2;
        }

        switch (k) {
        case 0: break;
        case 1: px_line += 1; break;
        case 2: break;
        case 3: px_line -= 1; break;
        }

        if (b->hit_tick) {
            px_line += (lerp_i32(0, 5, b->hit_tick, HANGINGBLOCK_HIT_TICKS) *
                        sin_q15(o->animation << 14)) /
                       32769;
        }
        rec_i32 rline       = {px_line - (w_line >> 1), p.y - b->l_line, w_line, b->l_line};
        rec_i32 rline_outer = {rline.x - 1, rline.y, rline.w + 2, rline.h};

        gfx_rec_fill(ctx, rline_outer, PRIM_MODE_WHITE);
        gfx_rec_fill(ctx, rline, PRIM_MODE_BLACK);
    }

    i32 tx = (o->w >> 4);
    i32 ty = (o->h >> 4);
    ps.x &= ~1;
    ps.y &= ~1;
    render_tile_terrain_block(ctx, ps, tx, ty, terrain);

    // render rope wrap tiles
    if (o->state == HANGINGBLOCK_IDLE || o->state == HANGINGBLOCK_FALLING) {
        i32 tilex = 1;
        if (o->state == HANGINGBLOCK_FALLING) {
            tilex += lerp_i32(0, 3, min_i32(o->timer, 16), 16);
        }

        texrec_s tr   = asset_texrec(TEXID_TILESET_TERRAIN, 32, 0, 32, 32);
        i32      posx = ps.x + ((tx << 3)) - 16;

        for (i32 y = 0; y < ty; y++) {

            i32 tiley = 1;
            if (ty == 1) {
                tiley = 4;
            } else {
                if (y == 0) {
                    tiley = 1;
                } else if (y == ty - 1) {
                    tiley = 3;
                } else {
                    tiley = 2;
                }
            }

            tr.y     = (tilex + (tiley) * 12) << 5;
            v2_i32 p = {posx, ps.y + (y << 4) - 8};
            gfx_spr(ctx, tr, p, 0, 0);
        }
    }
}

rec_i32 hangingblock_rec_line(obj_s *o)
{
    hangingblock_s *b = (hangingblock_s *)o->mem;
    rec_i32         r = {o->pos.x + (o->w >> 1) - 4,
                         o->pos.y - b->l_line,
                         8, b->l_line};
    return r;
}

void hangingblock_on_hit(g_s *g, obj_s *o)
{
    hangingblock_s *b = (hangingblock_s *)o->mem;

    if (o->state == HANGINGBLOCK_IDLE && b->hitID != g->hero_hitID) {
        b->hitID    = g->hero_hitID;
        b->hit_tick = HANGINGBLOCK_HIT_TICKS;
        o->health--;

        if (o->health == 0) {
            save_event_register(g, b->saveID);
            o->state = HANGINGBLOCK_FALLING;
            o->timer = 0;
        } else {
            o->timer = HANGINGBLOCK_RECOVER_TICKS;
        }
    }
}