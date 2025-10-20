// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "wire.h"
#include "game.h"

// corner struct: p = corner vertex, u and v are vectors to the connected neighbour corners
typedef struct {
    v2_i32 p;
    v2_i32 u;
    v2_i32 v;
} wire_convex_pt_s;

// simple array struct
typedef struct {
    ALIGNAS(8)
    i32     n;
    v2_i32 *pt;
} wire_pts_s;

static i32         wire_pts_find(wire_pts_s *pts, v2_i32 p);
static void        wire_pts_remove(wire_pts_s *pts, v2_i32 p);
static wirenode_s *wirenode_insert(wire_s *r, wirenode_s *a, wirenode_s *b, v2_i32 p);
static void        wirenode_delete(wire_s *r, wirenode_s *rn);

void wire_init(wire_s *r)
{
    mclr(r, sizeof(wire_s));
    for (i32 n = 2; n < NUM_WIRE_NODES - 1; n++) {
        r->nodesraw[n].next = &r->nodesraw[n + 1];
    }
    r->pool = &r->nodesraw[2];

    wirenode_s *rh = &r->nodesraw[0];
    wirenode_s *rt = &r->nodesraw[1];
    rh->next       = rt;
    rt->prev       = rh;
    r->head        = rh;
    r->tail        = rt;
}

void wire_init_as_copy(wire_s *r, wire_s *to_copy)
{
    wire_init(r);

    r->head->p = to_copy->head->p;
    r->tail->p = to_copy->tail->p;

    for (wirenode_s *a = to_copy->head->next; a != to_copy->tail; a = a->next) {
        wirenode_insert(r, r->tail->prev, r->tail, a->p);
    }

    r->pmax  = to_copy->pmax;
    r->pmin  = to_copy->pmin;
    r->dirty = to_copy->dirty;
}

static wirenode_s *wirenode_insert(wire_s *r, wirenode_s *a, wirenode_s *b, v2_i32 p)
{
    assert((a->next == b && b->prev == a) ||
           (b->next == a && a->prev == b));
    wirenode_s *rn = r->pool;
    if (!rn) {
        BAD_PATH();
        return 0;
    }
    r->pool = rn->next;
    rn->p   = p;
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
        BAD_PATH();
    }
    r->pmin = v2_min(r->pmin, p);
    r->pmax = v2_max(r->pmax, p);
    return rn;
}

static void wirenode_delete(wire_s *r, wirenode_s *rn)
{
    wirenode_s *prev = rn->prev;
    wirenode_s *next = rn->next;
    assert(prev && next); // can only delete nodes in the middle
    if (!prev || !next) {
        BAD_PATH();
        return;
    }
    assert(prev->next == rn && next->prev == rn);
    prev->next = next;
    next->prev = prev;
    rn->next   = r->pool;
    r->pool    = rn;
}

// checks collinearity of a point and returns whether to add the point
// and at which position
static i32 wire_points_collinearity(wire_pts_s *pts, v2_i32 c)
{
    for (i32 n = 0; n < pts->n - 1; n++) {
        v2_i32 a   = pts->pt[n];
        v2_i32 ac  = v2_i32_sub(c, a);
        u32    dac = v2_i32_lensq(ac);

        for (i32 i = n + 1; i < pts->n; i++) {
            v2_i32 b  = pts->pt[i];
            v2_i32 ab = v2_i32_sub(b, a);
            if (v2_i32_crs(ac, ab) != 0) continue;

            // at this point a, b, c are collinear which doesn't
            // work with our convex hull algorithm
            u32 dab = v2_i32_lensq(ab);
            u32 dbc = v2_i32_distancesq(c, b);

            if (dab < dac && dbc < dac) return i;  // b is midpoint, replace at i
            if (dac < dbc && dab < dbc) return n;  // a is midpoint, replace at n
            if (dac < dab && dbc < dab) return -1; // c is midpoint, don't add
        }
    }
    return pts->n;
}

