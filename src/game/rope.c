// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "rope.h"
#include "game.h"

typedef struct {
        v2_i32 p;
        v2_i32 u;
        v2_i32 v;
} convex_vertex_s;

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
        r->len_max_q16 = r->len_max << 16;
        r->damping_q8  = 190;
        r->spring_q8   = 240;
}

ropenode_s *ropenode_insert(rope_s *r, ropenode_s *a, ropenode_s *b, v2_i32 p)
{
        ASSERT((a->next == b && b->prev == a) ||
               (b->next == a && a->prev == b));
        ASSERT(r->pool);
        if (v2_eq(p, (v2_i32){256, 176})) {
                int aa = 1;
        }
        ropenode_s *rn = r->pool;
        r->pool        = rn->next;
        rn->p          = p;
        if (a->next == b && b->prev == a) {
                a->next  = rn;
                b->prev  = rn;
                rn->prev = a;
                rn->next = b;
        } else if (b->next == a && a->prev == b) {
                b->next  = rn;
                a->prev  = rn;
                rn->prev = b;
                rn->next = a;
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
static int rope_points_collinearity(v2_arr *pts, v2_i32 c)
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

static void try_add_point_in_tri(convex_vertex_s p, tri_i32 t1, tri_i32 t2,
                                 v2_arr *pts)
{

        if (!overlap_tri_pnt_incl(t1, p.p) || !overlap_tri_pnt_incl(t2, p.p))
                return;
        for (int i = 0; i < 3; i++) {
                if (v2_eq(p.p, t1.p[i])) return;
                if (v2_eq(p.p, t2.p[i])) return;
        }
        // nasty with lots of calculations...
        // ONLY NEEDED FOR SOLID MOVEMENT though
        lineseg_i32 lu = {p.p, p.u};
        lineseg_i32 lv = {p.p, p.v};
        if (!overlap_tri_lineseg_excl(t1, lu) &&
            !overlap_tri_lineseg_excl(t1, lv) &&
            !overlap_tri_lineseg_excl(t2, lu) &&
            !overlap_tri_lineseg_excl(t2, lv)) return;

        if (v2_arrcontains(pts, p.p)) return;
        int k = rope_points_collinearity(pts, p.p);
        if (k == -1) return;    // don't add
        v2_arrput(pts, p.p, k); // add or overwrite
}

static void try_add_point_in_tri_(v2_i32 p, tri_i32 t1, tri_i32 t2, v2_arr *pts)
{
        if (!overlap_tri_pnt_incl(t1, p) || !overlap_tri_pnt_incl(t2, p)) return;
        for (int i = 0; i < 3; i++) {
                if (v2_eq(p, t1.p[i])) return;
                if (v2_eq(p, t2.p[i])) return;
        }

        if (v2_arrcontains(pts, p)) return;
        int k = rope_points_collinearity(pts, p);
        if (k == -1) return;  // don't add
        v2_arrput(pts, p, k); // add or overwrite
}

static void rope_points_in_tris(game_s *g, tri_i32 t1, tri_i32 t2, v2_arr *pts)
{
        ASSERT(v2_crs(v2_sub(t1.p[2], t1.p[0]), v2_sub(t1.p[1], t1.p[0])) != 0);
        ASSERT(v2_crs(v2_sub(t2.p[2], t2.p[0]), v2_sub(t2.p[1], t2.p[0])) != 0);

        v2_i32 pmin1 = v2_min(t1.p[0], v2_min(t1.p[1], t1.p[2]));
        v2_i32 pmin2 = v2_min(t2.p[0], v2_min(t2.p[1], t2.p[2]));
        v2_i32 pmax1 = v2_max(t1.p[0], v2_max(t1.p[1], t1.p[2]));
        v2_i32 pmax2 = v2_max(t2.p[0], v2_max(t2.p[1], t2.p[2]));
        i32    x1, y1, x2, y2;
        game_tile_bounds_minmax(g, v2_min(pmin1, pmin2), v2_max(pmax1, pmax2), &x1, &y1, &x2, &y2);
        for (int y = y1; y <= y2; y++) {
                for (int x = x1; x <= x2; x++) {
                        int t = g->tiles[x + y * g->tiles_x];
                        if (t == 0) continue;
                        tilecollider_s tc    = tilecolliders[t - 1];
                        tri_i32        ttri1 = translate_tri_xy(tc.tris[0], x * 16, y * 16);
                        tri_i32        ttri2 = translate_tri_xy(tc.tris[1], x * 16, y * 16);
                        for (int i = 0; i < 3; i++) {
                                convex_vertex_s v1 = {ttri1.p[i], ttri1.p[(i + 1) % 3], ttri1.p[(i + 2) % 3]};
                                convex_vertex_s v2 = {ttri2.p[i], ttri2.p[(i + 1) % 3], ttri2.p[(i + 2) % 3]};
                                try_add_point_in_tri(v1, t1, t2, pts);
                                try_add_point_in_tri(v2, t1, t2, pts);
                        }
                }
        }

        const obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        for (int n = 0; n < solids.n; n++) {
                const obj_s *o = solids.o[n];
                if (o->soliddisabled) continue;
                v2_i32 points[4];
                points_from_rec(obj_aabb(o), points);
                for (int i = 0; i < 4; i++) {
                        convex_vertex_s v1 = {points[i], points[(i + 1) % 4], points[(i + 3) % 4]};
                        try_add_point_in_tri(v1, t1, t2, pts);
                }
        }
}

// pts needs to contain pfrom and pto
static void rope_build_convex_hull(v2_arr *pts, v2_i32 pfrom, v2_i32 pto,
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

void ropenode_on_moved(game_s *g, rope_s *r, ropenode_s *rn,
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
        rope_points_in_tris(g, tri, subtri, pts);
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

void ropenode_move(game_s *g, rope_s *r, ropenode_s *rn, v2_i32 dt)
{
        v2_i32 p_old = rn->p;
        v2_i32 p_new = v2_add(p_old, dt);
        rn->p        = p_new;

        if (rn->next) {
                tri_i32 tri = {p_old, p_new, rn->next->p};
                ropenode_on_moved(g, r, rn, p_old, p_new, rn->next, tri);
        }
        if (rn->prev) {
                tri_i32 tri = {p_old, p_new, rn->prev->p};
                ropenode_on_moved(g, r, rn, p_old, p_new, rn->prev, tri);
        }
}
/*
 * bug (also see sketch) -> temporary solution added in points in tri (linesegs)
 * node is pushed by solid to the left
 * o <-o______
 *  \  |
 *   \ o <-- this one gets wrongly added to the convex hull (bc inside tri)
 *    \|     and ends up being inside the solid
 *     o______
 */
void rope_moved_by_solid(game_s *g, rope_s *r, obj_s *solid, v2_i32 dt)
{
        v2_i32 points[4];
        points_from_rec(obj_aabb(solid), points);

        // this is only for debugging purposes to rerun
        // this algorithm through the debugger on break points
        rope_s rcopy = *r;

        for (int n = 0; n < 4; n++) {
                v2_i32      p_beg  = points[n];
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
                                ropenode_on_moved(g, r, r1, rnold, p_end, r1->prev, tri);
                        }
                        if (r1->next) {
                                tri_i32 tri = {r1->next->p, rnold, p_end};
                                ropenode_on_moved(g, r, r1, rnold, p_end, r1->next, tri);
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
                        if (!(overlap_lineseg_incl(ls, ls_mov) &&
                              !v2_eq(r1->p, ls_mov.a) &&
                              !v2_eq(r1->p, ls_mov.b) &&
                              !v2_eq(r2->p, ls_mov.a) &&
                              !v2_eq(r2->p, ls_mov.b))) {
                                continue;
                        }

                        if (overlap_lineseg_pnt_excl(ls, p_end)) {
                                // point lies exactly on the rope segment
                                // between r1 and r2 and isn't necessary
                                continue;
                        }

                        ropenode_s *ri  = ropenode_insert(r, r1, r2, p_end);
                        // only consider points which are on the
                        // "side of penetration"
                        tri_i32     tri = {r1->p, r2->p, p_end};
                        ropenode_on_moved(g, r, ri, p_beg, p_end, r1, tri);
                        ropenode_on_moved(g, r, ri, p_beg, p_end, r2, tri);
                }
        }

#if 0
        // for debugging purposes
        // double check that no line segment overlaps the solid
        // after the routine above
        for (ropenode_s *r1 = r->head, *r2 = r->head->next; r2;
             r1 = r2, r2 = r2->next) {
                v2_i32      p1  = r1->p;
                v2_i32      p2  = r2->p;
                lineseg_i32 ls1 = {p1, p2};
                tri_i32     tris[2];
                rec_i32     rr = obj_aabb(solid);
                rr.x += dt.x;
                rr.y += dt.y;
                bool32 a = overlap_rec_lineseg_excl(rr, ls1);
                // ASSERT(!a);
                if (a) {
                        // rope_moved_by_solid(g, &rcopy, solid, dt);
                }
        }
#endif
}

static bool32 rope_vertex_convex(i32 z, v2_i32 p, v2_i32 u, v2_i32 v,
                                 v2_i32 curr, v2_i32 c_to_p, v2_i32 c_to_n)
{
        if (!v2_eq(curr, p)) return 0;
        v2_i32 c_to_u = v2_sub(u, curr);
        v2_i32 c_to_v = v2_sub(v, curr);
        i32    s1     = v2_crs(c_to_p, c_to_u);
        i32    s2     = v2_crs(c_to_n, c_to_u);
        i32    t1     = v2_crs(c_to_p, c_to_v);
        i32    t2     = v2_crs(c_to_n, c_to_v);
        return ((z >= 0 && s1 >= 0 && s2 <= 0 && t1 >= 0 && t2 <= 0) ||
                (z <= 0 && s1 <= 0 && s2 >= 0 && t1 <= 0 && t2 >= 0));
}

void tighten_ropesegment(game_s *g, rope_s *r,
                         ropenode_s *rp, ropenode_s *rc, ropenode_s *rn)
{
        ASSERT(rp->next == rc && rn->prev == rc &&
               rc->next == rn && rc->prev == rp);
        v2_i32 pprev = rp->p;
        v2_i32 pcurr = rc->p;
        v2_i32 pnext = rn->p;
        // check if the three points are collinear
        if (v2_crs(v2_sub(pprev, pcurr), v2_sub(pnext, pcurr)) == 0) {
                ropenode_delete(r, rc);
                return;
        }

        /* check if current ropenode is still bend around a convex triangle
         * test if v and u of triangle point into the relevant arc
         *
         *     curr
         *      o----------> next
         *     /|\
         *    / | \
         *   /  |__\u
         *  /   v
         * prev
         */
        v2_i32 c_to_p = v2_sub(pprev, pcurr);   // from curr to prev
        v2_i32 c_to_n = v2_sub(pnext, pcurr);   // from curr to next
        i32    z      = v2_crs(c_to_p, c_to_n); // benddirection

        tri_i32 trispan = {pprev, pcurr, pnext};
        i32     x1, y1, x2, y2;
        game_tile_bounds_tri(g, trispan, &x1, &y1, &x2, &y2);
        for (int y = y1; y <= y2; y++) {
                for (int x = x1; x <= x2; x++) {
                        int t = g->tiles[x + y * g->tiles_x];
                        if (t == 0) continue;
                        tilecollider_s tc    = tilecolliders[t - 1];
                        tri_i32        ttri1 = translate_tri_xy(tc.tris[0], x * 16, y * 16);
                        tri_i32        ttri2 = translate_tri_xy(tc.tris[1], x * 16, y * 16);
                        for (int i = 0; i < 3; i++) {
                                if (rope_vertex_convex(z, ttri1.p[i],
                                                       ttri1.p[(i + 2) % 3],
                                                       ttri1.p[(i + 1) % 3],
                                                       pcurr, c_to_p, c_to_n))
                                        return;
                                if (rope_vertex_convex(z, ttri2.p[i],
                                                       ttri2.p[(i + 2) % 3],
                                                       ttri2.p[(i + 1) % 3],
                                                       pcurr, c_to_p, c_to_n))
                                        return;
                        }
                }
        }

        obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
        for (int n = 0; n < solids.n; n++) {
                obj_s *o = solids.o[n];
                if (o->soliddisabled) continue;
                v2_i32 opoints[4];
                points_from_rec(obj_aabb(o), opoints);
                for (int i = 0; i < 4; i++) {
                        if (rope_vertex_convex(z, opoints[i],
                                               opoints[(i + 3) % 4],
                                               opoints[(i + 1) % 4],
                                               pcurr, c_to_p, c_to_n))
                                return;
                }
        }

        ropenode_delete(r, rc);

        os_spmem_push();
        v2_arr *hull = v2_arrcreate(16, os_spmem_alloc);
        v2_arr *pts  = v2_arrcreate(64, os_spmem_alloc);
        v2_arradd(pts, pprev);
        v2_arradd(pts, pnext);
        tri_i32 tri = {pprev, pcurr, pnext};
        rope_points_in_tris(g, tri, tri, pts);
        v2_arrdelq(pts, pcurr); // ignore vertex at p1
        if (v2_arrlen(pts) == 2) {
                // no convex hull to be found, direct connection
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

void rope_update(game_s *g, rope_s *r)
{
        ropenode_s *rprev = r->head;
        ropenode_s *rcurr = r->head->next;
        ropenode_s *rnext = r->head->next->next;

        while (rprev && rcurr && rnext) {
                tighten_ropesegment(g, r, rprev, rcurr, rnext);
                rcurr = rnext;
                rprev = rcurr->prev;
                rnext = rcurr->next;
        }
}

static u32 rope_length_q4(rope_s *r)
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

bool32 rope_intact(game_s *g, rope_s *r)
{
        u32 len_q4     = rope_length_q4(r);
        u32 len_max_q4 = r->len_max << 4;

#if 0
        // check for maximum stretch
        if (len_q4 > (len_max_q4 * 2)) {
                PRINTF("LENGTH\n");
                PRINTF("%i (%i) - %i (%i)\n", len_q4 >> 4, r->len_max, len_q4, len_max_q4);
                return 0;
        }
#endif

        for (ropenode_s *r1 = r->head, *r2 = r->head->next; r2;
             r1 = r2, r2 = r2->next) {
                lineseg_i32 ls = {r1->p, r2->p};
                i32         x1, y1, x2, y2;
                game_tile_bounds_minmax(g, v2_min(r1->p, r2->p), v2_max(r1->p, r2->p),
                                        &x1, &y1, &x2, &y2);
                for (int y = y1; y <= y2; y++) {
                        for (int x = x1; x <= x2; x++) {
                                int t = g->tiles[x + y * g->tiles_x];
                                if (t == 0) continue;
                                tilecollider_s tc = tilecolliders[t - 1];
                                v2_i32         ts = {x * 16, y * 16};

                                tri_i32 tr1 = translate_tri(tc.tris[0], ts);
                                tri_i32 tr2 = translate_tri(tc.tris[1], ts);
                                if (overlap_tri_lineseg_excl(tr1, ls) ||
                                    overlap_tri_lineseg_excl(tr2, ls)) {
                                        PRINTF("TILES\n");
                                        overlap_tri_lineseg_excl(tr1, ls); // ony for debugging
                                        overlap_tri_lineseg_excl(tr2, ls);
                                        return 0;
                                }
                        }
                }

                obj_listc_s solids = objbucket_list(g, OBJ_BUCKET_SOLID);
                for (int n = 0; n < solids.n; n++) {
                        obj_s  *o  = solids.o[n];
                        rec_i32 ro = obj_aabb(o);
                        if (overlap_rec_lineseg_excl(ro, ls)) {
                                PRINTF("SOLID\n");
                                overlap_rec_lineseg_excl(ro, ls); // ony for debugging
                                return 0;
                        }
                }
        }
        return 1;
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

        i32 dt_len = (i32)(len_q4 - len_max_q4);
        if (dt_len <= 0) return vel; // rope is not stretched

        // damping force
        v2_i32 vzero = {0};
        v2_i32 vrad  = project_pnt_line(vel, vzero, dt_q4);
        v2_i32 fdamp = v2_q_mulr(vrad, r->damping_q8, 8);

        // spring force
        i32    fspring_scalar = q_mulr(dt_len, r->spring_q8, 8);
        v2_i32 fspring        = v2_setlen(dt_q4, fspring_scalar);

        v2_i32 frope   = v2_add(fdamp, fspring);
        v2_i32 vel_new = v2_sub(vel, frope);
        return vel_new;
}