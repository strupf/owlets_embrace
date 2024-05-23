// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "collider.h"
#include "game.h"

bool32 map_grid_solid(collider_game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 ri;
    if (!intersect_rec(r, rgrid, &ri)) return 0;

    i32 px0 = ri.x;
    i32 py0 = ri.y;
    i32 px1 = ri.x + ri.w - 1;
    i32 py1 = ri.y + ri.h - 1;
    i32 tx0 = px0 >> 4;
    i32 ty0 = py0 >> 4;
    i32 tx1 = px1 >> 4;
    i32 ty1 = py1 >> 4;

    for (i32 ty = ty0; ty <= ty1; ty++) {
        i32 y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        i32 y1 = (ty == ty1 ? py1 & 15 : 15);

        for (i32 tx = tx0; tx <= tx1; tx++) {
            i32 c = g->tiles[tx + ty * g->tiles_x];
            if (!TILE_IS_SHAPE(c)) continue;
            if (TILE_IS_BLOCK(c)) return 1;
            i32 x0 = (tx == tx0 ? px0 & 15 : 0);
            i32 x1 = (tx == tx1 ? px1 & 15 : 15);
            u32 mk = (0xFFFF >> x0) & (0xFFFF << (15 - x1));
            for (i32 py = y0; py <= y1; py++)
                if (g_tile_px[c][py] & mk) return 1;
        }
    }
    return 0;
}

bool32 map_solid(collider_game_s *g, collider_s *c, rec_i32 r, i32 m)
{
    if (map_grid_solid(g, r)) return 1;

    for (i32 n = 0; n < g->n_colls; n++) {
        collider_s *cc = &g->colls[n];
        if (c == cc) continue;
        if (cc->m == 0) continue;
        if (m <= cc->m && overlap_rec(r, cc->r)) return 1;
    }
    return 0;
}

i32 collider_move_x(collider_game_s *g, collider_s *c, i32 dx, bool32 slide, i32 mpush)
{
    if (dx == 0) return 0;
    i32 m = mpush ? mpush : c->m;

    rec_i32 checkr = dx == 1 ? (rec_i32){c->r.x + c->r.w, c->r.y, 1, c->r.h}
                             : (rec_i32){c->r.x - 1, c->r.y, 1, c->r.h};

    if ((c->f & COLLIDER_COLLIDE_MAP) && map_solid(g, c, checkr, m)) {
        if (!slide || !(c->f && COLLIDER_SIDESTEP)) {
            c->bump |= dx == 1 ? COLLIDER_BUMP_X_POS : COLLIDER_BUMP_X_NEG;
            return 0;
        }
        rec_i32 r1       = {c->r.x + dx, c->r.y - 1, c->r.w, c->r.h};
        rec_i32 r2       = {c->r.x + dx, c->r.y + 1, c->r.w, c->r.h};
        bool32  could_r1 = (c->f & COLLIDER_CLIMB_SLOPES) && !map_solid(g, c, r1, m);
        bool32  could_r2 = !map_solid(g, c, r2, m);

        if (!(could_r1 && collider_move_y(g, c, -1, 0, m)) &&
            !(could_r2 && collider_move_y(g, c, +1, 0, m))) {
            c->bump |= dx == 1 ? COLLIDER_BUMP_X_POS : COLLIDER_BUMP_X_NEG;
            return 0;
        }
    }

    checkr = dx == 1 ? (rec_i32){c->r.x + c->r.w, c->r.y, 1, c->r.h}
                     : (rec_i32){c->r.x - 1, c->r.y, 1, c->r.h};

    if ((c->f & COLLIDER_COLLIDE_MAP) && map_solid(g, c, checkr, m)) {
        c->bump |= dx == 1 ? COLLIDER_BUMP_X_POS : COLLIDER_BUMP_X_NEG;
        return 0;
    }

    rec_i32 cr = c->r;
    c->r.x += dx;

    if (0 < c->m) {
        for (i32 n = 0; n < g->n_colls; n++) {
            collider_s *cc = &g->colls[n];
            if (c == cc) continue;
            bool32 linked  = 0;
            bool32 pushed  = 0;
            bool32 carried = 0;

            if (c->m > cc->m) {
                pushed = overlap_rec(c->r, cc->r);
            }
            if (c->m >= cc->m) {
                rec_i32 ccfeet = {cc->r.x, cc->r.y, cc->r.w, cc->r.h + 1};
                carried        = overlap_rec(cr, ccfeet);
            }
            if (linked || pushed || carried) {
                collider_move_x(g, cc, dx, 1, pushed ? m : 0);
            }
        }
    }

    if (c->f & COLLIDER_GLUE_GROUND) {
        rec_i32 rg1 = {c->r.x, c->r.y, c->r.w, c->r.h + 1};
        rec_i32 rg2 = {c->r.x, c->r.y, c->r.w, c->r.h + 2};
        if (!map_solid(g, c, rg1, c->m) && map_solid(g, c, rg2, c->m)) {
            collider_move_y(g, c, +1, 0, 0);
        }
    }

    return 1;
}

