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

typedef struct {
    v2_i32 pt[64];
    int    n;
} ropepts_s;

static solid_rec_list_s rope_solid_recs(game_s *g)
{
    return game_solid_recs(g);
}

static int ropepts_find(ropepts_s *pts, v2_i32 p)
{
    for (int i = 0; i < pts->n; i++) {
        if (pts->pt[i].x == p.x && pts->pt[i].y == p.y)
            return i;
    }
    return -1;
}

static void ropepts_remove(ropepts_s *pts, v2_i32 p)
{
    int i = ropepts_find(pts, p);
    if (i >= 0) {
        pts->pt[i] = pts->pt[--pts->n];
    }
}

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
}

void rope_set_len_max_q4(rope_s *r, i32 len_max_q4)
{
    r->len_max_q4 = len_max_q4;
}

ropenode_s *ropenode_insert(rope_s *r, ropenode_s *a, ropenode_s *b, v2_i32 p)
{
    assert((a->next == b && b->prev == a) ||
           (b->next == a && a->prev == b));
    assert(r->pool);
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
        assert(0);
    }
    r->pmin = v2_min(r->pmin, p);
    r->pmax = v2_max(r->pmax, p);
    return rn;
}

void ropenode_delete(rope_s *r, ropenode_s *rn)
{
    ropenode_s *prev = rn->prev;
    ropenode_s *next = rn->next;
    assert(prev && next); // can only delete nodes in the middle
    assert(prev->next == rn && next->prev == rn);
    prev->next = next;
    next->prev = prev;
    rn->next   = r->pool;
    r->pool    = rn;
}

// checks collinearity of a point and returns whether to add the point
// and at which position
static int rope_points_collinearity(ropepts_s *pts, v2_i32 c)
{
    for (int n = 0; n < pts->n - 1; n++) {
        v2_i32 a   = pts->pt[n];
        v2_i32 ac  = v2_sub(c, a);
        u32    dac = v2_lensq(ac);
        for (int i = n + 1; i < pts->n; i++) {
            v2_i32 b  = pts->pt[i];
            v2_i32 ab = v2_sub(b, a);
            if (v2_crs(ac, ab) != 0) continue;
            // at this point a, b, c are collinear which doesn't
            // work with our convex hull algorithm
            u32 dab = v2_lensq(ab);
            u32 dbc = v2_distancesq(c, b);

            if (dac > dab && dac > dbc) return i;  // b is midpoint, replace at i
            if (dbc > dac && dbc > dab) return n;  // a is midpoint, replace at n
            if (dab > dac && dab > dbc) return -1; // c is midpoint, don't add
        }
    }
    return pts->n;
}

static void try_add_point_in_tri(convex_vertex_s p, tri_i32 t1, tri_i32 t2,
                                 ropepts_s *pts)
{
    if (!overlap_tri_pnt_incl(t1, p.p) || !overlap_tri_pnt_incl(t2, p.p))
        return;
    // nasty with lots of calculations...
    // ONLY NEEDED FOR SOLID MOVEMENT though
    lineseg_i32 lu = {p.p, p.u};
    lineseg_i32 lv = {p.p, p.v};
    if (!overlap_tri_lineseg_excl(t1, lu) &&
        !overlap_tri_lineseg_excl(t1, lv) &&
        !overlap_tri_lineseg_excl(t2, lu) &&
        !overlap_tri_lineseg_excl(t2, lv)) return;

    if (ropepts_find(pts, p.p) >= 0) return;
    int k = rope_points_collinearity(pts, p.p);
    // add or overwrite
    if (k == pts->n) {
        pts->pt[pts->n++] = p.p;
    } else if (k >= 0) { // add only if positive
        pts->pt[k] = p.p;
    }
}

