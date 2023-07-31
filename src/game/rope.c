/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#include "rope.h"

void rope_init(rope_s *r)
{
        for (int n = 2; n < NUM_ROPE_NODES - 1; n++) {
                r->nodesraw[n].next = &r->nodesraw[n + 1];
        }
        r->nodesraw[NUM_ROPE_NODES - 1].next = NULL;
        r->pool                              = &r->nodesraw[2];

        ropenode_s *rh = &r->nodesraw[0];
        ropenode_s *rt = &r->nodesraw[1];
        rh->next       = rt;
        rh->prev       = NULL;
        rt->prev       = rh;
        rt->next       = NULL;
        r->head        = rh;
        r->tail        = rt;
        r->len_max     = 200;
        r->damping_q8  = 620;
        r->spring_q8   = 220;
}

ropenode_s *ropenode_insert(rope_s *r, ropenode_s *r1, ropenode_s *r2,
                            v2_i32 p)
{
        ASSERT((r1->next == r2 && r2->prev == r1) ||
               (r2->next == r1 && r1->prev == r2));
        ASSERT(r->pool);
        ropenode_s *rn = r->pool;
        r->pool        = rn->next;
        rn->p          = p;
        if (r1->next == r2 && r2->prev == r1) {
                r1->next = rn;
                r2->prev = rn;
                rn->prev = r1;
                rn->next = r2;
        } else if (r2->next == r1 && r1->prev == r2) {
                r2->next = rn;
                r1->prev = rn;
                rn->prev = r2;
                rn->next = r1;
        } else {
                ASSERT(0);
        }
        return rn;
}

void ropenode_delete(rope_s *r, ropenode_s *rn)
{
        ropenode_s *prev = rn->prev;
        ropenode_s *next = rn->next;
        ASSERT(prev && next); // can only delete nodes in the middle
        ASSERT(prev->next == rn && next->prev == rn);
        prev->next = next;
        next->prev = prev;
        rn->next   = r->pool;
        r->pool    = rn;
}

// checks collinearity of a point and returns whether to add the point
// and at which position
int rope_points_collinearity(v2_arr *pts, v2_i32 c)
{
        for (int n = 0; n < v2_arrlen(pts) - 1; n++) {
                v2_i32 a   = v2_arrat(pts, n);
                v2_i32 ac  = v2_sub(c, a);
                u32    dac = v2_lensq(ac);
                for (int i = n + 1; i < v2_arrlen(pts); i++) {
                        v2_i32 b  = v2_arrat(pts, i);
                        v2_i32 ab = v2_sub(b, a);
                        if (v2_crs(ac, ab) != 0) continue;
                        // at this point a, b, c are collinear which doesn't
                        // work with our convex hull algorithm
                        u32 dab = v2_lensq(ab);
                        u32 dbc = v2_distancesq(c, b);

                        if (dac > dab && dac > dbc)
                                return i; // b is midpoint, replace at i
                        if (dbc > dac && dbc > dab)
                                return n; // a is midpoint, replace at n
                        if (dab > dac && dab > dbc)
                                return -1; // c is midpoint, don't add
                }
        }
        return v2_arrlen(pts);
}

void rope_points_in_tris(tri_i32 t1, tri_i32 t2, ropecollider_s *coll, v2_arr *pts)
{
        ASSERT(v2_crs(v2_sub(t1.p[2], t1.p[0]), v2_sub(t1.p[1], t1.p[0])) != 0);
        ASSERT(v2_crs(v2_sub(t2.p[2], t2.p[0]), v2_sub(t2.p[1], t2.p[0])) != 0);

        for (ropecollider_s *c = coll; c; c = c->next) {
                for (int n = 0; n < c->n; n++) {
                        tri_i32 t = c->tris[n];
                        ASSERT(v2_crs(v2_sub(t.p[2], t.p[0]), v2_sub(t.p[1], t.p[0])) != 0);
                        for (int i = 0; i < 3; i++) {
                                v2_i32 p = t.p[i];
                                if (!overlap_tri_pnt_incl(t1, p) ||
                                    !overlap_tri_pnt_incl(t2, p) ||
                                    v2_arrcontains(pts, p)) continue;
                                int k = rope_points_collinearity(pts, p);
                                if (k == -1) continue; // don't add
                                v2_arrput(pts, p, k);  // add or overwrite
                        }
                }
        }
}