static void wire_try_add_point_in_tri(wire_convex_pt_s p, tri_i32 t1, tri_i32 t2, wire_pts_s *pts)
{
    if (!overlap_tri_pnt_incl(t1, p.p) || !overlap_tri_pnt_incl(t2, p.p))
        return;
    lineseg_i32 lu = {p.p, p.u};
    lineseg_i32 lv = {p.p, p.v};

    if (!overlap_tri_lineseg_excl(t1, lu) && !overlap_tri_lineseg_excl(t1, lv) &&
        !overlap_tri_lineseg_excl(t2, lu) && !overlap_tri_lineseg_excl(t2, lv))
        return;

    if (wire_pts_find(pts, p.p) >= 0) return;

    i32 k = wire_points_collinearity(pts, p.p);
    // add or overwrite
    if (k == pts->n) {
        pts->pt[pts->n++] = p.p;
    } else if (0 <= k) { // add only if positive
        pts->pt[k] = p.p;
    }
}

static void wire_points_in_tris(g_s *g, tri_i32 t1, tri_i32 t2, wire_pts_s *pts)
{
    assert(v2_i32_crs(v2_i32_sub(t1.p[2], t1.p[0]), v2_i32_sub(t1.p[1], t1.p[0])) != 0);
    assert(v2_i32_crs(v2_i32_sub(t2.p[2], t2.p[0]), v2_i32_sub(t2.p[1], t2.p[0])) != 0);

    v2_i32            pmin1  = v2_min(t1.p[0], v2_min(t1.p[1], t1.p[2]));
    v2_i32            pmin2  = v2_min(t2.p[0], v2_min(t2.p[1], t2.p[2]));
    v2_i32            pmax1  = v2_max(t1.p[0], v2_max(t1.p[1], t1.p[2]));
    v2_i32            pmax2  = v2_max(t2.p[0], v2_max(t2.p[1], t2.p[2]));
    tile_map_bounds_s bounds = tile_map_bounds_pts(g,
                                                   v2_min(pmin1, pmin2),
                                                   v2_max(pmax1, pmax2));

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32 t = g->tiles[x + y * g->tiles_x].shape;
            if (!(0 < t && t < NUM_TILE_SHAPES)) continue;
            v2_i32 pos = {x << 4, y << 4};
            if (TILE_IS_BLOCK(t)) {
                v2_i32  p[4];
                rec_i32 rblock = {pos.x, pos.y, 16, 16};
                points_from_rec(rblock, p);
                wire_convex_pt_s v0 = {p[0], p[1], p[2]};
                wire_convex_pt_s v1 = {p[1], p[2], p[3]};
                wire_convex_pt_s v2 = {p[2], p[3], p[0]};
                wire_convex_pt_s v3 = {p[3], p[0], p[1]};
                wire_try_add_point_in_tri(v0, t1, t2, pts);
                wire_try_add_point_in_tri(v1, t1, t2, pts);
                wire_try_add_point_in_tri(v2, t1, t2, pts);
                wire_try_add_point_in_tri(v3, t1, t2, pts);
            }
            if (TILE_IS_SLOPE_45(t)) {
                tri_i32 tr = translate_tri(((tri_i32 *)g_tile_tris)[t], pos);
                v2_i32 *p  = tr.p;

                wire_convex_pt_s v0 = {p[0], p[1], p[2]};
                wire_convex_pt_s v1 = {p[1], p[2], p[0]};
                wire_convex_pt_s v2 = {p[2], p[0], p[1]};
                wire_try_add_point_in_tri(v0, t1, t2, pts);
                wire_try_add_point_in_tri(v1, t1, t2, pts);
                wire_try_add_point_in_tri(v2, t1, t2, pts);
            }
        }
    }

    for (obj_each(g, o)) {
        if (!(o->flags & OBJ_FLAG_SOLID)) continue;
        v2_i32 p[4];
        points_from_rec(obj_aabb(o), p);

        wire_convex_pt_s v0 = {p[0], p[1], p[2]};
        wire_convex_pt_s v1 = {p[1], p[2], p[3]};
        wire_convex_pt_s v2 = {p[2], p[3], p[0]};
        wire_convex_pt_s v3 = {p[3], p[0], p[1]};
        wire_try_add_point_in_tri(v0, t1, t2, pts);
        wire_try_add_point_in_tri(v1, t1, t2, pts);
        wire_try_add_point_in_tri(v2, t1, t2, pts);
        wire_try_add_point_in_tri(v3, t1, t2, pts);
    }
}