static void rope_points_in_tris(game_s *g, tri_i32 t1, tri_i32 t2, ropepts_s *pts)
{
    assert(v2_crs(v2_sub(t1.p[2], t1.p[0]), v2_sub(t1.p[1], t1.p[0])) != 0);
    assert(v2_crs(v2_sub(t2.p[2], t2.p[0]), v2_sub(t2.p[1], t2.p[0])) != 0);

    v2_i32      pmin1  = v2_min(t1.p[0], v2_min(t1.p[1], t1.p[2]));
    v2_i32      pmin2  = v2_min(t2.p[0], v2_min(t2.p[1], t2.p[2]));
    v2_i32      pmax1  = v2_max(t1.p[0], v2_max(t1.p[1], t1.p[2]));
    v2_i32      pmax2  = v2_max(t2.p[0], v2_max(t2.p[1], t2.p[2]));
    bounds_2D_s bounds = game_tilebounds_pts(g,
                                             v2_min(pmin1, pmin2),
                                             v2_max(pmax1, pmax2));

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            int t = g->tiles[x + y * g->tiles_x].collision;
            if (!(0 < t && t < NUM_TILE_BLOCKS)) continue;
            v2_i32 pos = {x << 4, y << 4};
            if (t == TILE_BLOCK) {
                v2_i32  p[4];
                rec_i32 rblock = {pos.x, pos.y, 16, 16};
                points_from_rec(rblock, p);
                convex_vertex_s v0 = {p[0], p[1], p[2]};
                convex_vertex_s v1 = {p[1], p[2], p[3]};
                convex_vertex_s v2 = {p[2], p[3], p[0]};
                convex_vertex_s v3 = {p[3], p[0], p[1]};
                try_add_point_in_tri(v0, t1, t2, pts);
                try_add_point_in_tri(v1, t1, t2, pts);
                try_add_point_in_tri(v2, t1, t2, pts);
                try_add_point_in_tri(v3, t1, t2, pts);
                continue;
            }
            if (t < TILE_SLOPE_HI) {
                tri_i32 tr = translate_tri(tilecolliders[t], pos);
                v2_i32 *p  = tr.p;

                convex_vertex_s v0 = {p[0], p[1], p[2]};
                convex_vertex_s v1 = {p[1], p[2], p[0]};
                convex_vertex_s v2 = {p[2], p[0], p[1]};
                try_add_point_in_tri(v0, t1, t2, pts);
                try_add_point_in_tri(v1, t1, t2, pts);
                try_add_point_in_tri(v2, t1, t2, pts);
                continue;
            }
            NOT_IMPLEMENTED
        }
    }

    solid_rec_list_s solids = rope_solid_recs(g);
    for (int n = 0; n < solids.n; n++) {
        v2_i32 p[4];
        points_from_rec(solids.recs[n], p);

        convex_vertex_s v0 = {p[0], p[1], p[2]};
        convex_vertex_s v1 = {p[1], p[2], p[3]};
        convex_vertex_s v2 = {p[2], p[3], p[0]};
        convex_vertex_s v3 = {p[3], p[0], p[1]};
        try_add_point_in_tri(v0, t1, t2, pts);
        try_add_point_in_tri(v1, t1, t2, pts);
        try_add_point_in_tri(v2, t1, t2, pts);
        try_add_point_in_tri(v3, t1, t2, pts);
    }
}

// pts needs to contain pfrom and pto
static void rope_build_convex_hull(ropepts_s *pts, v2_i32 pfrom, v2_i32 pto,
                                   i32 dir, ropepts_s *newnodes)
{
    int l = 0;
    int p = 0;
    do {
        v2_i32 pc = pts->pt[p]; // current point
        if (!v2_eq(pc, pfrom) && !v2_eq(pc, pto)) {
            newnodes->pt[newnodes->n++] = pc;
        }

        int q = (p + 1) % pts->n;
        assert(0 <= q && q < 64);
        v2_i32 pn = pts->pt[q]; // next point
        for (int i = 0; i < pts->n; i++) {
            if (i == p || i == q) continue;
            v2_i32 pi = pts->pt[i];
            i32    v  = v2_crs(v2_sub(pn, pi), v2_sub(pi, pc));
            if ((v | dir) >= 0 || (v <= 0 && dir <= 0)) {
                q  = i;
                pn = pi;
            }
        }
        p = q;
    } while (p != l);
}

