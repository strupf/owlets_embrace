// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "pathmovement.h"

static pathnode_s *next_pathnode(pathnode_s *comingfrom, pathnode_s *crossed)
{
        if (crossed->next == comingfrom) return crossed->prev;
        if (crossed->prev == comingfrom) return crossed->next;
        ASSERT(0);
        return NULL;
}

static void path_circle(pathmover_s *p)
{
        p->x = p->x % p->circumference;
        if (p->x < 0) p->x += p->circumference;
}

static void path_polyline(pathmover_s *p)
{
        while (1) {

                i32         l        = v2_distance(p->from->p, p->to->p);
                pathnode_s *crossed  = NULL;
                pathnode_s *nextnode = NULL;

                if (p->x < 0) {
                        crossed  = p->from;
                        nextnode = next_pathnode(p->to, p->from);
                } else if (p->x >= l) {
                        crossed  = p->to;
                        nextnode = next_pathnode(p->from, p->to);
                } else break;

                if (!nextnode) {
                        p->from = crossed;
                        p->to   = crossed->next ? crossed->next : crossed->prev;
                        p->x    = 0;
                        if (p->movetype == PATH_LINE_STOP_END) {
                                p->v = 0;
                        } else if (p->movetype == PATH_LINE_PINGPONG) {
                                p->v = ABS(p->v);
                        }
                        break;
                }

                i32 ll = 0;
                if (crossed == p->from) {
                        p->to   = p->from;
                        p->from = nextnode;
                        ll      = v2_distance(p->to->p, p->from->p);
                        p->x += ll;
                } else if (crossed == p->to) {
                        p->from = p->to;
                        p->to   = nextnode;
                        ll      = v2_distance(p->to->p, p->from->p);
                        p->x -= l;
                }

                if (0 <= p->x && p->x < ll) break;
        }
}

void path_update(pathmover_s *p)
{
        p->x += p->v;
        switch (p->pathtype) {
        case PATH_TYPE_CIRC:
                path_circle(p);
                break;
        case PATH_TYPE_LINE:
                path_polyline(p);
                break;
        }
}

v2_i32 path_pos(pathmover_s *p)
{
        switch (p->pathtype) {
        case PATH_TYPE_CIRC: {
                i32    a   = (Q16_ANGLE_TURN * p->x) / p->circumference;
                v2_i32 res = {
                    p->circcenter.x + ((sin_q16(a) * p->circr) >> 16),
                    p->circcenter.y + ((cos_q16(a) * p->circr) >> 16)};
                return res;
        }
        case PATH_TYPE_LINE: {
                i32    l   = v2_distance(p->from->p, p->to->p);
                v2_i32 res = {
                    p->from->p.x + (p->to->p.x - p->from->p.x) * p->x / l,
                    p->from->p.y + (p->to->p.y - p->from->p.y) * p->x / l,
                };
                return res;
        }
        }
        ASSERT(0);
        return (v2_i32){0};
}