// pts needs to contain pfrom and pto
void rope_build_convex_hull(v2_arr *pts, v2_i32 pfrom, v2_i32 pto,
                            i32 dir, v2_arr *newnodes)
{
        int l = 0, p = 0;
        do {
                v2_i32 pc = v2_arrat(pts, p); // current point
                if (!v2_eq(pc, pfrom) && !v2_eq(pc, pto)) {
                        v2_arradd(newnodes, pc);
                }

                int    q  = (p + 1) % v2_arrlen(pts);
                v2_i32 pn = v2_arrat(pts, q); // next point
                foreach_v2_arr (pts, it) {
                        if (it.i == p || it.i == q) continue;
                        i32 v = v2_crs(v2_sub(pn, it.e), v2_sub(it.e, pc));
                        if ((v >= 0 && dir >= 0) || (v <= 0 && dir <= 0)) {
                                q  = it.i;
                                pn = it.e;
                        }
                }
                p = q;
        } while (p != l);
}

void ropenode_on_moved(rope_s *r, ropecollider_s *c, ropenode_s *rn,
                       v2_i32 p1, v2_i32 p2, ropenode_s *rn_anchor, tri_i32 subtri)
{
        ASSERT((rn->next == rn_anchor && rn_anchor->prev == rn) ||
               (rn->prev == rn_anchor && rn_anchor->next == rn));
        if (v2_eq(p1, p2)) return;

        v2_i32 p0  = rn_anchor->p; // anchor
        i32    dir = v2_crs(v2_sub(p1, p0), v2_sub(p2, p0));
        if (dir == 0) return;

        os_spmem_push();

        v2_arr *hull = v2_arrcreate(64, os_spmem_alloc);
        v2_arr *pts  = v2_arrcreate(64, os_spmem_alloc);
        v2_arradd(pts, p0);
        v2_arradd(pts, p2);
        tri_i32 tri = {p0, p1, p2};

        // add points inside the arc
        rope_points_in_tris(tri, subtri, c, pts);
        if (v2_arrlen(pts) == 2) {
                os_spmem_pop();
                return;
        }

        rope_build_convex_hull(pts, p0, p2, dir, hull);

        // at this point p_ropearc contains the "convex hull" when moving
        // from p1 around p0 to p2. Points in array are in the correct order
        ropenode_s *rc = rn;
        foreach_v2_arr (hull, it) {
                ropenode_s *rr = ropenode_insert(r, rn_anchor, rc, it.e);
                rc             = rr;
        }
        os_spmem_pop();
}

void ropenode_move(rope_s *r, ropecollider_s *c, ropenode_s *rn, v2_i32 dt)
{
        v2_i32 p_old = rn->p;
        v2_i32 p_new = v2_add(p_old, dt);
        rn->p        = p_new;

        if (rn->next) {
                tri_i32 tri = {p_old, p_new, rn->next->p};
                ropenode_on_moved(r, c, rn, p_old, p_new, rn->next, tri);
        }
        if (rn->prev) {
                tri_i32 tri = {p_old, p_new, rn->prev->p};
                ropenode_on_moved(r, c, rn, p_old, p_new, rn->prev, tri);
        }
}