void ropenode_on_moved(game_s *g, rope_s *r, ropenode_s *rn,
                       v2_i32 p1, v2_i32 p2, ropenode_s *rn_anchor,
                       tri_i32 subtri)
{
    assert((rn->next == rn_anchor && rn_anchor->prev == rn) ||
           (rn->prev == rn_anchor && rn_anchor->next == rn));

    if (v2_eq(p1, p2)) return;
    r->pmin = v2_min(r->pmin, p2);
    r->pmax = v2_max(r->pmax, p2);

    v2_i32 p0  = rn_anchor->p; // anchor
    i32    dir = v2_crs(v2_sub(p1, p0), v2_sub(p2, p0));
    if (dir == 0) return;

    spm_push();

    ropepts_s *pts = (ropepts_s *)spm_alloc(sizeof(ropepts_s));
    pts->n         = 2;
    pts->pt[0]     = p0;
    pts->pt[1]     = p2;

    // add points inside the arc
    tri_i32 tri = {p0, p1, p2};
    rope_points_in_tris(g, tri, subtri, pts);
    if (pts->n == 2) { // encountered to obstacles
        spm_pop();
        return;
    }

    // at this point we have some obstacles
    // wrap the rope around them - build a convex hull
    ropepts_s *hull = (ropepts_s *)spm_alloc(sizeof(ropepts_s));
    hull->n         = 0;
    rope_build_convex_hull(pts, p0, p2, dir, hull);

    // at this point p_ropearc contains the "convex hull" when moving
    // from p1 around p0 to p2. Points in array are in the correct order
    ropenode_s *rc = rn;
    for (int i = 0; i < hull->n; i++) {
        rc = ropenode_insert(r, rn_anchor, rc, hull->pt[i]);
    }
    spm_pop();
}

void ropenode_move(game_s *g, rope_s *r, ropenode_s *rn, v2_i32 dt)
{
    r->dirty     = 1;
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
    // assert(rope_intact(g, r));
}

