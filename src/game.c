// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "app.h"
#include "render.h"

void game_tick_gameplay(g_s *g);

void game_init(g_s *g)
{
    map_world_load(&g->map_world, "world.world");
    g->cam.mode = CAM_MODE_FOLLOW_HERO;
}

void game_tick(g_s *g)
{
#ifdef PLTF_SDL
    i32 ab = -1;
    if (pltf_sdl_jkey(SDL_SCANCODE_0)) ab = 0;
    if (pltf_sdl_jkey(SDL_SCANCODE_1)) ab = 1;
    if (pltf_sdl_jkey(SDL_SCANCODE_2)) ab = 2;
    if (pltf_sdl_jkey(SDL_SCANCODE_3)) ab = 3;
    if (pltf_sdl_jkey(SDL_SCANCODE_4)) ab = 4;
    if (pltf_sdl_jkey(SDL_SCANCODE_5)) ab = 5;
    if (pltf_sdl_jkey(SDL_SCANCODE_6)) ab = 6;
    if (pltf_sdl_jkey(SDL_SCANCODE_7)) ab = 7;
    if (pltf_sdl_jkey(SDL_SCANCODE_8)) ab = 8;
    if (pltf_sdl_jkey(SDL_SCANCODE_9)) ab = 9;
    if (0 <= ab) {
        if (8 <= ab) {
            i32 dt                    = ab == 8 ? -1 : +1;
            g->save.stamina_upgrades  = max_i32(g->save.stamina_upgrades + dt, 0);
            g->hero_mem.stamina       = hero_stamina_max(g, obj_get_hero(g));
            g->hero_mem.stamina_added = 0;
            pltf_log("# STAMINA: %i\n", g->save.stamina_upgrades);
        } else {
            if (hero_has_upgrade(g, ab)) {
                hero_rem_upgrade(g, ab);
            } else {
                hero_add_upgrade(g, ab);
            }
        }
    }

    if (pltf_sdl_jkey(SDL_SCANCODE_SPACE)) {
        obj_s *cp    = coin_create(g);
        obj_s *ohero = obj_get_hero(g);
        cp->pos.x    = (ohero->pos.x) + 50;
        cp->pos.y    = ohero->pos.y;
        cp->v_q8.y   = -300;
        cp->v_q8.x   = rngr_i32(-200, +200);
    }
#endif

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

void game_tick_gameplay(g_s *g)
{
    g->gameplay_tick++;
    g->events_frame = 0;

    objs_update(g);
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
    particles_update(g, &g->particles);

    obj_s *ohero = NULL;
    if (hero_present_and_alive(g, &ohero)) {
        hero_post_update(g, ohero);

        if (ohero->health == 0) {
            gameover_start(g);
        }
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

    objs_animate(g);
}

void game_resume(g_s *g)
{
}

void game_paused(g_s *g)
{
}

i32 gameplay_time(g_s *g)
{
    return g->gameplay_tick;
}

i32 gameplay_time_since(g_s *g, i32 t)
{
    return (g->gameplay_tick - t);
}

void game_load_savefile(g_s *g)
{
    save_s *s = &g->save;

    if (s->tick == 0) { // new game

    } else { // continue
    }

    game_load_map(g, s->hero_mapfile);
    obj_s *oh = hero_create(g);

    oh->pos.x = s->hero_pos.x - oh->w / 2;
    oh->pos.y = s->hero_pos.y - oh->h;
    game_prepare_new_map(g);
}

bool32 game_save_savefile(g_s *g)
{
    str_cpy(g->save.hero_mapfile, g->areaname.filename);
    g->save_ticks = 1;
    bool32 r      = savefile_write(g->save_slot, (const save_s *)&g->save);
    pltf_log("SAVED!\n");
    return (r);
}

void game_on_trigger(g_s *g, i32 trigger)
{
    if (trigger) {
        objs_trigger(g, trigger);
    }
}

i32 tick_to_index_freq(i32 tick, i32 n_frames, i32 freqticks)
{
    i32 i = ((tick * n_frames) / freqticks) % n_frames;
    assert(0 <= i && i < n_frames);
    return i;
}

void game_on_solid_appear(g_s *g)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        hero_check_rope_intact(g, ohero);
    }

    for (obj_each(g, o)) {
        obj_try_wiggle(g, o);
    }
}

bool32 obj_game_enemy_attackboxes(g_s *g, hitbox_s *boxes, i32 nb)
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
        o->bumpflags &= ~OBJ_BUMP_Y; // have to clr y bump
        g->events_frame |= EVENT_HERO_DAMAGE;
        return 1;
    }
    return 0;
}

bool32 obj_game_player_attackbox_o(g_s *g, obj_s *o, hitbox_s box);

bool32 obj_game_player_attackboxes(g_s *g, hitbox_s *boxes, i32 nb)
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

bool32 obj_game_player_attackbox(g_s *g, hitbox_s box)
{
    return obj_game_player_attackboxes(g, &box, 1);
}

bool32 obj_game_player_attackbox_o(g_s *g, obj_s *o, hitbox_s box)
{
    switch (o->ID) {
    default: break;
    case OBJ_ID_SWITCH: switch_on_interact(g, o); break;
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
        default: break;
        case OBJ_ID_CRAWLER: crawler_on_weapon_hit(g, o, box); break;
        case OBJ_ID_FLYBLOB: flyblob_on_hit(g, o, box); break;
        }
        return 1;
    }
    return 0;
}