// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

u32 hitbox_UID_gen(g_s *g)
{
    return ++g->hitboxUID;
}

hitbox_s *hitbox_gen(g_s *g, u32 UID, i32 ID, hitbox_cb_f cb, void *cb_arg)
{
    if (HITBOX_NUM <= g->n_hitboxes) {
        BAD_PATH();
        return 0;
    }
    hitbox_s *hb = &g->hitboxes[g->n_hitboxes++];
    hb->UID      = UID ? UID : hitbox_UID_gen(g);
    hb->ID       = ID;
    hb->cb       = cb;
    hb->cb_arg   = cb_arg;
    return hb;
}

// sets a user to not be able to hit itself even if in the same group
void hitbox_set_user(hitbox_s *hb, obj_s *o)
{
    hb->user = handle_from_obj(o);
}

void hitbox_set_flags_group(hitbox_s *hb, i32 flags)
{
    hb->flags_group = flags;
}

void hitbox_type_parent(hitbox_s *hb)
{
    NOT_IMPLEMENTED("hitbox parents");
    hb->type  = HITBOX_TYPE_PARENT;
    hb->u.n_c = 0;
}

void hitbox_parent_add_child(hitbox_s *hb_parent, hitbox_s *hb_child)
{
    NOT_IMPLEMENTED("hitbox parents");
    assert(hb_parent < hb_child);

    hb_parent->u.n_c++;
    hb_child->parent_offs = (u8)(hb_child - hb_parent);
    hb_child->UID         = hb_parent->UID;
#if PLTF_DEBUG
    for (i32 n = 0; n < hb_parent->u.n_c; n++) {
        hitbox_s *hb_child_debug = hb_parent + 1 + n;
        assert(hb_parent = hb_child_debug - hb_child_debug->parent_offs);
    }
#endif
}

void hitbox_type_recr(hitbox_s *hb, rec_i32 r)
{
    hitbox_type_rec(hb, r.x, r.y, r.w, r.h);
}

void hitbox_type_recxy(hitbox_s *hb, i32 x1, i32 y1, i32 x2, i32 y2)
{
    hb->type             = HITBOX_TYPE_REC;
    hitbox_type_rec_s *r = &hb->u.rec;
    r->x1                = min_i32(x1, x2);
    r->y1                = min_i32(y1, y2);
    r->x2                = max_i32(x1, x2);
    r->y2                = max_i32(y1, y2);
}

void hitbox_type_rec(hitbox_s *hb, i32 x, i32 y, i32 w, i32 h)
{
    hitbox_type_recxy(hb, x, y, x + w, y + h);
}

void hitbox_type_cir(hitbox_s *hb, i32 x, i32 y, i32 r)
{
    hb->type             = HITBOX_TYPE_CIR;
    hitbox_type_cir_s *c = &hb->u.cir;
    c->x                 = x;
    c->y                 = y;
    c->r                 = r;
}

void hitboxes_flush(g_s *g)
{
    for (obj_each(g, o)) {
        if (!o->hitbox_flags_group) continue;

        rec_i32      o_aabb  = obj_aabb(o);
        hitbox_res_s res     = {0};
        bool32       was_hit = 0;

        for (i32 n = 0; n < g->n_hitboxes; n++) {
            hitbox_s *hb = &g->hitboxes[n];
            if (!(o->hitbox_flags_group & hb->flags_group)) continue;
            if (obj_from_handle(hb->user) == o) continue; // dont hit yourself i.e. enemy should also damage other enemies but of course not itself

            switch (hb->type) {
            default:
            case HITBOX_TYPE_NULL: goto HITBOX_NEXT; // does not hit
            case HITBOX_TYPE_REC: {
                hitbox_type_rec_s *hr = &hb->u.rec;
                rec_i32            r  = {hr->x1, hr->y1, hr->x2 - hr->x1, hr->y2 - hr->y1};

                if (!overlap_rec(o_aabb, r)) goto HITBOX_NEXT; // does not hit
                break;
            }
            case HITBOX_TYPE_CIR: {
                hitbox_type_cir_s *hc = &hb->u.cir;
                if (1) goto HITBOX_NEXT; // does not hit
                break;
            }
            }

            // already hit by this UID earlier?
            for (i32 i = 0; i < OBJ_NUM_HITBOXID; i++) {
                if (hb->UID == o->hitboxUID_registered[i]) {
                    goto HITBOX_NEXT; // does not hit
                }
            }

            was_hit = 1;
            res.damage += hb->damage;
            res.dx_q4 += hb->dx_q4;
            res.dy_q4 += hb->dy_q4;

            o->hitboxUID_registered[o->n_hitboxUID_registered++] = hb->UID; // remember this UID
            o->n_hitboxUID_registered &= (OBJ_NUM_HITBOXID - 1);
            if (o->hitbox_flags_group & HITBOX_FLAG_GROUP_TRIGGERS_CALLBACK) {
                hb->flags |= HITBOX_FLAG_HIT;
            }
        HITBOX_NEXT:;
        }

        if (was_hit && o->on_hitbox) {
            o->on_hitbox(g, o, res);
        }
    }

    // callbacks and clearing of hitbox queue
    for (i32 n = 0; n < g->n_hitboxes; n++) {
        hitbox_s *hb = &g->hitboxes[n];

        // execute callback if:
        // - has callback (duh)
        // - did hit
        // - either no specified user or only if user still valid
        bool32 cb_exe = hb->cb != 0 &&
                        (hb->flags & HITBOX_FLAG_HIT) &&
                        (!hb->user.o || obj_handle_valid(hb->user));

        if (cb_exe) {
            hb->cb(g, hb, hb->cb_arg);
        }
        mclr(hb, sizeof(hitbox_s));
    }
    g->n_hitboxes = 0;
}