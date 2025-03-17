// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

enum {
    CRUMBLE_STATE_IDLE,
    CRUMBLE_STATE_BREAKING,
    CRUMBLE_STATE_RESPAWNING,
};

#define CRUMBLE_TICKS_BREAK   25
#define CRUMBLE_TICKS_RESPAWN 110
#define CRUMBLE_PARTICLE_TIME 50

void crumbleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);
void crumbleblock_on_update(g_s *g, obj_s *o);

void crumbleblock_load(g_s *g, map_obj_s *mo)
{
    obj_s *o           = obj_create(g);
    o->ID              = OBJID_CRUMBLEBLOCK;
    o->on_update       = crumbleblock_on_update;
    o->on_draw         = crumbleblock_on_draw;
    o->render_priority = RENDER_PRIO_HERO + 1;
    o->state           = CRUMBLE_STATE_IDLE;
    o->substate        = TILE_BLOCK;
    o->pos.x           = mo->x;
    o->pos.y           = mo->y;
    o->w               = mo->w;
    o->h               = mo->h;
    o->flags =
        OBJ_FLAG_SOLID |
        OBJ_FLAG_CLIMBABLE;
    o->substate = map_obj_bool(mo, "Platform") ? TILE_ONE_WAY : TILE_BLOCK;
}

void crumbleblock_break(g_s *g, obj_s *o);

static void crumbleblock_start_breaking(g_s *g, obj_s *o)
{
    if (o->state == CRUMBLE_STATE_IDLE) {
        o->state = CRUMBLE_STATE_BREAKING;
        o->timer = CRUMBLE_TICKS_BREAK;

        i32 nx = o->w >> 4;
        i32 ny = o->w >> 4;
        for (i32 tx = 0; tx < nx; tx++) {
            v2_i32 p1 = {o->pos.x + tx * 16 + 8, o->pos.y + 4};
            particle_emit_ID(g, PARTICLE_EMIT_ID_CRUMBLEBLOCK, p1);
            v2_i32 p2 = {o->pos.x + tx * 16 + 8, o->pos.y + o->h - 4};
            particle_emit_ID(g, PARTICLE_EMIT_ID_CRUMBLEBLOCK, p2);
        }
        for (i32 ty = 0; ty < ny; ty++) {
            v2_i32 p1 = {o->pos.x + 4, o->pos.y + ty * 16 + 8};
            particle_emit_ID(g, PARTICLE_EMIT_ID_CRUMBLEBLOCK, p1);
            v2_i32 p2 = {o->pos.x + o->w - 4, o->pos.y + ty * 16 + 8};
            particle_emit_ID(g, PARTICLE_EMIT_ID_CRUMBLEBLOCK, p2);
        }
    }
}

void crumbleblock_on_hooked(g_s *g, obj_s *o)
{
    if (o->state == CRUMBLE_STATE_IDLE && o->substate == TILE_BLOCK) {
        crumbleblock_start_breaking(g, o);
    }
}

void crumbleblock_on_update(g_s *g, obj_s *o)
{
    if (o->subtimer) {
        o->subtimer++;
        if (CRUMBLE_PARTICLE_TIME <= o->subtimer) {
            o->subtimer = 0;
        }
    }

    switch (o->state) {
    case CRUMBLE_STATE_IDLE: {
        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (!ohero) break;

        hero_s *h           = (hero_s *)ohero->heap;
        rec_i32 r           = {o->pos.x, o->pos.y, o->w, 1};
        rec_i32 rclimb      = {ohero->pos.x - 1,
                               ohero->pos.y + HERO_CLIMB_Y1_OFFS,
                               ohero->w + 2,
                               ohero->h - HERO_CLIMB_Y1_OFFS - HERO_CLIMB_Y2_OFFS};
        bool32  standing_on = 0 <= ohero->v_q8.y &&
                             overlap_rec(obj_rec_bottom(ohero), r);
        bool32 climbing_on = h->climbing && overlap_rec(rclimb, obj_aabb(o));
        if (standing_on || climbing_on) {
            crumbleblock_start_breaking(g, o);
        }
        break;
    }
    case CRUMBLE_STATE_BREAKING: {
        o->timer--;
        if (0 < o->timer) break;

        crumbleblock_break(g, o);
        cam_screenshake_xy(&g->cam, 16, 1, 1);
        break;
    }
    case CRUMBLE_STATE_RESPAWNING: {
        o->timer--;
        if (0 < o->timer) break;

        o->flags |= OBJ_FLAG_SOLID;
        o->state = CRUMBLE_STATE_IDLE;
        game_on_solid_appear_ext(g, o);
        break;
    }
    }
}

void crumbleblock_break(g_s *g, obj_s *o)
{
    if (o->state == CRUMBLE_STATE_RESPAWNING) return;
    rec_i32 r = {o->pos.x - 1, o->pos.y - 1, o->w + 2, o->h + 2};

    o->flags &= ~OBJ_FLAG_SOLID;
    o->state    = CRUMBLE_STATE_RESPAWNING;
    o->timer    = CRUMBLE_TICKS_RESPAWN;
    o->subtimer = 1;

    for (obj_each(g, i)) {
        if (i->ID == OBJID_CRUMBLEBLOCK &&
            i->state == CRUMBLE_STATE_IDLE &&
            overlap_rec(r, obj_aabb(i))) {
            crumbleblock_start_breaking(g, i);
        }
    }
}

void crumbleblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    v2_i32    p   = v2_i32_add(o->pos, cam);
    gfx_ctx_s ctx = gfx_ctx_display();

    if (o->subtimer) {
        // falling particles using integration: x = att/2 + vt
        tex_s     tterrain = asset_tex(TEXID_TILESET_TERRAIN);
        i32       nx       = o->w >> 4;
        i32       ny       = o->h >> 4;
        i32       t        = o->subtimer - 1;
        u32       s        = 213;
        i32       py_acc   = 20 * t * t;
        gfx_ctx_s ctxp     = ctx;

        if (CRUMBLE_PARTICLE_TIME - 20 <= o->subtimer) {
            i32 t1   = o->subtimer - (CRUMBLE_PARTICLE_TIME - 20);
            ctxp.pat = gfx_pattern_interpolate(20 - t1, 20);
        }

        for (i32 ty = 0; ty < ny; ty++) {
            for (i32 tx = 0; tx < nx; tx++) {
                i32 k = tileindex_terrain_block(nx, ny,
                                                TILE_TYPE_BRIGHT_BREAKING,
                                                tx, ty);

                // subdivide each tile in 4 subtile particles
                for (i32 i = 0; i < 4; i++) {
                    i32 u = i & 1;
                    i32 v = i >> 1;

                    texrec_s trec = {tterrain, (u << 4), (v << 4) + (k << 5),
                                     16, 16};
                    v2_i32   v_q8 = {rngsr_i32(&s, -100, +100),
                                     rngsr_i32(&s, -150, +150) - 100};
                    v2_i32   p_q8 = {v_q8.x * t,
                                     v_q8.y * t + py_acc};
                    v2_i32   p_px = {p.x + (tx << 4) - 8 + (u << 4),
                                     p.y + (ty << 4) - 8 + (v << 4)};
                    p_px          = v2_i32_add(p_px, v2_i32_shr(p_q8, 8));
                    gfx_spr(ctxp, trec, p_px, 0, 0);
                }
            }
        }
    }

    i32 i0_break = CRUMBLE_TICKS_BREAK - o->timer;
    if (o->state == CRUMBLE_STATE_BREAKING && i0_break < 10) {
        // visual growing rectangle "impact" effect
        spm_push();
        i32   wend = o->w + 16;
        i32   hend = o->h + 16;
        tex_s tmp  = tex_create(wend, hend, 1, spm_allocator(), 0);
        tex_clr(tmp, GFX_COL_CLEAR);

        // render block to temporary texture
        gfx_ctx_s ctxtmp = gfx_ctx_default(tmp);
        v2_i32    postmp = {8, 8};
        render_tile_terrain_block(ctxtmp, postmp, o->w >> 4, o->h >> 4,
                                  TILE_TYPE_BRIGHT_BREAKING);

        // render growing rectangle (from center)
        i32     rw    = lerp_i32(0, wend, min_i32(i0_break, 6), 6);
        i32     rh    = lerp_i32(0, hend, min_i32(i0_break, 6), 6);
        rec_i32 rfill = {(wend - rw) >> 1, (hend - rh) >> 1, rw, rh};
        gfx_rec_fill(ctxtmp, rfill, PRIM_MODE_INV);

        // render final block with shaking offset
        i32    ox = rngr_i32(-1, +1);
        i32    oy = (rngr_i32(0, 1) * 2 - 1) * ox;
        v2_i32 pf = {p.x - 8 + ox, p.y - 8 + oy};
        gfx_spr(ctx, texrec_from_tex(tmp), pf, 0, 0);
        spm_pop();
    } else if (o->state == CRUMBLE_STATE_RESPAWNING) {
        i32 i0_respawn = 10 - o->timer;
        if (i0_respawn < 0) return;

        // visual growing rectangle "impact" effect
        spm_push();
        i32   wend = o->w + 16;
        i32   hend = o->h + 16;
        tex_s tmp  = tex_create(wend, hend, 1, spm_allocator(), 0);
        tex_clr(tmp, GFX_COL_CLEAR);

        // render block to temporary texture
        gfx_ctx_s ctxtmp = gfx_ctx_default(tmp);
        v2_i32    postmp = {8, 8};
        render_tile_terrain_block(ctxtmp, postmp, o->w >> 4, o->h >> 4,
                                  TILE_TYPE_BRIGHT_BREAKING);

        // render growing rectangle (from center)

        i32     rw    = lerp_i32(0, wend, min_i32(i0_respawn, 6), 6);
        i32     rh    = lerp_i32(0, hend, min_i32(i0_respawn, 6), 6);
        rec_i32 rfill = {(wend - rw) >> 1, (hend - rh) >> 1, rw, rh};
        gfx_rec_fill(ctxtmp, rfill, PRIM_MODE_INV);

        // render final block
        v2_i32 pf = {p.x - 8, p.y - 8};
        ctx.pat   = gfx_pattern_interpolate(10 - o->timer, 10);
        gfx_spr(ctx, texrec_from_tex(tmp), pf, 0, 0);
        spm_pop();
    } else {
        render_tile_terrain_block(ctx, p, o->w >> 4, o->h >> 4,
                                  TILE_TYPE_BRIGHT_BREAKING);
    }
}