void rope_moved_by_solid(rope_s *r, ropecollider_s *c, ropecollider_s *solid, v2_i32 dt)
{
        os_spmem_push();
        v2_arr *points = v2_arrcreate(256, os_spmem_alloc);
        for (int n = 0; n < solid->n; n++) {
                tri_i32 tri = solid->tris[n];
                if (!v2_arrcontains(points, tri.p[0]))
                        v2_arradd(points, tri.p[0]);
                if (!v2_arrcontains(points, tri.p[1]))
                        v2_arradd(points, tri.p[1]);
                if (!v2_arrcontains(points, tri.p[2]))
                        v2_arradd(points, tri.p[2]);
        }

        foreach_v2_arr (points, it) {
                v2_i32      p_beg  = it.e;
                v2_i32      p_end  = v2_add(p_beg, dt);
                v2_i32      p_pst  = v2_sub(p_beg, dt);
                lineseg_i32 ls_mov = {p_beg, p_end};
                lineray_i32 lr_pst = {p_beg, p_pst};

                // first move all nodes which are directly hit by a
                // solid vertex
                for (ropenode_s *r1 = r->head, *r2 = r->head->next; r2;
                     r1 = r2, r2 = r2->next) {
                        if (v2_eq(p_end, r1->p) ||
                            !overlap_lineseg_pnt_incl(ls_mov, r1->p)) {
                                continue;
                        }

                        v2_i32 rnold = r1->p;
                        r1->p        = p_end;
                        if (r1->prev) {
                                tri_i32 tri = {r1->prev->p, rnold, p_end};
                                ropenode_on_moved(r, c, r1,
                                                  rnold, p_end,
                                                  r1->prev, tri);
                        }
                        if (r1->next) {
                                tri_i32 tri = {r1->next->p, rnold, p_end};
                                ropenode_on_moved(r, c, r1,
                                                  rnold, p_end,
                                                  r1->next, tri);
                        }
                }

                // now check penetrating segments
                for (ropenode_s *r1 = r->head, *r2 = r->head->next; r2;
                     r1 = r2, r2 = r2->next) {
                        lineseg_i32 ls = {r1->p, r2->p};

                        // shall not overlap "piston" ray
                        if (overlap_lineseg_lineray_excl(ls, lr_pst))
                                continue;
                        // moving line segment of "piston" should
                        // overlap the rope segment
                        if (!overlap_lineseg_excl(ls, ls_mov) &&
                            !overlap_lineseg_pnt_excl(ls, p_beg))
                                continue;

                        ropenode_s *ri = ropenode_insert(r, r1, r2, p_end);

                        // only consider points which are on the
                        // "side of penetration"
                        tri_i32 tri = {r1->p, r2->p, p_end};
                        ropenode_on_moved(r, c, ri, p_beg, p_end, r1, tri);
                        ropenode_on_moved(r, c, ri, p_beg, p_end, r2, tri);
                }
        }

        os_spmem_pop();
}

void tighten_ropesegment(rope_s *r, ropecollider_s *c,
                         ropenode_s *rp, ropenode_s *rc, ropenode_s *rn)
{
        ASSERT(rp->next == rc && rn->prev == rc &&
               rc->next == rn && rc->prev == rp);
        v2_i32 pprev = rp->p;
        v2_i32 pcurr = rc->p;
        v2_i32 pnext = rn->p;
        if (v2_eq(pcurr, pprev) || v2_eq(pcurr, pnext)) {
                ropenode_delete(r, rc);
                return;
        }

        // check if current ropenode is still bend around a convex triangle
        // test if v and u of triangle point into the relevant arc
        //
        //     curr
        //      o----------> next
        //     /|\
        //    / | \
        //   /  |__\u
        //  /   v
        // prev
        //
        v2_i32 c_to_p = v2_sub(pprev, pcurr);   // from curr to prev
        v2_i32 c_to_n = v2_sub(pnext, pcurr);   // from curr to next
        i32    z      = v2_crs(c_to_p, c_to_n); // benddirection
        for (ropecollider_s *cc = c; cc; cc = cc->next) {
                for (int n = 0; n < cc->n; n++) {
                        tri_i32 tri = cc->tris[n];
                        for (int i = 0; i < 3; i++) {
                                if (!v2_eq(pcurr, tri.p[i])) continue;
                                v2_i32 u      = tri.p[(i + 1) % 3];
                                v2_i32 v      = tri.p[(i + 2) % 3];
                                v2_i32 c_to_u = v2_sub(u, pcurr);
                                v2_i32 c_to_v = v2_sub(v, pcurr);
                                i32    s1     = v2_crs(c_to_p, c_to_u);
                                i32    s2     = v2_crs(c_to_n, c_to_u);
                                i32    t1     = v2_crs(c_to_p, c_to_v);
                                i32    t2     = v2_crs(c_to_n, c_to_v);
                                if ((z >= 0 &&
                                     s1 >= 0 && s2 <= 0 &&
                                     t1 >= 0 && t2 <= 0) ||
                                    (z <= 0 &&
                                     s1 <= 0 && s2 >= 0 &&
                                     t1 <= 0 && t2 >= 0))
                                        return; // convex vertex
                        }
                }
        }

        ropenode_delete(r, rc);
        os_spmem_push();
        v2_arr *hull = v2_arrcreate(16, os_spmem_alloc);
        v2_arr *pts  = v2_arrcreate(64, os_spmem_alloc);
        v2_arradd(pts, pprev);
        v2_arradd(pts, pnext);

        tri_i32 tri = {pprev, pcurr, pnext};
        rope_points_in_tris(tri, tri, c, pts);
        v2_arrdelq(pts, pcurr); // ignore vertex at p1
        if (v2_arrlen(pts) == 2) {
                os_spmem_pop();
                return;
        }

        rope_build_convex_hull(pts, pprev, pnext, z, hull); // TODO: verify

        ropenode_s *tmp = rp;
        foreach_v2_arr (hull, it) {
                ropenode_s *rr = ropenode_insert(r, tmp, rn, it.e);
                tmp            = rr;
        }

        os_spmem_pop();
}