i32 collider_move_y(collider_game_s *g, collider_s *c, i32 dy, bool32 slide, i32 mpush)
{
    if (dy == 0) return 0;
    i32 m = mpush ? mpush : c->m;

    rec_i32 checkr = dy == 1 ? (rec_i32){c->r.x, c->r.y + c->r.h, c->r.w, 1}
                             : (rec_i32){c->r.x, c->r.y - 1, c->r.w, 1};

    if ((c->f & COLLIDER_COLLIDE_MAP) && map_solid(g, c, checkr, m)) {
        if (!slide || !(c->f && COLLIDER_SIDESTEP)) {
            c->bump |= dy == 1 ? COLLIDER_BUMP_Y_POS : COLLIDER_BUMP_Y_NEG;
            return 0;
        }
        rec_i32 r1       = {c->r.x - 1, c->r.y + dy, c->r.w, c->r.h};
        rec_i32 r2       = {c->r.x + 1, c->r.y + dy, c->r.w, c->r.h};
        bool32  could_r1 = !map_solid(g, c, r1, m);
        bool32  could_r2 = !map_solid(g, c, r2, m);

        if (!(could_r1 && collider_move_x(g, c, -1, 0, m)) &&
            !(could_r2 && collider_move_x(g, c, +1, 0, m))) {
            c->bump |= dy == 1 ? COLLIDER_BUMP_Y_POS : COLLIDER_BUMP_Y_NEG;
            return 0;
        }
    }

    checkr = dy == 1 ? (rec_i32){c->r.x, c->r.y + c->r.h, c->r.w, 1}
                     : (rec_i32){c->r.x, c->r.y - 1, c->r.w, 1};

    if ((c->f & COLLIDER_COLLIDE_MAP) && map_solid(g, c, checkr, m)) {
        c->bump |= dy == 1 ? COLLIDER_BUMP_Y_POS : COLLIDER_BUMP_Y_NEG;
        return 0;
    }

    rec_i32 cr = c->r;
    c->r.y += dy;

    if (0 < c->m) {
        for (i32 n = 0; n < g->n_colls; n++) {
            collider_s *cc = &g->colls[n];
            if (c == cc) continue;
            bool32 linked  = 0;
            bool32 pushed  = 0;
            bool32 carried = 0;

            if (m > cc->m) {
                pushed = overlap_rec(c->r, cc->r);
            }
            if (c->m >= cc->m) {
                rec_i32 ccfeet = {cc->r.x, cc->r.y, cc->r.w, cc->r.h + 1};
                carried        = overlap_rec(cr, ccfeet);
            }

            if (linked || pushed || carried) {
                collider_move_y(g, cc, dy, 1, pushed ? m : 0);
            }
        }
    }

    return 1;
}