// pts needs to contain pfrom and pto
static void wire_build_convex_hull(wire_pts_s *pts, v2_i32 pfrom, v2_i32 pto, i32 dir, wire_pts_s *newnodes)
{
    i32 l = 0;
    i32 p = 0;
    do {
        v2_i32 pc = pts->pt[p]; // current point
        if (!v2_i32_eq(pc, pfrom) && !v2_i32_eq(pc, pto)) {
            newnodes->pt[newnodes->n++] = pc;
        }

        i32 q = (p + 1) % pts->n;
        assert(0 <= q && q < 64);
        v2_i32 pn = pts->pt[q]; // next point
        for (i32 i = 0; i < pts->n; i++) {
            if (i == p || i == q) continue;
            v2_i32 pi = pts->pt[i];
            i32    v  = v2_i32_crs(v2_i32_sub(pn, pi), v2_i32_sub(pi, pc));
            if (0 <= (v | dir) || (v <= 0 && dir <= 0)) {
                q  = i;
                pn = pi;
            }
        }
        p = q;
    } while (p != l);
}

static void wirenode_on_moved(g_s *g, wire_s *r, wirenode_s *rn, v2_i32 p1, v2_i32 p2, wirenode_s *rn_anchor, tri_i32 subtri)
{
    assert((rn->next == rn_anchor && rn_anchor->prev == rn) ||
           (rn->prev == rn_anchor && rn_anchor->next == rn));

    if (v2_i32_eq(p1, p2)) return;
    r->pmin = v2_min(r->pmin, p2);
    r->pmax = v2_max(r->pmax, p2);

    v2_i32 p0  = rn_anchor->p; // anchor
    i32    dir = v2_i32_crs(v2_i32_sub(p1, p0), v2_i32_sub(p2, p0));
    if (dir == 0) return;

    ALIGNAS(8) v2_i32 arr_pts[64];
    arr_pts[0]     = p0;
    arr_pts[1]     = p2;
    wire_pts_s pts = {2, arr_pts};

    // add points inside the arc
    tri_i32 tri = {{p0, p1, p2}};
    wire_points_in_tris(g, tri, subtri, &pts);
    if (pts.n != 2) {
        // at this point we have some obstacles
        // wrap the rope around them - build a convex hull
        ALIGNAS(8) v2_i32 arr_hull[64];
        wire_pts_s        hull = {0, arr_hull};
        wire_build_convex_hull(&pts, p0, p2, dir, &hull);

        // point p_ropearc contains the "convex hull" when moving
        // from p1 around p0 to p2. Points in array are in the correct order
        wirenode_s *rc = rn;
        for (i32 i = 0; i < hull.n; i++) {
            rc = wirenode_insert(r, rn_anchor, rc, hull.pt[i]);
        }
    }
}

void wirenode_move(g_s *g, wire_s *r, wirenode_s *rn, i32 dx, i32 dy)
{
    r->dirty     = 1;
    v2_i32 p_old = rn->p;
    v2_i32 p_new = {p_old.x + dx, p_old.y + dy};
    rn->p        = p_new;

    if (rn->next) {
        tri_i32 tri = {{p_old, p_new, rn->next->p}};
        wirenode_on_moved(g, r, rn, p_old, p_new, rn->next, tri);
    }
    if (rn->prev) {
        tri_i32 tri = {{p_old, p_new, rn->prev->p}};
        wirenode_on_moved(g, r, rn, p_old, p_new, rn->prev, tri);
    }
}

