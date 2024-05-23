// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"

void game_tick_gameplay(game_s *g);
void game_objs_update(game_s *g);
void game_objs_move(game_s *g);

void game_init(game_s *g)
{
#ifdef SYS_SDL
    sys_set_volume(0.25f);
#endif
    mus_set_trg_vol(0.1f);
    g->cam.mode = CAM_MODE_FOLLOW_HERO;

    map_world_load(&g->map_world, "world.world");
    lighting_init(&g->lighting);
    g->lighting.n_lights    = 1;
    g->lighting.lights[0].r = 100;

    sys_printf("GAME VERSION %u\n", GAME_VERSION);
}

void game_tick(game_s *g)
{
    bool32 update_gameplay = 0;
    switch (g->substate) {
    case SUBSTATE_GAMEOVER: gameover_update(g); break;
    case SUBSTATE_TEXTBOX: textbox_update(g); break;
    case SUBSTATE_MAPTRANSITION: maptransition_update(g); break;
    case SUBSTATE_HEROUPGRADE: heroupgrade_update(g); break;
    case SUBSTATE_MENUSCREEN: menu_screen_update(g, &g->menu_screen); break;
    case SUBSTATE_FREEZE: g->freeze_tick = max_i32(g->freeze_tick - 1, 0); break;
    default: update_gameplay = 1; break;
    }

    if (!update_gameplay) {
        g->item_select.docked = 0;
    }

    if (update_gameplay) {
        g->gameplay_tick++;
        game_tick_gameplay(g);

        for (obj_each(g, o)) {
            if (o->on_animate) {
                o->on_animate(g, o);
            }
        }

        obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
        if (ohero) {
            const rec_i32 heroaabb = obj_aabb(ohero);
            for (int n = 0; n < g->n_wiggle_deco; n++) {
                wiggle_deco_s *wd = &g->wiggle_deco[n];
                if (wd->t) {
                    wd->t--;
                    wd->tr.r.x = 64 * ((wd->t >> 2) & 3);
                }

                if (overlap_rec(wd->r, heroaabb)) {
                    if (!wd->overlaps) {
                        wd->overlaps = 1;
                        wd->t        = 20;
                    }
                } else {
                    wd->overlaps = 0;
                }
            }
        }

        for (int n = g->n_enemy_decals - 1; 0 <= n; n--) {
            g->enemy_decals[n].tick--;
            if (g->enemy_decals[n].tick <= 0) {
                g->enemy_decals[n] = g->enemy_decals[--g->n_enemy_decals];
            }
        }
    }

    cam_update(g, &g->cam);
    if (g->areaname.fadeticks) {
        g->areaname.fadeticks++;
        if (FADETICKS_AREALABEL <= g->areaname.fadeticks) {
            g->areaname.fadeticks = 0;
        }
    }

    if (g->save_ticks) {
        g->save_ticks++;
        if (SAVE_TICKS <= g->save_ticks) {
            g->save_ticks = 0;
        }
    }

    // every other tick to save some CPU cycles;
    // split between even and uneven frames
    if (sys_tick() & 1) {
        backforeground_animate_grass(g);
    } else {
    }

    area_update(g, &g->area);
}

void game_tick_gameplay(game_s *g)
{
    item_select_update(&g->item_select);
    g->events_frame          = 0;
    g->hero_mem.interactable = obj_handle_from_obj(NULL);

    game_objs_update(g);
    game_objs_move(g);

    // out of bounds deletion
    const rec_i32 roombounds = {0, 0, g->pixel_x, g->pixel_y};
    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_KILL_OFFSCREEN) &&
            !overlap_rec(obj_aabb(o), roombounds)) {
            assert(o->ID != OBJ_ID_HERO);
            obj_delete(g, o);
        }
    }

    objs_cull_to_delete(g);
    coinparticle_update(g);

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        if (ohero->health) {
            hero_post_update(g, ohero);
        } else {
        }
    }

#ifdef SYS_DEBUG
    for (obj_each(g, o)) {
        assert(o->magic == OBJ_MAGIC);
    }
#endif

    if (!g->substate) {
        if (g->events_frame & EVENT_HERO_DAMAGE) {
            g->freeze_tick            = 4;
            g->hero_mem.was_hit_ticks = 30;
        } else if (g->events_frame & EVENT_HIT_ENEMY) {
            g->freeze_tick = 2;
        }
        if (g->freeze_tick) {
            g->substate = SUBSTATE_FREEZE;
        }
    }

    if (g->hero_mem.was_hit_ticks) {
        g->hero_mem.was_hit_ticks--;
        aud_set_lowpass(((g->hero_mem.was_hit_ticks * 12) / 30));
    }

    objs_cull_to_delete(g);
    particles_update(g, &g->particles);

    if (g->coins_added) {
        if (g->coins_added_ticks) {
            g->coins_added_ticks--;
        } else {
            i32 to_add = clamp_i(g->coins_added, -2, +2);
            g->save.coins += to_add;
            g->coins_added -= to_add;
        }
    }
}

