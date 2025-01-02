// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "grapplinghook.h"
#include "game.h"

i32 grapplinghook_step(g_s *g, grapplinghook_s *h, i32 sx, i32 sy);

void grapplinghook_update(g_s *g, grapplinghook_s *h)
{
    switch (h->state) {
    case GRAPPLINGHOOK_FLYING: {
        h->p_q8 = v2_i16_add(h->p_q8, h->v_q8);
        h->v_q8.y += 60;

        i32 dx = h->p_q8.x >> 8;
        i32 dy = h->p_q8.y >> 8;
        h->p_q8.x &= 0xFF;
        h->p_q8.y &= 0xFF;

        i32 px = +abs_i32(dx);
        i32 py = -abs_i32(dy);
        i32 sx = +sgn_i32(dx);
        i32 sy = +sgn_i32(dy);
        i32 e  = px + py;
        i32 x  = 0;
        i32 y  = 0;

        while (x != dx || y != dy) {
            i32 e2 = e << 1;
            if (e2 >= py) {
                e += py;
                x += sx;
                i32 r = grapplinghook_step(g, h, sx, 0);
                if (r) break;
            }
            if (e2 <= px) {
                e += px;
                y += sy;
                i32 r = grapplinghook_step(g, h, 0, sy);
                if (r) break;
            }
        }
    } break;
    case GRAPPLINGHOOK_HOOKED_SOLID: {

    } break;
    case GRAPPLINGHOOK_HOOKED_TERRAIN: {

    } break;
    case GRAPPLINGHOOK_HOOKED_OBJ: {

    } break;
    }
}

i32 grapplinghook_step(g_s *g, grapplinghook_s *h, i32 sx, i32 sy)
{
    v2_i32 p = h->p;
    p.x += sx;
    p.y += sy;
    if (tile_map_solid_pt(g, p.x, p.y)) {
        h->state = GRAPPLINGHOOK_HOOKED_TERRAIN;
        return h->state;
    }

    for (obj_each(g, it)) {
        if (it->mass) {
            h->state        = GRAPPLINGHOOK_HOOKED_SOLID;
            h->o2           = obj_handle_from_obj(it);
            h->solid_offs.x = h->p.x - it->pos.x;
            h->solid_offs.y = h->p.y - it->pos.y;
            return h->state;
        }

        switch (it->ID) {
        default: break;
        }
    }

    ropenode_move(g, &h->rope, h->rn, sx, sy);
    h->p = p;
    return 0;
}