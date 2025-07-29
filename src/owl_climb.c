// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl.h"

void owl_climb(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h     = (owl_s *)o->heap;
    o->bumpflags = 0;

    if (inps_btn_jp(inp, INP_A)) {
        owl_jump_ground(g, o);
        owl_cancel_climb(g, o);
    } else {
        if (h->climb_camx_smooth < 255) {
            h->climb_camx_smooth++;
        }

        switch (h->climb) {
        case OWL_CLIMB_LADDER: {
            owl_climb_ladder(g, o, inp);
            break;
        }
        }
    }
}

void owl_climb_ladder(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h       = (owl_s *)o->heap;
    i32    dp_x    = inps_x(inp);
    i32    dp_y    = inps_y(inp);
    i32    to_move = dp_y ? 2 : 0;

    for (i32 n = 0; n < to_move; n++) {
        bool32 blocked = map_blocked(g, obj_rec_y_leading(o, dp_y));
        if (blocked && 0 < dp_y) { // back on ground
            owl_cancel_climb(g, o);
            break;
        }
        if (!blocked && owl_climb_still_on_ladder(g, o, 0, dp_y)) {
            obj_move(g, o, 0, dp_y);
            h->climb_anim -= dp_y;
        } else {
            break;
        }
    }
}

bool32 owl_climb_still_on_ladder(g_s *g, obj_s *o, i32 offx, i32 offy)
{
    owl_s *h = (owl_s *)o->heap;
    if (h->climb != OWL_CLIMB_LADDER) return 0;

    i32 px = o->pos.x + (OWL_W >> 1);
    i32 py = o->pos.y + o->h - OWL_CLIMB_LADDER_SNAP_Y + offy;

    if (px + offx != h->climb_ladderx) return 0;
    if (py < 0 || g->pixel_y <= py) return 0;

    i32 t = g->tiles[(px >> 4) + (py >> 4) * g->tiles_x].shape;
    if (t != TILE_LADDER) return 0;
    return 1;
}

bool32 owl_climb_still_on_wall(g_s *g, obj_s *o, i32 offx, i32 offy)
{
    return 1;
}

bool32 owl_climb_try_snap_to_ladder(g_s *g, obj_s *o)
{
    owl_s *h  = (owl_s *)o->heap;
    i32    py = o->pos.y + o->h - OWL_CLIMB_LADDER_SNAP_Y;
    if (py < 0 || g->pixel_y <= py) return 0;

    i32 ty           = py >> 4;
    i32 px           = o->pos.x + (OWL_W >> 1);
    i32 tx1          = max_i32(0, px - OWL_CLIMB_LADDER_SNAP_X_SYM) >> 4;
    i32 tx2          = min_i32(g->pixel_x - 1, px + OWL_CLIMB_LADDER_SNAP_X_SYM) >> 4;
    i32 pos_ladder_x = -1; // pixel position of the owl if snapped
    i32 x_move       = 64; // random threshold to figure out the closest ladder

    for (i32 tx = tx1; tx <= tx2; tx++) {
        i32 t = g->tiles[tx + ty * g->tiles_x].shape;
        if (t == TILE_LADDER) {
            i32 plx = (tx << 4) + 8;
            i32 m   = abs_i32(px - plx);
            if (m < x_move) {
                x_move       = m;
                pos_ladder_x = plx;
            }
        }
    }

    if (pos_ladder_x < 0) return 0; // no ladder found

    // snapping: first move sideways to ladder,
    // then 1 pixel up (to avoid being on the ground)
    i32     dtx   = pos_ladder_x - px;
    rec_i32 rside = {o->pos.x + (dtx < 0 ? dtx : 0), o->pos.y - 1, OWL_W + abs_i32(dtx), o->h};
    rec_i32 rup   = {o->pos.x + dtx, o->pos.y - 1, OWL_W, o->h};

    if (!map_blocked(g, rside) && !map_blocked(g, rup)) {
        h->climb_ladderx                 = pos_ladder_x;
        h->climb                         = OWL_CLIMB_LADDER;
        h->climb_anim                    = 0;
        o->v_q12.x                       = 0;
        o->v_q12.y                       = 0;
        o->subpos_q12.x                  = 0;
        o->subpos_q12.y                  = 0;
        h->sprint                        = 0;
        h->ground_sprint_doubletap_ticks = 0;
        h->climb_camx_smooth             = 1;
        h->climb_from_x                  = px;
        owl_on_touch_ground(g, o);
        owl_cancel_air(g, o);
        owl_cancel_ground(g, o);
        owl_cancel_attack(g, o);
        obj_move(g, o, dtx, 0);
        obj_move(g, o, 0, -1);
        return 1;
    }
    return 0;
}