void game_objs_update(game_s *g)
{
    for (obj_each(g, o)) {
        const v2_i32 posprev = o->pos;

        if (o->on_update) {
            o->on_update(g, o);
        }

        o->posprev = posprev;

        if (0 < o->invincible_tick) {
            o->invincible_tick--;
        }

#ifdef SYS_DEBUG
        assert(o->magic == OBJ_MAGIC);
        assert((o->flags & (OBJ_FLAG_PLATFORM | OBJ_FLAG_SOLID)) !=
               (OBJ_FLAG_PLATFORM | OBJ_FLAG_SOLID));
#endif
    }
}

void game_objs_move(game_s *g)
{
    // apply movement
    for (obj_each(g, o)) { // integrate acc, vel and drag: adds tomove accumulator
        if (o->flags & OBJ_FLAG_MOVER) {
            obj_apply_movement(o);
        }
    }

    for (obj_each(g, o)) {
        o->moverflags |= OBJ_MOVER_MAP;
        obj_move(g, o, o->tomove);
        o->tomove.x = 0, o->tomove.y = 0;
    }
}

void game_resume(game_s *g)
{
    g->item_select.docked = 0;
}

void game_paused(game_s *g)
{
    if (g->state == APP_STATE_GAME) {
        render_pause(g);
    }
}

i32 gameplay_time(game_s *g)
{
    return g->gameplay_tick;
}

i32 gameplay_time_since(game_s *g, i32 t)
{
    return (g->gameplay_tick - t);
}

bool32 game_load_savefile(game_s *g)
{
    void *f = sys_file_open(SAVEFILE_NAME, SYS_FILE_R);

    if (!f) {
        sys_printf("New game\n");
        return 1;
    }

    save_s *hs = &g->save;

    i32    bread   = sys_file_read(f, hs, sizeof(save_s));
    i32    eclosed = sys_file_close(f);
    bool32 success = (bread == (i32)sizeof(save_s) && eclosed == 0);
    if (!success) {
        sys_printf("+++ Error loading savefile!\n");
        BAD_PATH
        return 0;
    }

    game_load_map(g, hs->hero_mapfile);
    obj_s  *oh   = hero_create(g);
    hero_s *hero = &g->hero_mem;

    oh->pos.x = hs->hero_pos.x - oh->w / 2;
    oh->pos.y = hs->hero_pos.y - oh->h;
    game_prepare_new_map(g);
    return success;
}

bool32 game_save_savefile(game_s *g)
{
    save_s *hs = &g->save;

    void *f = sys_file_open(SAVEFILE_NAME, SYS_FILE_W);
    if (!f) {
        sys_printf("+++ Can't write savefile!\n");
        return 0;
    }
    sys_printf("SAVED!\n");
    g->save_ticks = 1;
    i32 bwritten  = sys_file_write(f, (const void *)hs, sizeof(save_s));
    i32 eclosed   = sys_file_close(f);
    return (bwritten == (i32)sizeof(save_s) && eclosed == 0);
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

void game_put_grass(game_s *g, i32 tx, i32 ty)
{
    if (g->n_grass >= ARRLEN(g->grass)) return;
    grass_s *gr = &g->grass[g->n_grass++];
    *gr         = (grass_s){0};
    gr->pos.x   = tx * 16;
    gr->pos.y   = ty * 16;
    gr->type    = rngr_i32(0, 2);
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
        if (o->flags & OBJ_FLAG_ACTOR) {
            actor_try_wiggle(g, o);
        }
    }
}

void obj_game_player_attackbox_o(game_s *g, obj_s *o, hitbox_s box);

void obj_game_player_attackbox(game_s *g, hitbox_s box)
{
    for (obj_each(g, o)) {
        rec_i32 aabb = obj_aabb(o);
        if (!overlap_rec(aabb, box.r)) continue;
        obj_game_player_attackbox_o(g, o, box);
    }
}

void obj_game_player_attackbox_o(game_s *g, obj_s *o, hitbox_s box)
{
    switch (o->ID) {
    case OBJ_ID_SWITCH: switch_on_interact(g, o); break;
    }
}

static void *game_alloc_ctx(void *ctx, usize s)
{
    NOT_IMPLEMENTED
    game_s *g = (game_s *)ctx;
    return NULL;
}

alloc_s game_allocator(game_s *g)
{
    alloc_s a = {game_alloc_ctx, (void *)g};
    return a;
}

void backforeground_animate_grass(game_s *g)
{
    for (i32 n = 0; n < g->n_grass; n++) {
        grass_s *gr = &g->grass[n];
        rec_i32  r  = {gr->pos.x, gr->pos.y, 16, 16};

        for (obj_each(g, o)) {
            if ((o->flags & OBJ_FLAG_MOVER) && overlap_rec(r, obj_aabb(o))) {
                gr->v_q8 += o->vel_q8.x >> 4;
            }
        }

        gr->v_q8 += rngr_sym_i32(6) - ((gr->x_q8 * 15) >> 8);
        gr->x_q8 += gr->v_q8;
        gr->x_q8 = clamp_i32(gr->x_q8, -256, +256);
        gr->v_q8 = (gr->v_q8 * 230) >> 8;
    }
}