// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "app.h"
#include "render.h"

void game_tick_gameplay(game_s *g);

void game_init(game_s *g)
{
    map_world_load(&g->map_world, "world.world");
    pltf_log("GAME VERSION %u\n", GAME_VERSION);
    g->cam.mode = CAM_MODE_FOLLOW_HERO;
#if 0
    g->n_deco_verlet = 1;
    for (u32 k = 0; k < g->n_deco_verlet; k++) {
        g->deco_verlet[k].n_pt     = 10;
        g->deco_verlet[k].dist     = 128 << 2;
        g->deco_verlet[k].n_it     = 3;
        g->deco_verlet[k].grav.y   = 8 << 2;
        g->deco_verlet[k].haspos_2 = 1;
        g->deco_verlet[k].pos_2    = (v2_i16){64 << 6, 32};
        g->deco_verlet[k].pos      = (v2_i32){64 + k * 16, 32};
        for (u32 n = 0; n < g->deco_verlet[k].n_pt; n++) {
            g->deco_verlet[k].pt[n].p  = (v2_i16){0};
            g->deco_verlet[k].pt[n].pp = (v2_i16){0};
        }
    }
#endif
}

void game_tick(game_s *g)
{
    g->n_hitboxes = 0;
    g->save.tick++;
    if (g->hero_hurt_lowpass_tick) {
        g->hero_hurt_lowpass_tick--;
        aud_set_lowpass(((g->hero_hurt_lowpass_tick * 12) / 30));
    }

    if (g->substate) {
        g->freeze_tick = 0;
    } else if (0 < g->freeze_tick) {
        g->freeze_tick--;
    } else {
        game_tick_gameplay(g);
    }

    switch (g->substate) {
    case SUBSTATE_TEXTBOX:
        textbox_update(g);
        break;
    case SUBSTATE_MAPTRANSITION:
        maptransition_update(g);
        break;
    case SUBSTATE_GAMEOVER:
        gameover_update(g);
        break;
    case SUBSTATE_POWERUP:
        hero_powerup_update(g);
        break;
    }

    cam_update(g, &g->cam);
    g->cam_prev_world = cam_pos_px(g, &g->cam);

    if (g->areaname.fadeticks) {
        g->areaname.fadeticks++;
        if (FADETICKS_AREALABEL <= g->areaname.fadeticks) {
            g->areaname.fadeticks = 0;
        }
    }

    // save animation
    if (g->save_ticks) {
        g->save_ticks++;
        if (SAVE_TICKS <= g->save_ticks) {
            g->save_ticks = 0;
        }
    }

    // every other tick to save some CPU cycles;
    // split between even and uneven frames
    if (g->gameplay_tick & 1) {
        grass_animate(g);
    } else {
    }

    deco_verlet_animate(g);
    area_update(g, &g->area);
}

