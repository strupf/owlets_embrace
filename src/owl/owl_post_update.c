// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "app.h"
#include "game.h"
#include "owl/owl.h"

void owl_post_update_alive(g_s *g, obj_s *o, inp_s inp);

void owl_on_update_post(g_s *g, obj_s *o, inp_s inp)
{
    owl_s *h              = (owl_s *)o->heap;
    i32    health_prev    = o->health;
    h->interactable       = handle_from_obj(0);
    rec_i32 owlr          = obj_aabb(o);
    v2_i32  owlc          = {o->pos.x + (OWL_W >> 1), o->pos.y + o->h - 12};
    i32     heal          = 0;
    i32     dmg           = 0;
    bool32  has_knockback = 0;
    v2_i32  knockback     = {0};

    // damage from objects
    for (obj_each(g, i)) {
        if (i == o) continue;
        rec_i32 ri = obj_aabb(i);

        if ((i->flags & OBJ_FLAG_HURT_ON_TOUCH) && overlap_rec_touch(owlr, ri)) {
            dmg           = max_i32(dmg, 1);
            v2_i32 ic     = obj_pos_center(i);
            v2_i32 dtp    = v2_i32_sub(owlc, ic);
            dtp           = v2_i32_setlen(dtp, Q_VOBJ(3.0));
            has_knockback = 1;
            knockback.x   = dtp.x;
            knockback.y   = -Q_VOBJ(3.0);
        }
    }

    for (i32 n = 0; n < g->n_hitboxes; n++) {
        hitbox_s *hb = &g->hitboxes[n];
        if (!hitbox_hits_obj(hb, o)) continue;
        dmg           = max_i32(dmg, 1);
        has_knockback = 1;
        knockback.x   = hb->dx * Q_VOBJ(3.0);
        knockback.y   = -Q_VOBJ(3.0);
    }

    // damage from tiles
    tile_map_bounds_s bd = tile_map_bounds_rec(g, owlr);
    for (i32 y = bd.y1; y <= bd.y2; y++) {
        for (i32 x = bd.x1; x <= bd.x2; x++) {
            tile_s t = g->tiles[x + y * g->tiles_x];
            if (t.type == TILE_TYPE_THORNS) {
                dmg = max_i32(dmg, 1);
            }
        }
    }

    if (!h->hurt_ticks && !h->invincible) {
        if (has_knockback) {
            o->v_q12.x = knockback.x;
            o->v_q12.y = knockback.y;
            o->bumpflags &= ~OBJ_BUMP_Y;
        }
        i32 health_dt = heal - dmg;
        o->health     = clamp_i32((i32)o->health + health_dt, 0, o->health_max);

        if (dmg) {
            g->freeze_tick = 4;
            snd_play(SNDID_HURT, 1.f, 1.f);

            if (o->health) {
                h->hurt_ticks = OWL_HURT_TICKS;
            }
        }
    }

    owl_post_update_alive(g, o, inp);
}

void owl_post_update_alive(g_s *g, obj_s *o, inp_s inp)
{
    owl_s  *h    = (owl_s *)o->heap;
    rec_i32 owlr = obj_aabb(o);
    v2_i32  owlc = {o->pos.x + (OWL_W >> 1), o->pos.y + o->h - 12};

    bool32 stomped_on_any = 0;
    bool32 jumped_on_any  = h->n_jumpstomped;

    for (i32 n = 0; n < h->n_jumpstomped; n++) {
        obj_jumpstomped_s js = h->jumpstomped[n];
        obj_s            *i  = obj_from_handle(js.h);

        stomped_on_any |= js.stomped;
        if (!i) continue;

        switch (i->ID) {
        case OBJID_MUSHROOM: {
            o->v_q12.y = -Q_VOBJ(8.0);
            o->bumpflags &= ~OBJ_BUMP_Y_POS;
            mushroom_on_jump_on(g, i);
            break;
        }
        case OBJID_STOMPABLE_BLOCK: {
            if (js.stomped) {
                stompable_block_break(g, i);
                owl_stomp_land(g, o);
                o->v_q12.y = -Q_VOBJ(4.0);
                o->bumpflags &= ~OBJ_BUMP_Y_POS;
            }
            break;
        }
        }
    }
    h->n_jumpstomped = 0;

    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_s *fa  = &g->fluid_areas[n];
        rec_i32       fr1 = {fa->x, fa->y - 4, fa->w, 16};
        rec_i32       fr2 = {fa->x, fa->y - 1, fa->w, 2};
        if (overlap_rec(fr1, owlr)) {
            i32 imp = clamp_sym_i32(abs_i32(o->v_q12.x) / 20, Q_VOBJ(0.4));
            fluid_area_impact(fa, owlc.x - fa->x, 12, imp >> 4, FLUID_AREA_IMPACT_COS);
        }
    }

    if (o->health) {
        minimap_try_visit_screen(g);
        if (cs_maptransition_try_slide_enter(g))
            return; // cutscene starts

        u32 d_interactable = POW2(OWL_INTERACTABLE_DST);
        for (obj_each(g, i)) {
            if (i == o) continue;
            rec_i32 ri = obj_aabb(i);

            if (i->flags & OBJ_FLAG_INTERACTABLE) {
                v2_i32 ic = {i->pos.x + (i->w >> 1) + i->offs_interact.x,
                             i->pos.y + (i->h >> 1) + i->offs_interact.y};
                u32    d  = v2_i32_distancesq(owlc, ic);
                if (d < d_interactable) {
                    d_interactable  = d;
                    h->interactable = handle_from_obj(i);
                }
            }

            switch (i->ID) {
            case OBJID_COIN: {
                if (v2_i32_distancesq(owlc, obj_pos_center(i)) < 50) {
                    coins_change(g, +1);
                    obj_delete(g, i);
                }
                break;
            }
            case OBJID_HEARTPIECE: {
                if (overlap_rec(ri, owlr)) {
                    heartpiece_on_collect(g, i);
                    return; // cutscene starts
                }
                break;
            }
            }
        }
    }
}