static void wire_move_vertex(g_s *g, wire_s *r, v2_i32 dt, v2_i32 point)
{
    v2_i32      p_beg  = point;
    v2_i32      p_end  = v2_i32_add(p_beg, dt);
    v2_i32      p_pst  = v2_i32_sub(p_beg, dt);
    lineseg_i32 ls_mov = {p_beg, p_end};
    lineray_i32 lr_pst = {p_beg, p_pst};

    // first move all nodes which are directly hit by a
    // solid vertex
    // here we SUPPOSE that neither head nor tail are directly
    // modified but instead pushed via an attached actor
    for (wirenode_s *r1 = r->head, *r2 = r->head->next; r2;
         r1 = r2, r2 = r2->next) {
        if (!overlap_lineseg_pnt_incl_excl(ls_mov, r1->p)) {
            continue;
        }

        v2_i32 rnold = r1->p;
        r1->p        = p_end;
        if (r1->prev) {
            tri_i32 tri = {{r1->prev->p, rnold, p_end}};
            wirenode_on_moved(g, r, r1, rnold, p_end, r1->prev, tri);
        }
        if (r1->next) {
            tri_i32 tri = {{r1->next->p, rnold, p_end}};
            wirenode_on_moved(g, r, r1, rnold, p_end, r1->next, tri);
        }
    }

    // now check penetrating segments
    for (wirenode_s *r1 = r->head, *r2 = r->head->next; r2;
         r1 = r2, r2 = r2->next) {
        lineseg_i32 ls = {r1->p, r2->p};

        // shall not overlap "piston" ray
        if (overlap_lineseg_lineray_excl(ls, lr_pst))
            continue;
        // moving line segment of "piston" should
        // overlap the rope segment
        if (!overlap_lineseg_incl(ls, ls_mov) ||
            v2_i32_eq(r1->p, p_beg) || v2_i32_eq(r1->p, p_end) ||
            v2_i32_eq(r2->p, p_beg) || v2_i32_eq(r2->p, p_end)) {
            continue;
        }

        if (overlap_lineseg_pnt_excl(ls, p_end)) {
            // point lies exactly on the rope segment
            // between r1 and r2 and isn't necessary
            continue;
        }

        wirenode_s *ri  = wirenode_insert(r, r1, r2, p_end);
        // only consider points which are on the
        // "side of penetration"
        tri_i32     tri = {{r1->p, r2->p, p_end}};
        wirenode_on_moved(g, r, ri, p_beg, p_end, r1, tri);
        wirenode_on_moved(g, r, ri, p_beg, p_end, r2, tri);
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
void wire_moved_by_aabb(g_s *g, wire_s *r, rec_i32 aabb, i32 dx, i32 dy)
{
    ALIGNAS(32) v2_i32 points_[4];
    rec_i32            rec = aabb;
    points_from_rec(aabb, points_);

    // only consider the points moving "forward" of the solid
    ALIGNAS(16) v2_i32 points[2];
    if (0) {
    } else if (0 < dx) {
        rec.w += dx;
        points[0] = points_[1];
        points[1] = points_[2];
    } else if (dx < 0) {
        rec.x -= dx;
        rec.w += dx;
        points[0] = points_[0];
        points[1] = points_[3];
    } else if (0 < dy) {
        rec.h += dy;
        points[0] = points_[2];
        points[1] = points_[3];
    } else {
        rec.y -= dy;
        rec.h += dy;
        points[0] = points_[0];
        points[1] = points_[1];
    }

    // early out if the solid doesn't at least overlap
    // the rope's aabb
    rec_i32 ropebounds = {r->pmin.x, r->pmin.y, r->pmax.x - r->pmin.x, r->pmax.y - r->pmin.y};
    if (!overlap_rec(rec, ropebounds)) return;
    r->dirty = 1;

    v2_i32 dt = {dx, dy};
    wire_move_vertex(g, r, dt, points[0]);
    wire_move_vertex(g, r, dt, points[1]);
}

static bool32 wire_pt_convex(i32 z, v2_i32 p, v2_i32 u, v2_i32 v, v2_i32 curr, v2_i32 c_to_p, v2_i32 c_to_n)
{
    if (!v2_i32_eq(curr, p)) return 0;

    v2_i32 c_to_u = v2_i32_sub(u, curr);
    v2_i32 c_to_v = v2_i32_sub(v, curr);
    i32    s1     = v2_i32_crs(c_to_p, c_to_u);
    i32    s2     = v2_i32_crs(c_to_n, c_to_u);
    i32    t1     = v2_i32_crs(c_to_p, c_to_v);
    i32    t2     = v2_i32_crs(c_to_n, c_to_v);
    return ((z >= 0 && s1 >= 0 && s2 <= 0 && t1 >= 0 && t2 <= 0) ||
            (z <= 0 && s1 <= 0 && s2 >= 0 && t1 <= 0 && t2 >= 0));
}

// tries to optimize the path between rp and rn = short path, with rc being the node in question to be removed
// --> check if rc can be deleted, and span/wrap the remaining wire around other corners
static void wire_optimize_wiresegment(g_s *g, wire_s *r, wirenode_s *rp, wirenode_s *rc, wirenode_s *rn)
{
    assert(rp->next == rc && rn->prev == rc &&
           rc->next == rn && rc->prev == rp);
    v2_i32 pprev = rp->p;
    v2_i32 pcurr = rc->p;
    v2_i32 pnext = rn->p;

    // check if the three points are collinear
    if (v2_i32_crs(v2_i32_sub(pprev, pcurr), v2_i32_sub(pnext, pcurr)) == 0) {
        wirenode_delete(r, rc);
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
    v2_i32            ctop    = v2_i32_sub(pprev, pcurr); // from curr to prev
    v2_i32            cton    = v2_i32_sub(pnext, pcurr); // from curr to next
    i32               z       = v2_i32_crs(ctop, cton);   // benddirection
    tri_i32           trispan = {{pprev, pcurr, pnext}};
    tile_map_bounds_s bounds  = tile_map_bounds_tri(g, trispan);

    for (i32 y = bounds.y1; y <= bounds.y2; y++) {
        for (i32 x = bounds.x1; x <= bounds.x2; x++) {
            i32 t = g->tiles[x + y * g->tiles_x].shape;
            if (!TILE_IS_SHAPE(t)) continue;

            v2_i32 pos = {x << 4, y << 4};
            if (TILE_IS_BLOCK(t)) {
                ALIGNAS(32) v2_i32 p[4];
                rec_i32            re = {pos.x, pos.y, 16, 16};
                points_from_rec(re, p);

                if (wire_pt_convex(z, p[0], p[3], p[1], pcurr, ctop, cton) ||
                    wire_pt_convex(z, p[1], p[0], p[2], pcurr, ctop, cton) ||
                    wire_pt_convex(z, p[2], p[1], p[3], pcurr, ctop, cton) ||
                    wire_pt_convex(z, p[3], p[2], p[0], pcurr, ctop, cton))
                    return;
            }
            if (TILE_IS_SLOPE_45(t)) {
                ALIGNAS(32) tri_i32 tr = translate_tri(((tri_i32 *)g_tile_tris)[t], pos);
                v2_i32             *p  = tr.p;

                if (wire_pt_convex(z, p[0], p[1], p[2], pcurr, ctop, cton) ||
                    wire_pt_convex(z, p[1], p[2], p[0], pcurr, ctop, cton) ||
                    wire_pt_convex(z, p[2], p[0], p[1], pcurr, ctop, cton))
                    return;
            }
        }
    }

    for (obj_each(g, o)) {
        if (!(o->flags & OBJ_FLAG_SOLID)) continue;

        ALIGNAS(32) v2_i32 p[4];
        points_from_rec(obj_aabb(o), p);

        if (wire_pt_convex(z, p[0], p[3], p[1], pcurr, ctop, cton) ||
            wire_pt_convex(z, p[1], p[0], p[2], pcurr, ctop, cton) ||
            wire_pt_convex(z, p[2], p[1], p[3], pcurr, ctop, cton) ||
            wire_pt_convex(z, p[3], p[2], p[0], pcurr, ctop, cton))
            return;
    }

    wirenode_delete(r, rc);

    ALIGNAS(8) v2_i32 arr_pts[64];
    ALIGNAS(8) v2_i32 arr_hull[64];
    arr_pts[0]      = pprev;
    arr_pts[1]      = pnext;
    wire_pts_s hull = {0, arr_hull};
    wire_pts_s pts  = {2, arr_pts};
    tri_i32    tri  = {{pprev, pcurr, pnext}};
    wire_points_in_tris(g, tri, tri, &pts);
    wire_pts_remove(&pts, pcurr); // ignore vertex at p1

    if (pts.n != 2) {                                         // added another point
        wire_build_convex_hull(&pts, pprev, pnext, z, &hull); // TODO: verify

        wirenode_s *tmp = rp;
        for (i32 i = 0; i < hull.n; i++) {
            tmp = wirenode_insert(r, tmp, rn, hull.pt[i]);
        }
    }
}

void wire_optimize(g_s *g, wire_s *r)
{
    assert(r->head);
    if (!r->dirty) return;

    r->dirty          = 0;
    wirenode_s *rprev = r->head;
    wirenode_s *rcurr = r->head->next;
    wirenode_s *rnext = r->head->next->next;

    while (rprev && rcurr && rnext) {
        wire_optimize_wiresegment(g, r, rprev, rcurr, rnext);
        rcurr = rnext;
        rprev = rcurr->prev;
        rnext = rcurr->next;
    }

    r->pmin = r->head->p;
    r->pmax = r->head->p;

    for (wirenode_s *rn = r->head->next; rn; rn = rn->next) {
        r->pmin = v2_min(r->pmin, rn->p);
        r->pmax = v2_max(r->pmax, rn->p);
    }
}

u32 wire_len_qx(g_s *g, wire_s *r, i32 q)
{
    wire_optimize(g, r);
    u32 l = 0;

    for (wirenode_s *a = r->head, *b = a->next; b; a = b, b = b->next) {
        l += v2_i32_len_appr(v2_i32_shl(v2_i32_sub(a->p, b->p), q));
    }
    return l;
}

bool32 wire_is_intact(g_s *g, wire_s *r)
{
    for (wirenode_s *a = r->head, *b = a->next; b; a = b, b = b->next) {
        lineseg_i32       ls     = {a->p, b->p};
        tile_map_bounds_s bounds = tile_map_bounds_pts(g, v2_min(a->p, b->p), v2_max(a->p, b->p));

        for (i32 y = bounds.y1; y <= bounds.y2; y++) {
            for (i32 x = bounds.x1; x <= bounds.x2; x++) {
                i32    t   = g->tiles[x + y * g->tiles_x].shape;
                v2_i32 pos = {x << 4, y << 4};

                switch (t) {
                case TILE_BLOCK: {
                    rec_i32 rr = {pos.x, pos.y, 16, 16};
                    if (overlap_rec_lineseg_excl(rr, ls)) {
                        return 0;
                    }
                    break;
                }
                case TILE_SLOPE_45_0:
                case TILE_SLOPE_45_1:
                case TILE_SLOPE_45_2:
                case TILE_SLOPE_45_3: {
                    tri_i32 tr = translate_tri(((tri_i32 *)g_tile_tris)[t], pos);
                    if (overlap_tri_lineseg_excl(tr, ls)) {
                        return 0;
                    }
                    break;
                }
                }
            }
        }

        for (obj_each(g, o)) {
            if ((o->flags & OBJ_FLAG_SOLID) &&
                overlap_rec_lineseg_excl(obj_aabb(o), ls)) {
                return 0;
            }
        }
    }
    return 1;
}

wirenode_s *wirenode_neighbour_of_end_node(wire_s *r, wirenode_s *rn)
{
    assert(rn == r->head || rn == r->tail);
    return (rn->next ? rn->next : rn->prev);
}

v2_i32 wirenode_vec(wire_s *r, wirenode_s *rn)
{
    wirenode_s *rn2 = wirenode_neighbour_of_end_node(r, rn);
    return v2_i32_sub(rn2->p, rn->p);
}

static i32 wire_pts_find(wire_pts_s *pts, v2_i32 p)
{
    for (i32 i = 0; i < pts->n; i++) {
        if (v2_i32_eq(pts->pt[i], p))
            return i;
    }
    return -1;
}

static void wire_pts_remove(wire_pts_s *pts, v2_i32 p)
{
    i32 i = wire_pts_find(pts, p);
    if (0 <= i) {
        pts->pt[i] = pts->pt[--pts->n];
    }
}