void game_tick_gameplay(game_s *g)
{
    g->gameplay_tick++;
    g->events_frame          = 0;
    g->hero_mem.interactable = obj_handle_from_obj(NULL);
    g->hero_mem.pushing      = 0;

    for (obj_each(g, o)) {
        o->v_prev_q8 = o->v_q8;

        switch (o->ID) {
        case OBJ_ID_HERO: hero_on_update(g, o); break;
        case OBJ_ID_HOOK: hook_update(g, o); break;
        case OBJ_ID_FLYBLOB: flyblob_on_update(g, o); break;
        case OBJ_ID_PROJECTILE: projectile_on_update(g, o); break;
        case OBJ_ID_BUDPLANT: budplant_on_update(g, o); break;
        case OBJ_ID_SPRITEDECAL: spritedecal_on_update(g, o); break;
        case OBJ_ID_PUSHBLOCK: pushblock_on_update(g, o); break;
        case OBJ_ID_NPC: npc_on_update(g, o); break;
        case OBJ_ID_HERO_POWERUP: hero_powerup_obj_on_update(g, o); break;
        }
    }
    for (obj_each(g, o)) {
        obj_try_wiggle(g, o);
    }

    rec_i32 roombounds = {0, 0, g->pixel_x, g->pixel_y};
    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_KILL_OFFSCREEN) && // out of bounds deletion
            !overlap_rec(obj_aabb(o), roombounds)) {
            if (o->ID == OBJ_ID_HERO) {
                rec_i32 roombounds_hero = {0, 0, g->pixel_x, g->pixel_y + 64};
                if (!overlap_rec(obj_aabb(o), roombounds_hero)) {
                    hero_kill(g, o);
                }

            } else {
                obj_delete(g, o);
            }
            continue;
        }

        if (o->enemy.die_tick) {
            o->enemy.die_tick--;
            if (o->enemy.die_tick == 2) {
                snd_play(SNDID_ENEMY_EXPLO, 4.f, 1.f);
            }
            if (o->enemy.die_tick == 0) {
                rec_i32 rdecal = {0, 128, 64, 64};
                v2_i32  pos    = obj_pos_center(o);
                pos.x -= 32;
                pos.y -= 32;
                spritedecal_create(g, 0x10000, NULL, pos, TEXID_EXPLOSIONS, rdecal, 30, 14, 0);
                // snd_play(SNDID_ENEMY_EXPLO, 4.f, 1.f);
                obj_delete(g, o);
                continue;
            }
        }

        if (o->enemy.hurt_tick) {
            o->enemy.hurt_tick--;
            o->enemy.hurt_shake_offs.x = rngr_sym_i32(3);
            o->enemy.hurt_shake_offs.y = rngr_sym_i32(3);
        } else {
            o->enemy.hurt_shake_offs.x = 0;
            o->enemy.hurt_shake_offs.y = 0;
        }
    }

    objs_cull_to_delete(g);
    coinparticle_update(g);
    particles_update(g, &g->particles);

    if (g->rope.active) {
        rope_update(g, &g->rope);
        rope_verletsim(g, &g->rope);
    }

    obj_s *ohero = NULL;

    if (hero_present_and_alive(g, &ohero)) {
        hero_process_hurting_things(g, ohero);

        if (ohero->health == 0) {
            gameover_start(g);
        }
    }

    if (hero_present_and_alive(g, &ohero)) {
        hero_post_update(g, ohero);
    }

#ifdef PLTF_DEBUG
    for (obj_each(g, o)) {
        assert(o->magic == OBJ_MAGIC);
    }
#endif

    if (g->events_frame & EVENT_HERO_DAMAGE) {
        g->freeze_tick            = 4;
        g->hero_hurt_lowpass_tick = 40;
    } else if (g->events_frame & EVENT_HIT_ENEMY) {
        g->freeze_tick = 2;
    }

    objs_cull_to_delete(g);

    // animate coin counter ui
    if (g->coins_added) {
        if (g->coins_added_ticks) {
            g->coins_added_ticks--;
        } else {
            i32 to_add = clamp_i32(g->coins_added, -2, +2);
            g->save.coins += to_add;
            g->coins_added -= to_add;
        }
    }

    for (obj_each(g, o)) {
        switch (o->ID) {
        case OBJ_ID_HERO: hero_on_animate(g, o); break;
        case OBJ_ID_HOOK: hook_on_animate(g, o); break;
        case OBJ_ID_FLYBLOB: flyblob_on_animate(g, o); break;
        case OBJ_ID_PROJECTILE: projectile_on_animate(g, o); break;
        case OBJ_ID_BUDPLANT: budplant_on_animate(g, o); break;
        case OBJ_ID_SPRITEDECAL: spritedecal_on_animate(g, o); break;
        case OBJ_ID_NPC: npc_on_animate(g, o); break;
        }
    }
}

void game_resume(game_s *g)
{
}

void game_paused(game_s *g)
{
}

i32 gameplay_time(game_s *g)
{
    return g->gameplay_tick;
}

i32 gameplay_time_since(game_s *g, i32 t)
{
    return (g->gameplay_tick - t);
}

void game_load_savefile(game_s *g)
{
    save_s *s = &g->save;

    if (s->tick == 0) { // new game

    } else { // continue
    }

    game_load_map(g, s->hero_mapfile);
    obj_s  *oh   = hero_create(g);
    hero_s *hero = &g->hero_mem;

    oh->pos.x = s->hero_pos.x - oh->w / 2;
    oh->pos.y = s->hero_pos.y - oh->h;
    game_prepare_new_map(g);
}

bool32 game_save_savefile(game_s *g)
{
    str_cpy(g->save.hero_mapfile, g->areaname.filename);
    g->save_ticks = 1;
    bool32 r      = savefile_write(g->save_slot, (const save_s *)&g->save);
    pltf_log("SAVED!\n");
    return (r);
}

