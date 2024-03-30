// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

void obj_game_player_jump_heads(game_s *g, obj_s *ohero)
{
    rec_i32 bot = obj_rec_bottom(ohero);

    for (obj_each(g, o)) {
        rec_i32 aabb = obj_aabb(o);
        rec_i32 rtop = {o->pos.x, o->pos.y, o->w, 1};

        if (!(o->flags & OBJ_FLAG_ENEMY)) continue;
        if (!overlap_rec(rtop, bot)) continue;

        // enemies
        enemy_s *e = &o->enemy;
        if (e->invincible) {
            // continue;
        }

        ohero->bumpflags |= OBJ_BUMPED_ON_HEAD | OBJ_BUMPED_Y;
        o->health -= 1;
        if (o->health <= 0) {
            obj_delete(g, o);
            snd_play_ext(e->sndID_die, 1.f, 1.f);
            texrec_s       trr   = o->sprites[0].trec;
            enemy_decal_s *decal = &g->enemy_decals[g->n_enemy_decals++];
            decal->pos.x         = o->pos.x + o->sprites[0].offs.x;
            decal->pos.y         = o->pos.y + o->sprites[0].offs.y;
            decal->t             = trr;
            decal->tick          = ENEMY_DECAL_TICK;

            int n_drops = 10;
            for (int n = 0; n < n_drops; n++) {
                coinparticle_s *c = coinparticle_create(g);
                if (!c) break;
                c->pos       = o->pos;
                c->vel_q8.y  = rngr_i32(-1500, -1000);
                c->vel_q8.x  = rngr_sym_i32(500);
                c->acc_q8.y  = 60;
                c->drag_q8.x = 254;
                c->drag_q8.y = 255;
                c->tick      = 1000;
            }

            particle_desc_s prt = {0};
            prt.p.p_q8          = v2_shl(obj_pos_center(o), 8);
            prt.p.v_q8.x        = rngr_sym_i32(1000);
            prt.p.v_q8.y        = -rngr_i32(500, 1000);
            prt.p.a_q8.y        = 25;
            prt.p.size          = 4;
            prt.p.ticks_max     = 100;
            prt.ticksr          = 20;
            prt.pr_q8.x         = 4000;
            prt.pr_q8.y         = 4000;
            prt.vr_q8.x         = 400;
            prt.vr_q8.y         = 400;
            prt.ar_q8.y         = 4;
            prt.sizer           = 2;
            prt.p.gfx           = PARTICLE_GFX_CIR;
            particles_spawn(g, &g->particles, prt, 30);
        } else {
            snd_play_ext(e->sndID_hurt, 1.f, 1.f);
            e->invincible = 10;

            switch (o->ID) {
            case OBJ_ID_NULL: break;
            }
        }
    }

    if (ohero->bumpflags & OBJ_BUMPED_ON_HEAD) {
        ohero->tomove.y           = -1;
        hero_s *hero              = (hero_s *)ohero->mem;
        hero->ground_impact_ticks = 6;
    }
}

void obj_game_player_attackbox(game_s *g, hitbox_s box)
{
    for (obj_each(g, o)) {
        rec_i32 aabb = obj_aabb(o);
        if (!overlap_rec(aabb, box.r)) continue;

        switch (o->ID) {
        case OBJ_ID_SWITCH: switch_on_interact(g, o); break;
        }

        if (o->flags & OBJ_FLAG_ENEMY) {
            enemy_s *e = &o->enemy;

            if (e->invincible || e->cannot_be_hurt) {
                switch (o->ID) {
                case OBJ_ID_CRAWLER: crawler_on_weapon_hit(g, o, box); break;
                }

                continue;
            }

            o->health -= box.damage;
            if (o->health <= 0) {
                obj_delete(g, o);
                snd_play_ext(e->sndID_die, 1.f, 1.f);
                texrec_s       trr   = o->sprites[0].trec;
                enemy_decal_s *decal = &g->enemy_decals[g->n_enemy_decals++];
                decal->pos.x         = o->pos.x + o->sprites[0].offs.x;
                decal->pos.y         = o->pos.y + o->sprites[0].offs.y;
                decal->t             = trr;
                decal->tick          = ENEMY_DECAL_TICK;

                for (int n = 0; n < 10; n++) {
                    coinparticle_s *c = coinparticle_create(g);
                    if (!c) {
                        break;
                    }
                    c->pos       = o->pos;
                    c->vel_q8.y  = rngr_i32(-1500, -1000);
                    c->vel_q8.x  = rngr_sym_i32(500);
                    c->acc_q8.y  = 60;
                    c->drag_q8.x = 254;
                    c->drag_q8.y = 255;
                    c->tick      = 1000;
                }

                particle_desc_s prt = {0};
                prt.p.p_q8          = v2_shl(obj_pos_center(o), 8);
                prt.p.v_q8.x        = rngr_sym_i32(1000);
                prt.p.v_q8.y        = -rngr_i32(500, 1000);
                prt.p.a_q8.y        = 25;
                prt.p.size          = 4;
                prt.p.ticks_max     = 100;
                prt.ticksr          = 20;
                prt.pr_q8.x         = 4000;
                prt.pr_q8.y         = 4000;
                prt.vr_q8.x         = 400;
                prt.vr_q8.y         = 400;
                prt.ar_q8.y         = 4;
                prt.sizer           = 2;
                prt.p.gfx           = PARTICLE_GFX_CIR;
                particles_spawn(g, &g->particles, prt, 30);
            } else {
                snd_play_ext(e->sndID_hurt, 1.f, 1.f);
                e->invincible = 15;

                switch (o->ID) {
                case OBJ_ID_NULL: break;
                }
            }
        }
    }
}