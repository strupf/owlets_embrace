// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "owl/owl.h"

void owl_climb(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h     = (owl_s *)o->heap;
    o->bumpflags = 0;

    if (h->climb_tick < 255) {
        h->climb_tick++;
    }

    switch (h->climb) {
    case OWL_CLIMB_LADDER: {
        if (inps_btn_jp(inp, INP_A)) {
            owl_jump_ground(g, o);
            owl_cancel_climb(g, o);
        } else {
            owl_climb_ladder(g, o, inp);
        }
        break;
    }
    case OWL_CLIMB_WALL: {
        if (inps_btn_jp(inp, INP_A)) {
            if (owl_jump_wall(g, o, -o->facing)) {

                h->climb = OWL_CLIMB_WALLJUMP;
            }
            // owl_cancel_climb(g, o);
        } else {
            owl_climb_wall(g, o, inp);
        }
        break;
    }
    case OWL_CLIMB_WALLJUMP: {
        if (h->wallj_ticks) {
            h->wallj_ticks++;
            if (OWL_WALLJUMP_INIT_TICKS <= h->wallj_ticks) {
                h->wallj_ticks = 0;
                o->facing      = -o->facing;
                owl_cancel_climb(g, o);
                owl_walljump_execute(g, o);
            }
        }
        break;
    }
    }
}

void owl_climb_wall(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h         = (owl_s *)o->heap;
    i32    dp_x      = inps_x(inp);
    i32    dp_y      = inps_y(inp);
    i32    to_move   = 0;
    bool32 can_climb = 1;

    if (can_climb) {
        if (dp_y) {
            if (h->climb_move_acc < 255)
                h->climb_move_acc++;
            to_move = dp_y * (3 <= h->climb_move_acc ? 2 : 1);
        } else {
            h->climb_move_acc = 0;
        }
    } else if (0 < to_move && h->climb_slide_down < 255) {
        h->climb_slide_down++;
    }

    for (i32 n = abs_i32(to_move), sy = sgn_i32(to_move); n; n--) {
        if (map_blocked(g, obj_rec_y_leading(o, sy))) {
            if (0 < sy) { // back on ground
                owl_cancel_climb(g, o);
            }
            break;
        }

        if (owl_climb_still_on_wall(g, o, o->facing, 0, sy)) {
            obj_move(g, o, 0, sy);
            h->climb_anim -= sy;
        } else if (sy < 0) {
            owl_cancel_climb(g, o);
            o->v_q12.y = -Q_VOBJ(4.5);
        } else if (sy > 0) {
            obj_move(g, o, 0, 1);
            owl_cancel_climb(g, o);
            o->v_q12.y = +Q_VOBJ(0.0);
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

bool32 owl_climb_still_on_wall(g_s *g, obj_s *o, i32 facing, i32 offx, i32 offy)
{
    rec_i32 r = {o->pos.x + offx, o->pos.y + offy, o->w, o->h};
    if (map_blocked(g, r)) return 0;

    i32 y1 = o->pos.y + offy + 4;
    i32 y2 = o->pos.y + offy + o->h - 4;
    i32 x  = o->pos.x + offx + (0 < facing ? o->w : -1);

    for (i32 y = y1; y <= y2; y++) {
        if (!map_blocked_pt(g, x, y)) {
            return 0;
        }
    }
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
        owl_set_to_climb(g, o);
        h->climb_ladderx = pos_ladder_x;
        h->climb         = OWL_CLIMB_LADDER;
        h->climb_tick    = 1;
        h->climb_from_x  = px;
        owl_on_touch_ground(g, o);
        obj_move(g, o, dtx, 0);
        obj_move(g, o, 0, -1);
        return 1;
    }
    return 0;
}

void owl_set_to_climb(g_s *g, obj_s *o)
{
    owl_s *h         = (owl_s *)o->heap;
    h->climb_ladderx = 0;
    h->climb         = 0;
    h->climb_anim    = 0;
    h->climb_tick    = 0;
    h->climb_from_x  = 0;
    o->v_q12.x       = 0;
    o->v_q12.y       = 0;
    o->subpos_q12.x  = 0;
    o->subpos_q12.y  = 0;
    h->sprint        = 0;
    owl_cancel_air(g, o);
    owl_cancel_ground(g, o);
    owl_cancel_attack(g, o);
    owl_cancel_hook_aim(g, o);
}