void game_on_trigger(game_s *g, i32 trigger)
{
    if (!trigger) return;
    for (obj_each(g, o)) {
        if (o->on_trigger) {
            o->on_trigger(g, o, trigger);
        }
    }
}

i32 tick_to_index_freq(i32 tick, i32 n_frames, i32 freqticks)
{
    i32 i = ((tick * n_frames) / freqticks) % n_frames;
    assert(0 <= i && i < n_frames);
    return i;
}

void game_on_solid_appear(game_s *g)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        hero_check_rope_intact(g, ohero);
    }

    for (obj_each(g, o)) {
        obj_try_wiggle(g, o);
    }
}

bool32 obj_game_enemy_attackboxes(game_s *g, hitbox_s *boxes, i32 nb)
{
    obj_s *o = obj_get_tagged(g, OBJ_TAG_HERO);
    if (!o) return 0;

    for (i32 n = 0; n < nb; n++) {
        hitbox_s hb = boxes[n];
        pltf_debugr(hb.r.x + g->cam_prev.x, hb.r.y + g->cam_prev.y, hb.r.w, hb.r.h, 0, 0xFF, 0, 10);
    }

    rec_i32 aabb      = obj_aabb(o);
    i32     strongest = -1;
    for (i32 n = 0; n < nb; n++) {
        hitbox_s hb = boxes[n];
        if (!overlap_rec(aabb, hb.r)) continue;

        if (strongest < 0 || boxes[strongest].damage < hb.damage) {
            strongest = n;
        }
    }

    if (0 <= strongest) {
        hero_hurt(g, o, 1);
        snd_play(SNDID_SWOOSH, 0.5f, 0.5f);
        o->v_q8.x = (boxes[strongest].force_q8.x);
        o->bumpflags &= ~OBJ_BUMPED_Y; // have to clr y bump
        g->events_frame |= EVENT_HERO_DAMAGE;
        return 1;
    }
    return 0;
}

bool32 obj_game_player_attackbox_o(game_s *g, obj_s *o, hitbox_s box);

bool32 obj_game_player_attackboxes(game_s *g, hitbox_s *boxes, i32 nb)
{
    bool32 res = 0;

    for (i32 n = 0; n < nb; n++) {
        hitbox_s hb = boxes[n];
        pltf_debugr(hb.r.x, hb.r.y, hb.r.w, hb.r.h, 0, 0, 0xFF, 10);
    }

    for (obj_each(g, o)) {
        rec_i32 aabb = obj_aabb(o);

        i32 strongest = -1;
        for (i32 n = 0; n < nb; n++) {
            hitbox_s hb = boxes[n];
            if (!overlap_rec(aabb, hb.r)) continue;

            if (strongest < 0 || boxes[strongest].damage < hb.damage) {
                strongest = n;
            }
        }

        if (0 <= strongest) {
            res |= obj_game_player_attackbox_o(g, o, boxes[strongest]);
        }
    }
    return res;
}

bool32 obj_game_player_attackbox(game_s *g, hitbox_s box)
{
    return obj_game_player_attackboxes(g, &box, 1);
}

bool32 obj_game_player_attackbox_o(game_s *g, obj_s *o, hitbox_s box)
{
    switch (o->ID) {
    case OBJ_ID_SWITCH: switch_on_interact(g, o); break;
    default: break;
    }

    if ((o->flags & OBJ_FLAG_ENEMY) && 0 < o->health && !o->enemy.invincible) {
        g->freeze_tick     = max_i32(g->freeze_tick, 3);
        o->health          = max_i32((i32)o->health - box.damage, 0);
        o->enemy.hurt_tick = 15;
        if (o->health == 0) {
            o->enemy.die_tick = 10;
            snd_play(SNDID_ENEMY_DIE, 1.f, 1.f);
            return 1;
        }
        snd_play(SNDID_ENEMY_HURT, 1.f, 1.f);

        switch (o->ID) {
        case OBJ_ID_CRAWLER: crawler_on_weapon_hit(g, o, box); break;
        case OBJ_ID_FLYBLOB: flyblob_on_hit(g, o, box); break;
        default: break;
        }
        return 1;
    }
    return 0;
}