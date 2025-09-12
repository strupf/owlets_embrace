// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

u32 hitbox_UID_gen(g_s *g)
{
    return ++g->hitbox_UID;
}

hitbox_s *hitbox_new(g_s *g)
{
    assert(g->n_hitboxes < HITBOX_NUM);
    hitbox_s *h = &g->hitboxes[g->n_hitboxes++];
    mclr(h, sizeof(hitbox_s));
    h->UID = hitbox_UID_gen(g);
    return h;
}

void hitbox_set_callback(hitbox_s *h, hitbox_on_hit_f f, void *ctx)
{
    h->cb  = f;
    h->ctx = ctx;
}

void hitbox_set_rec(hitbox_s *h, rec_i32 r)
{
    h->pos_x = r.x;
    h->pos_y = r.y;
    h->rec_w = r.w;
    h->rec_h = r.h;
}

bool32 hitbox_from_owl(hitbox_s *h)
{
    return (HITBOXID_OWL_0 <= h->ID && h->ID <= HITBOXID_OWL_1);
}

bool32 hitbox_from_enemy(hitbox_s *h)
{
    return (HITBOXID_ENEMY_0 <= h->ID && h->ID <= HITBOXID_ENEMY_1);
}

bool32 hitbox_hits_obj(hitbox_s *h, obj_s *o)
{
    rec_i32 r = obj_aabb(o);
    if (h->cir_r) {
        return overlap_rec_cir(r, h->pos_x, h->pos_y, h->cir_r);
    } else {
        rec_i32 rh = {h->pos_x, h->pos_y, h->rec_w, h->rec_h};
        return overlap_rec(r, rh);
    }
    return 0;
}

bool32 hitbox_try_apply_to_enemy(g_s *g, hitbox_s *hboxes, i32 n_hb, obj_s *i);

bool32 hitbox_try_apply_to_enemies(g_s *g, hitbox_s *hb, i32 n_hb)
{
    bool32 did_hit = 0;

    for (obj_each(g, i)) {
        did_hit |= hitbox_try_apply_to_enemy(g, hb, n_hb, i);
    }
    return did_hit;
}

bool32 hitbox_try_apply_to_enemy(g_s *g, hitbox_s *hboxes, i32 n_hb, obj_s *i)
{
    bool32 did_hit = 0;

    for (i32 n = 0; n < n_hb; n++) {
        hitbox_s *hb = &hboxes[n];
        if (!hitbox_hits_obj(hb, i)) continue;

        // check if this hitbox already hit
        // otherwise remember this hitboxID by overwriting the smallest
        // registered hitboxID (should be the oldest one)
        b32 already_hit_by = 0;
        i32 k_smallestID   = 0;
        u32 h_smallestID   = U32_MAX;
        for (i32 k = 0; k < OBJ_NUM_HITBOXID; k++) {
            u32 hID = i->hitboxIDs[k];
            if (hID == hb->UID) {
                already_hit_by = 1;
                break;
            }
            if (hID < h_smallestID) {
                h_smallestID = hID;
                k_smallestID = k;
            }
        }

        if (already_hit_by) continue;

        i->hitboxIDs[k_smallestID] = hb->UID;

        switch (i->ID) {
        case 0: break;
        case OBJID_JUMPER: {
            did_hit = 1;
            i->health--;
            jumper_on_hurt(g, i);
            if (!i->health) {
                animobj_create(g, obj_pos_center(i), ANIMOBJ_EXPLOSION_3);
            }
            break;
        }
        case OBJID_CRAWLER: {
            did_hit = 1;
            i->health--;
            crawler_on_hurt(g, i);
            if (!i->health) {
                animobj_create(g, obj_pos_center(i), ANIMOBJ_EXPLOSION_3);
            }
            break;
        }
        case OBJID_CRAB: {
            did_hit = 1;
            i->health--;
            crab_on_hurt(g, i);
            if (!i->health) {
                animobj_create(g, obj_pos_center(i), ANIMOBJ_EXPLOSION_3);
            }
            break;
        }
        case OBJID_FROG: {
            did_hit = 1;
            i->health--;
            frog_on_hurt(g, i);
            if (!i->health) {
                animobj_create(g, obj_pos_center(i), ANIMOBJ_EXPLOSION_3);
            }
            break;
        }
        case OBJID_FLYBLOB: {
            did_hit = 1;
            i->health--;
            flyblob_on_hurt(g, i, hb);
            if (!i->health) {
                animobj_create(g, obj_pos_center(i), ANIMOBJ_EXPLOSION_3);
            }
            break;
        }
        case OBJID_GEMPILE: {
            did_hit = 1;
            gempile_on_hit(g, i);
            break;
        }
        case OBJID_BOMBPLANT: {
            did_hit = 1;
            bombplant_on_hit(g, i);
            break;
        }
        }

        if (did_hit && hb->cb) {
            hb->cb(g, hb, hb->ctx);
        }
    }
    return did_hit;
}