void rope_update(rope_s *r, ropecollider_s *c)
{
        ropenode_s *rprev = r->head;
        ropenode_s *rcurr = r->head->next;
        ropenode_s *rnext = r->head->next->next;

        while (rprev && rcurr && rnext) {
                tighten_ropesegment(r, c, rprev, rcurr, rnext);
                rcurr = rnext;
                rprev = rcurr->prev;
                rnext = rcurr->next;
        }
}

bool32 rope_blocked(rope_s *r, ropecollider_s *c)
{
        for (ropecollider_s *cc = c; cc; cc = cc->next) {
                for (int n = 0; n < cc->n; n++) {
                        tri_i32 tri = cc->tris[n];
                        for (ropenode_s *r1 = r->head, *r2 = r->head->next; r2;
                             r1 = r2, r2 = r2->next) {
                                lineseg_i32 ls = {r1->p, r2->p};
                                if (overlap_tri_lineseg_excl(tri, ls)) {
                                        return 1;
                                }
                        }
                }
        }
        return 0;
}

u32 rope_length_q4(rope_s *r)
{
        ropenode_s *rn1 = r->head;
        ropenode_s *rn2 = r->head->next;
        u32         len = 0;
        do {
                v2_i32 dt = v2_sub(rn1->p, rn2->p);
                dt        = v2_shl(dt, 4);
                len += v2_len(dt);
                rn1 = rn2;
                rn2 = rn2->next;
        } while (rn2);
        return len;
}

v2_i32 rope_adjust_connected_vel(rope_s *r, ropenode_s *rn,
                                 v2_i32 subpos, v2_i32 vel)
{
        ASSERT(!rn->prev || !rn->next);
        ASSERT(rn == r->head || rn == r->tail);

        u32 len_q4     = rope_length_q4(r);
        u32 len_max_q4 = r->len_max << 4;
        if (len_q4 <= len_max_q4) return vel;

        ropenode_s *rprev = rn->next ? rn->next : rn->prev;
        ASSERT(rprev);

        v2_i32 ropedt    = v2_sub(rn->p, rprev->p);
        v2_i32 subpos_q4 = v2_shr(subpos, 4);
        v2_i32 dt_q4     = v2_add(v2_shl(ropedt, 4), subpos_q4);

        v2_i32 frope = {0};
        if (v2_dot(ropedt, vel) <= 0) {
                v2_i32 vzero = {0};
                v2_i32 vrad  = project_pnt_line(vel, vzero, dt_q4);
                v2_i32 fdamp = v2_q_mulr(vrad, r->damping_q8, 8);
                frope        = fdamp;
        }

        i32 dt_len = (i32)(len_q4 - len_max_q4);
        if (dt_len >= 1) {
                i32    fspring_scalar = q_mulr(dt_len, r->spring_q8, 8);
                v2_i32 fspring        = v2_setlen(dt_q4, fspring_scalar);
                frope                 = v2_add(frope, fspring);
        }

        v2_i32 vel_new = v2_sub(vel, frope);
        return vel_new;
}