static void rope_move_vertex(game_s *g, rope_s *r, v2_i32 dt, v2_i32 point)
{
    const v2_i32      p_beg  = point;
    const v2_i32      p_end  = v2_add(p_beg, dt);
    const v2_i32      p_pst  = v2_sub(p_beg, dt);
    const lineseg_i32 ls_mov = {p_beg, p_end};
    const lineray_i32 lr_pst = {p_beg, p_pst};

    // first move all nodes which are directly hit by a
    // solid vertex
    // here we SUPPOSE that neither head nor tail are directly
    // modified but instead pushed via an attached actor
    for (ropenode_s *r1 = r->head, *r2 = r->head->next; r2;
         r1 = r2, r2 = r2->next) {
        if (!overlap_lineseg_pnt_incl_excl(ls_mov, r1->p)) {
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
        const lineseg_i32 ls = {r1->p, r2->p};

        // shall not overlap "piston" ray
        if (overlap_lineseg_lineray_excl(ls, lr_pst))
            continue;
        // moving line segment of "piston" should
        // overlap the rope segment
        if (!overlap_lineseg_incl(ls, ls_mov) ||
            v2_eq(r1->p, p_beg) || v2_eq(r1->p, p_end) ||
            v2_eq(r2->p, p_beg) || v2_eq(r2->p, p_end)) {
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
    v2_i32  points_[4];
    rec_i32 aabb = obj_aabb(solid);
    rec_i32 rec  = aabb;
    points_from_rec(aabb, points_);

    // only consider the points moving "forward" of the solid
    v2_i32 points[2];
    if (dt.x > 0) {
        rec.w += dt.x;
        points[0] = points_[1];
        points[1] = points_[2];
    } else if (dt.x < 0) {
        rec.x -= dt.x;
        rec.w += dt.x;
        points[0] = points_[0];
        points[1] = points_[3];
    } else if (dt.y > 0) {
        rec.h += dt.y;
        points[0] = points_[2];
        points[1] = points_[3];
    } else {
        rec.y -= dt.y;
        rec.h += dt.y;
        points[0] = points_[0];
        points[1] = points_[1];
    }

    // early out if the solid doesn't at least overlap
    // the rope's aabb
    rec_i32 ropebounds = {r->pmin.x, r->pmin.y,
                          r->pmax.x - r->pmin.x, r->pmax.y - r->pmin.y};
    if (!overlap_rec(rec, ropebounds)) return;
    r->dirty = 1;

    rope_move_vertex(g, r, dt, points[0]);
    rope_move_vertex(g, r, dt, points[1]);
}

static bool32 rope_pt_convex(i32 z, v2_i32 p, v2_i32 u, v2_i32 v,
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
    assert(rp->next == rc && rn->prev == rc &&
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
    const v2_i32      ctop    = v2_sub(pprev, pcurr); // from curr to prev
    const v2_i32      cton    = v2_sub(pnext, pcurr); // from curr to next
    const i32         z       = v2_crs(ctop, cton);   // benddirection
    const tri_i32     trispan = {pprev, pcurr, pnext};
    const bounds_2D_s bounds  = game_tilebounds_tri(g, trispan);

    for (int y = bounds.y1; y <= bounds.y2; y++) {
        for (int x = bounds.x1; x <= bounds.x2; x++) {
            int t = g->tiles[x + y * g->tiles_x].collision;
            if (!(0 < t && t < NUM_TILE_BLOCKS)) continue;
            v2_i32 pos = {x << 4, y << 4};
            if (t == TILE_BLOCK) {
                v2_i32  p[4];
                rec_i32 r = {pos.x, pos.y, 16, 16};
                points_from_rec(r, p);
                if (rope_pt_convex(z, p[0], p[3], p[1], pcurr, ctop, cton) ||
                    rope_pt_convex(z, p[1], p[0], p[2], pcurr, ctop, cton) ||
                    rope_pt_convex(z, p[2], p[1], p[3], pcurr, ctop, cton) ||
                    rope_pt_convex(z, p[3], p[2], p[0], pcurr, ctop, cton))
                    return;
                continue;
            }
            if (t < TILE_SLOPE_HI) {
                tri_i32 tr = translate_tri(tilecolliders[t], pos);
                v2_i32 *p  = tr.p;
                if (rope_pt_convex(z, p[0], p[1], p[2], pcurr, ctop, cton) ||
                    rope_pt_convex(z, p[1], p[2], p[0], pcurr, ctop, cton) ||
                    rope_pt_convex(z, p[2], p[0], p[1], pcurr, ctop, cton))
                    return;
                continue;
            }
            NOT_IMPLEMENTED
        }
    }

    solid_rec_list_s solids = rope_solid_recs(g);
    for (int n = 0; n < solids.n; n++) {
        v2_i32 p[4];
        points_from_rec(solids.recs[n], p);
        if (rope_pt_convex(z, p[0], p[3], p[1], pcurr, ctop, cton) ||
            rope_pt_convex(z, p[1], p[0], p[2], pcurr, ctop, cton) ||
            rope_pt_convex(z, p[2], p[1], p[3], pcurr, ctop, cton) ||
            rope_pt_convex(z, p[3], p[2], p[0], pcurr, ctop, cton))
            return;
    }

    ropenode_delete(r, rc);

    spm_push();
    ropepts_s *hull   = (ropepts_s *)spm_alloc(sizeof(ropepts_s));
    hull->n           = 0;
    ropepts_s *pts    = (ropepts_s *)spm_alloc(sizeof(ropepts_s));
    pts->n            = 0;
    pts->pt[pts->n++] = pprev;
    pts->pt[pts->n++] = pnext;
    tri_i32 tri       = {pprev, pcurr, pnext};
    rope_points_in_tris(g, tri, tri, pts);
    ropepts_remove(pts, pcurr); // ignore vertex at p1
    if (pts->n == 2) {
        spm_pop(); // no convex hull, direct connection
        return;
    }

    rope_build_convex_hull(pts, pprev, pnext, z, hull); // TODO: verify

    ropenode_s *tmp = rp;
    for (int i = 0; i < hull->n; i++) {
        tmp = ropenode_insert(r, tmp, rn, hull->pt[i]);
    }

    spm_pop();
}

void rope_update(game_s *g, rope_s *r)
{
    assert(r->head);
    if (!r->dirty) return;
    r->dirty          = 0;
    ropenode_s *rprev = r->head;
    ropenode_s *rcurr = r->head->next;
    ropenode_s *rnext = r->head->next->next;

    while (rprev && rcurr && rnext) {
        tighten_ropesegment(g, r, rprev, rcurr, rnext);
        rcurr = rnext;
        rprev = rcurr->prev;
        rnext = rcurr->next;
    }

    r->pmin = r->head->p;
    r->pmax = r->head->p;

    for (ropenode_s *rn = r->head->next; rn; rn = rn->next) {
        r->pmin = v2_min(r->pmin, rn->p);
        r->pmax = v2_max(r->pmax, rn->p);
    }

    // assert(rope_intact(g, r));
}

u32 rope_length_q4(game_s *g, rope_s *r)
{
    rope_update(g, r);
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

bool32 rope_stretched(game_s *g, rope_s *r)
{
    u32 len_q4     = rope_length_q4(g, r);
    u32 len_max_q4 = r->len_max_q4;
    return (len_q4 >= len_max_q4);
}

bool32 rope_intact(game_s *g, rope_s *r)
{
#if 0
        u32 len_q4     = rope_length_q4(g, r);
        u32 len_max_q4 = r->len_max << 4;
        // check for maximum stretch
        if (len_q4 > (len_max_q4 * 2)) {
                PRINTF("LENGTH\n");
                PRINTF("%i (%i) - %i (%i)\n", len_q4 >> 4, r->len_max, len_q4, len_max_q4);
                return 0;
        }
#endif

    rope_update(g, r);

    for (ropenode_s *r1 = r->head, *r2 = r->head->next; r2;
         r1 = r2, r2 = r2->next) {
        lineseg_i32 ls     = {r1->p, r2->p};
        bounds_2D_s bounds = game_tilebounds_pts(g,
                                                 v2_min(r1->p, r2->p),
                                                 v2_max(r1->p, r2->p));

        for (int y = bounds.y1; y <= bounds.y2; y++) {
            for (int x = bounds.x1; x <= bounds.x2; x++) {
                int t = g->tiles[x + y * g->tiles_x].collision;
                if (!(0 < t && t < NUM_TILE_BLOCKS)) continue;
                v2_i32 pos = {x << 4, y << 4};
                if (t == TILE_BLOCK) {
                    rec_i32 rr = {pos.x, pos.y, 16, 16};
                    if (overlap_rec_lineseg_excl(rr, ls)) {
                        return 0;
                    }
                } else if (t < TILE_SLOPE_HI) {
                    tri_i32 tr = translate_tri(tilecolliders[t], pos);
                    if (overlap_tri_lineseg_excl(tr, ls)) {
                        return 0;
                    }
                } else {
                    NOT_IMPLEMENTED
                }
            }
        }

        solid_rec_list_s solids = rope_solid_recs(g);
        for (int n = 0; n < solids.n; n++) {
            rec_i32 ro = solids.recs[n];
            if (overlap_rec_lineseg_excl(ro, ls)) {
                sys_printf("BREAKME\n");
                return 0;
            }
        }
    }
    return 1;
}
