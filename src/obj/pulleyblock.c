// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

typedef struct {
    obj_s *children[4];
    obj_s *oparent;
    i32    y0;
    i32    y1;
    i32    l_rope;
    i32    y_rope;
    i8     movsign;
    u8     n_children;
} pulleyblock_s;

// link up pulleyblocks post map parsing
void pulleyblocks_setup(g_s *g)
{
    for (obj_each(g, o)) {
        if (o->ID != OBJID_PULLEYBLOCK || !o->state) continue;

        pulleyblock_s *p1 = (pulleyblock_s *)o->mem;

        for (obj_each(g, i)) {
            if (i->ID == OBJID_PULLEYBLOCK &&
                !i->state &&
                o->subID == i->subID) {
                pulleyblock_s *p2              = (pulleyblock_s *)i->mem;
                p2->oparent                    = o;
                p1->children[p1->n_children++] = i;
            }
        }
    }
}

void pulleyblock_on_update(g_s *g, obj_s *o);
void pulleyblock_on_update_parent(g_s *g, obj_s *o);
void pulleyblock_on_hook(g_s *g, obj_s *o, i32 hooked);
void pulleyblock_on_draw(g_s *g, obj_s *o, v2_i32 cam);

obj_s *pulleyblock_create(g_s *g, map_obj_s *mo)
{
    obj_s         *o = obj_create(g);
    pulleyblock_s *s = (pulleyblock_s *)o->mem;
    o->ID            = OBJID_PULLEYBLOCK;
    o->pos.x         = mo->x;
    o->pos.y         = mo->y;
    o->w             = mo->w;
    o->h             = mo->h;
    o->on_hook       = pulleyblock_on_hook;
    o->on_draw       = pulleyblock_on_draw;
    o->flags =
        OBJ_FLAG_SOLID |
        OBJ_FLAG_HOOKABLE |
        OBJ_FLAG_CLIMBABLE;
    o->ropeobj.m_q12   = Q_12(1.0);
    o->subID           = map_obj_i32(mo, "refID");
    i32 l_rope         = map_obj_i32(mo, "l_rope");
    s->y_rope          = o->pos.y - l_rope * 16;
    o->render_priority = RENDER_PRIO_DEFAULT_OBJ + 1;
    o->editorID        = mo->ID;
    return o;
}

void pulleyblock_load_parent(g_s *g, map_obj_s *mo)
{
    obj_s         *o = pulleyblock_create(g, mo);
    pulleyblock_s *s = (pulleyblock_s *)o->mem;
    o->on_update     = pulleyblock_on_update_parent;
    o->state         = 1;
    s->y0            = mo->y - map_obj_i32(mo, "dt_y_up");
    s->y1            = mo->y + map_obj_i32(mo, "dt_y_down");
}

void pulleyblock_load_child(g_s *g, map_obj_s *mo)
{
    obj_s         *o = pulleyblock_create(g, mo);
    pulleyblock_s *s = (pulleyblock_s *)o->mem;
    o->on_update     = pulleyblock_on_update;
    s->movsign       = map_obj_bool(mo, "reverse") ? -1 : +1;
}

void pulleyblock_on_draw(g_s *g, obj_s *o, v2_i32 cam)
{
    pulleyblock_s *s   = (pulleyblock_s *)o->mem;
    gfx_ctx_s      ctx = gfx_ctx_display();
    v2_i32         p   = v2_i32_add(o->pos, cam);
    p.x &= ~1;
    p.y &= ~1;
    i32     tx     = o->w >> 4;
    i32     ty     = o->h >> 4;
    rec_i32 rrope1 = {p.x + (o->w >> 1) - 2, s->y_rope + cam.y, 4, o->pos.y - s->y_rope};
    gfx_rec_fill(ctx, rrope1, PRIM_MODE_BLACK);
    render_tile_terrain_block(ctx, p, tx, ty, TILE_TYPE_BRIGHT_STONE);

    // render rope wrap tiles
    i32      tilex = 1;
    texrec_s tr    = asset_texrec(TEXID_TILESET_TERRAIN, 0, 0, 32, 32);
    i32      posx  = p.x + ((tx << 3)) - 16;

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

        tr.y      = (tilex + (tiley) * 12) << 5;
        v2_i32 pr = {posx, p.y + (y << 4) - 8};
        gfx_spr(ctx, tr, pr, 0, 0);
    }
}

void pulleyblock_on_update(g_s *g, obj_s *o)
{
    pulleyblock_s *s = (pulleyblock_s *)o->mem;

    i32    f                = 0;
    obj_s *ohero            = obj_get_owl(g);
    bool32 hero_standing_on = 0;

    if (ohero && (overlap_rec(obj_aabb(o), obj_rec_bottom(ohero)) || ohero->linked_solid.o == o)) {
        f += Q_VOBJ(0.4);
        hero_standing_on = 1;
    }

    if (!hero_standing_on && o->substate) { // is hooked
        grapplinghook_s *gh = &g->ghook;
        v2_i32           v  = {0, 1};
        f -= grapplinghook_f_at_obj_proj(gh, o, v);
    }

    if (f) { // apply force to parent
        obj_s *oparent = 0;

        if (o->state) { // is parent
            oparent = o;
        } else { // is child
            oparent = s->oparent;
            f *= s->movsign;
        }

        oparent->v_q12.y += f >> 5;
        oparent->v_q12.y = clamp_sym_i32(oparent->v_q12.y, Q_VOBJ(4.0));
    }
}

void pulleyblock_on_update_parent(g_s *g, obj_s *o)
{
    pulleyblock_s *s = (pulleyblock_s *)o->mem;
    pulleyblock_on_update(g, o);

    obj_vy_q8_mul(o, Q_8(0.95));
    o->subpos_q12.y += o->v_q12.y;
    // moves in 2 px increments only
    i32 tm = clamp_i32(o->pos.y + ((o->subpos_q12.y >> 12) & ~1), s->y0, s->y1) - o->pos.y;
    o->subpos_q12.y &= 0x1FFF;
    obj_move(g, o, 0, tm);

    if ((o->v_q12.y > 0 && o->pos.y == s->y1) ||
        (o->v_q12.y < 0 && o->pos.y == s->y0)) {
        o->v_q12.y      = 0;
        o->subpos_q12.y = 0;
    }
    o->ropeobj.v_q12.y = o->v_q12.y;

    for (i32 n = 0; n < s->n_children; n++) {
        obj_s         *i  = s->children[n];
        pulleyblock_s *si = (pulleyblock_s *)i->mem;
        obj_move(g, i, 0, si->movsign * tm);
        i->ropeobj.v_q12.y = si->movsign * o->v_q12.y;
    }
}

void pulleyblock_on_hook(g_s *g, obj_s *o, i32 hooked)
{
    o->substate = hooked;
}