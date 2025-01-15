// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "app.h"
#include "render.h"

void game_tick_gameplay(g_s *g);

void game_init(g_s *g)
{
    g->cam.mode      = CAM_MODE_FOLLOW_HERO;
    g->obj_head_free = &g->obj_raw[0];
    for (i32 n = 0; n < NUM_OBJ; n++) {
        obj_s *o = &g->obj_raw[n];
        o->GID   = (u32)n << OBJ_GID_INDEX_SH;
        if (n < NUM_OBJ - 1) {
            o->next = &g->obj_raw[n + 1];
        }
    }
}

void game_tick(g_s *g)
{
#if PLTF_DEV_ENV
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
            i32 dt                   = ab == 8 ? -1 : +1;
            g->hero.stamina_upgrades = max_i32(g->hero.stamina_upgrades + dt, 0);
            g->hero.stamina          = hero_stamina_max(g, obj_get_hero(g));
            g->hero.stamina_added    = 0;
            pltf_log("# STAMINA: %i\n", g->hero.stamina_upgrades);
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

    if (g->hero_hurt_lp_tick) {
        g->hero_hurt_lp_tick--;
        i32 lp = lerp_i32(0, 12, g->hero_hurt_lp_tick, HERO_HURT_LP_TICKS);
        aud_set_lowpass(lp);
    }

    bool32 tick_gp = 1;
    if (g->substate && g->substate != SUBSTATE_GAMEOVER) {
        g->freeze_tick = 0;
        tick_gp        = 0;
    } else if (0 < g->freeze_tick) {
        g->freeze_tick--;
        tick_gp = 0;
    } else {
        obj_s *ohero = obj_get_hero(g);
        if (ohero) {
            hero_s *h = (hero_s *)ohero->heap;
            if (g->grapple_tick) {
                if (inp_btn(INP_B)) {
                    g->grapple_tick++;
                    if (10 <= g->grapple_tick) {
                        i32 gt = max_i32(0, 50 - (g->grapple_tick - 10)) / 8;
                        gt     = max_i32(gt, 2);
                        if ((g->grapple_tick % gt) == 0) {
                            tick_gp = 0;
                        }
                    }
                } else {
                    g->grapple_tick = 0;
                    hero_item_button(g, inp_cur(), ohero);
                }
            }
        }
    }

    if (tick_gp) {
        game_tick_gameplay(g);
    }

    switch (g->substate) {
    case SUBSTATE_TEXTBOX:
        dialog_update(g);
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
    g->cam_prev_world = cam_pos_px_top_left(g, &g->cam);

    // save animation
    if (g->save_ticks) {
        g->save_ticks++;
        if (SAVE_TICKS <= g->save_ticks) {
            g->save_ticks = 0;
        }
    }

    // every other tick to save some CPU cycles;
    // split between even and uneven frames
    if (g->tick & 1) {
        grass_animate(g);
    } else {
        deco_verlet_animate(g);
    }

    area_update(g, &g->area);
}

void game_tick_gameplay(g_s *g)
{
    g->tick++;
    g->events_frame = 0;

    boss_update(g, &g->boss);
    areafx_snow_update(g, &g->area.fx.snow);
    areafx_rain_update(g, &g->area.fx.rain);
    areafx_heat_update(g, &g->area.fx.heat);

    obj_s *ohero = obj_get_hero(g);
    if (ohero) {
        inp_s heroinp = inp_cur();
        hero_on_update(g, ohero, heroinp);
    }

    objs_update(g);
    for (obj_each(g, o)) {
        obj_try_wiggle(g, o);
    }

    rec_i32 roombounds = {0, 0, g->pixel_x, g->pixel_y};
    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_KILL_OFFSCREEN) && // out of bounds deletion
            !overlap_rec(obj_aabb(o), roombounds)) {
            if (o->ID == OBJID_HERO) {
                rec_i32 roombounds_hero = {0, 0, g->pixel_x, g->pixel_y + 64};
                if (!overlap_rec(obj_aabb(o), roombounds_hero)) {
                    // hero_kill(g, o);
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
                spritedecal_create(g, 0x10000, 0, pos, TEXID_EXPLOSIONS, rdecal, 30, 14, 0);
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

    if (hero_present_and_alive(g, &ohero)) {
        inp_s heroinp = inp_cur();
        hero_post_update(g, ohero, heroinp);

        if (ohero->health == 0) {
            gameover_start(g);
        }
    }

#if PLTF_DEBUG
    for (obj_each(g, o)) {
        assert(o->magic == OBJ_MAGIC);
    }
#endif

    if (g->events_frame & EVENT_HERO_DAMAGE) {
        g->freeze_tick       = max_i32(g->freeze_tick, 4);
        g->hero_hurt_lp_tick = HERO_HURT_LP_TICKS;
    }
    if (g->events_frame & EVENT_HERO_DEATH) {
        g->freeze_tick       = max_i32(g->freeze_tick, 25);
        g->hero_hurt_lp_tick = HERO_HURT_LP_TICKS;
    }
    if (g->events_frame & EVENT_HIT_ENEMY) {
        g->freeze_tick = max_i32(g->freeze_tick, 2);
    }

    objs_cull_to_delete(g);
    objs_animate(g);

    // animate coin counter ui
    if (g->coins_added) {
        if (g->coins_added_ticks) {
            g->coins_added_ticks--;
        } else {
            i32 to_add = clamp_sym_i32(g->coins_added, 1);
            g->hero.coins += to_add;
            g->coins_added -= to_add;
        }
    }
}

void game_resume(g_s *g)
{
}

void game_paused(g_s *g)
{
}

i32 gameplay_time(g_s *g)
{
    return g->tick;
}

void *game_alloc(g_s *g, usize s, usize alignment)
{
    return marena_alloc_aligned(&g->memarena, s, alignment);
}

void *game_alloc_aligned_f(void *ctx, usize s, usize alignment)
{
    return game_alloc((g_s *)ctx, s, alignment);
}

allocator_s game_allocator(g_s *g)
{
    allocator_s a = {game_alloc_aligned_f, g};
    return a;
}

i32 gameplay_time_since(g_s *g, i32 t)
{
    return (g->tick - t);
}

void game_load_savefile(g_s *g)
{
    spm_push();
    save_s *s   = spm_alloct(save_s, 1);
    i32     res = savefile_r(g->save_slot, s);
    if (res != 0) {
    }

    if (s->tick == 0) { // new game

    } else { // continue
    }

    obj_delete(g, obj_get_hero(g));
    mclr(&g->hero, sizeof(hero_s));
    objs_cull_to_delete(g);

    obj_s *o         = hero_create(g);
    g->hero.upgrades = s->upgrades;
    g->hero.stamina  = s->stamina;
    o->pos.x         = s->hero_pos.x - o->w / 2;
    o->pos.y         = s->hero_pos.y - o->h;
    game_load_map(g, s->map_hash);
    pltf_sync_timestep();
}

bool32 game_save_savefile(g_s *g)
{
    spm_push();
    save_s *s = spm_alloctz(save_s, 1);
    {
        s->tick      = g->tick;
        s->upgrades  = g->hero.upgrades;
        s->n_saveIDs = g->n_saveIDs;
        mcpy(s->saveIDs, g->saveIDs, sizeof(s->saveIDs));
        mcpy(s->name, g->hero.name, sizeof(s->name));
        s->map_hash = g->map_hash;
    }
    i32 res = savefile_w(g->save_slot, s);
    spm_pop();
    pltf_sync_timestep();
    return (res == 0);
}

void game_on_trigger(g_s *g, i32 trigger)
{
    if (trigger) {
        pltf_log("trigger %i\n", trigger);
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
    case OBJID_SWITCH: switch_on_interact(g, o); break;
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
        case OBJID_CRAWLER: crawler_on_weapon_hit(g, o, box); break;
        case OBJID_FLYBLOB: flyblob_on_hit(g, o, box); break;
        }
        return 1;
    }
    return 0;
}

void game_open_map(void *ctx, i32 opt)
{
    g_s *g = (g_s *)ctx;
    pltf_log("Open MAP!\n");
}

void game_unlock_map(g_s *g)
{
    saveID_put(g, SAVEID_UNLOCKED_MAP);
#ifdef PLTF_PD
    pltf_pd_menu_add("Map", game_open_map, g);
#endif
}