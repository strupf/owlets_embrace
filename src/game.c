// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "app.h"
#include "render.h"

void game_tick_gameplay(g_s *g);

void game_init(g_s *g)
{
    g->savefile      = &APP.save;
    g->obj_head_free = &g->obj_raw[0];
    for (i32 n = 1; n < NUM_OBJ; n++) {
        obj_s *o = &g->obj_raw[n - 1];
        o->next  = &g->obj_raw[n];
    }

    marena_init(&g->memarena, g->mem, sizeof(g->mem));
    wad_el_s *we      = 0;
    void     *f       = wad_open_str("WORLD", 0, &we);
    i32       n_rooms = 0;
    pltf_file_r(f, &n_rooms, sizeof(i32));

    assert(n_rooms <= GAME_N_ROOMS);
    g->n_map_rooms = 0;

    i32 x1 = I32_MAX;
    i32 y1 = I32_MAX;
    i32 x2 = I32_MIN;
    i32 y2 = I32_MIN;
    for (i32 k = 0; k < n_rooms; k++) {
        map_room_s     mr  = {0};
        map_room_wad_s mrw = {0};
        pltf_file_r(f, &mrw, sizeof(map_room_wad_s));
        mcpy(mr.map_name, mrw.map_name, sizeof(mr.map_name));
        mr.x     = mrw.x;
        mr.y     = mrw.y;
        mr.w     = mrw.w;
        mr.h     = mrw.h;
        mr.musID = mrw.musID;
        mr.flags = mrw.flags;
        mr.t     = tex_create(mrw.w, mrw.h, 0, app_allocator(), 0);
        pltf_file_r(f, mr.t.px, tex_size_bytes(mr.t));
        g->map_rooms[g->n_map_rooms++] = mr;

        x1 = min_i32(x1, mr.x / 25);
        y1 = min_i32(y1, mr.y / 15);
        x2 = max_i32(x2, (mr.x + mr.w) / 25);
        y2 = max_i32(y2, (mr.y + mr.h) / 15);
    }
    pltf_file_close(f);
    assert(0 <= x1 && 0 <= y1 && x2 < MINIMAP_SCREENS_X && y2 < MINIMAP_SCREENS_Y);
    pltf_log("%i | %i | %i | %i\n", x1, y1, x2, y2);
}

void game_tick(g_s *g, inp_state_s inpstate)
{
    static bool32 once = 0;
    if (!once) {
        once = 1;
    }
    g->tick++;
    g->tick_animation++;
    g->inp.p = g->inp.c;
    g->inp.c = inpstate;

    if (g->hurt_lp_tick) {
        g->hurt_lp_tick--;

        // works for now
        switch (g->hurt_lp_tick) {
        case 80 - 3 * 0: aud_lowpass(1); break;
        case 80 - 3 * 1: aud_lowpass(2); break;
        case 80 - 3 * 2: aud_lowpass(3); break;
        case 80 - 3 * 3: aud_lowpass(4); break;
        case 80 - 3 * 4: aud_lowpass(5); break;
        case 80 - 3 * 5: aud_lowpass(6); break;
        case 0 + 6 * 5: aud_lowpass(5); break;
        case 0 + 6 * 4: aud_lowpass(4); break;
        case 0 + 6 * 3: aud_lowpass(3); break;
        case 0 + 6 * 2: aud_lowpass(2); break;
        case 0 + 6 * 1: aud_lowpass(1); break;
        case 0 + 6 * 0: aud_lowpass(0); break;
        }
    }

    bool32 do_tick = 1;
    bool32 do_anim = 1;
    if (0 < g->freeze_tick) {
        g->freeze_tick--;
        do_tick = 0;
        do_anim = 0;
    }

    if (g->dia.state) {
        dia_update(g, inp_cur());
    }

    cs_s *cs = &g->cs;
    if (cs->on_update) {
        cs->tick++;
        if (cs->on_update) {
            cs->on_update(g, cs, inp_cur());
        }
    }

    if (g->flags & GAME_FLAG_BLOCK_UPDATE) {
        do_tick = 0;
    }

    if (g->minimap.state) {
        do_tick = 0;
        minimap_update(g);
    }

    if (do_tick) {
        game_tick_gameplay(g);
    }

    if (do_anim) {
        game_anim(g);
    }
}

void game_tick_gameplay(g_s *g)
{
    g->tick_gameplay++;
    g->events_frame = 0;
    g->n_hitboxes   = 0;

    boss_update(g);
    battleroom_on_update(g);

#if 0
    if (pltf_sdl_jkey(SDL_SCANCODE_SPACE)) {
        owl_kill(g, obj_get_owl(g));
    }
#endif

    obj_s *owl  = obj_get_owl(g);
    inp_s  hinp = {0};
    if (owl && !(owl->flags & OBJ_FLAG_DONT_UPDATE)) {
        owl_s *h = (owl_s *)&g->owl;

        bool32 control_hero = owl->health &&
                              !(g->flags & GAME_FLAG_BLOCK_PLAYER_INPUT) &&
                              !h->squish;
        if (control_hero) {
            hinp = g->inp;
        }
        owl_on_update(g, owl, hinp);
    }

    for (obj_each(g, o)) {
        if (o->flags & OBJ_FLAG_DONT_UPDATE) continue;

        o->v_prev_q12    = o->v_q12;
        bool32 do_update = 1;

        if (o->flags & OBJ_FLAG_ENEMY) {
            enemy_s *enemy = &o->enemy;
            v2_i32   pos   = obj_pos_center(o);
            do_update      = (enemy->hurt_tick == 0);

            if (enemy->flash_tick) {
                enemy->flash_tick--;
            }
            if (0 < enemy->hurt_tick) {
                enemy->hurt_tick--;
            }
            if (enemy->hurt_tick < 0) {
                enemy->hurt_tick++;
                i32 htick = -enemy->hurt_tick;

                if (htick == 0) {
                    if (enemy->coins_on_death) {
                        for (i32 n = 0; n < 5; n++) {
                            obj_s *ocoin   = coin_create(g);
                            ocoin->pos.x   = pos.x - (ocoin->w >> 1);
                            ocoin->pos.y   = pos.y - (ocoin->h >> 1) - 10;
                            ocoin->v_q12.y = -rngr_i32(Q_VOBJ(4.0), Q_VOBJ(6.0));
                            ocoin->v_q12.x = rngr_sym_i32(Q_VOBJ(1.5));
                        }
                        v2_i32 phealth   = {pos.x, pos.y - 24};
                        obj_s *ohealth   = healthdrop_spawn(g, phealth);
                        ohealth->v_q12.y = -Q_VOBJ(2.0);
                    }
                    obj_delete(g, o);
                }
            }
        }

        if (do_update && o->on_update) {
            o->on_update(g, o);
        }
    }

    hitboxes_flush(g);
    grapplinghook_calc_f_internal(g, &g->ghook);

    rec_i32 roombounds = {0, 0, g->pixel_x, g->pixel_y};
    for (obj_each(g, o)) {
#if PLTF_DEBUG
        assert(o->magic == OBJ_MAGIC);
#endif

        if (!obj_try_wiggle(g, o)) {
            o->on_squish(g, o);
        }

        // out of bounds deletion
        if ((o->flags & OBJ_FLAG_KILL_OFFSCREEN) &&
            !overlap_rec(obj_aabb(o), roombounds)) {
            obj_delete(g, o);
            continue;
        }

        // still ignore "ignored solids"? = still solid and overlapped
        rec_i32 r = obj_aabb(o);
        for (i32 n = o->n_ignored_solids - 1; 0 <= n; n--) {
            obj_s *i = obj_from_handle(o->ignored_solids[n]);

            if (i && (i->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(i)))
                continue;
            o->ignored_solids[n] = o->ignored_solids[--o->n_ignored_solids];
        }
    }

    objs_cull_to_delete(g);
    if ((owl = owl_if_present_and_alive(g))) {
        owl_on_update_post(g, owl, hinp);
    }
    objs_cull_to_delete(g);

    if (g->events_frame & EVENT_HIT_ENEMY) {
        g->freeze_tick = max_i32(g->freeze_tick, 2);
    }
}

void game_anim(g_s *g)
{
    cam_update(g, &g->cam);
    g->cam_center = cam_pos_px_center(&g->cam);
    aud_set_pos_cam(g->cam_center.x, g->cam_center.y, 1);
    background_update(g);
    boss_animate(g);

    if (g->health_ui_show && g->health_ui_fade < HEALTH_UI_TICKS) {
        g->health_ui_fade++;
    } else if (!g->health_ui_show && g->health_ui_fade) {
        g->health_ui_fade--;
    }

    switch (g->area_anim_st) {
    case AREANAME_ST_INACTIVE: break;
    case AREANAME_ST_DELAY:
        if (AREANAME_TICKS_DELAY <= ++g->area_anim_tick) {
            g->area_anim_st++;
            g->area_anim_tick = 0;
        }
        break;
    case AREANAME_ST_FADE_IN:
        if (AREANAME_TICKS_IN <= ++g->area_anim_tick) {
            g->area_anim_st++;
            g->area_anim_tick = 0;
        }
        break;
    case AREANAME_ST_SHOW:
        if (AREANAME_TICKS_SHOW <= ++g->area_anim_tick) {
            g->area_anim_st++;
            g->area_anim_tick = 0;
        }
        break;
    case AREANAME_ST_FADE_OUT:
        if (AREANAME_TICKS_OUT <= ++g->area_anim_tick) {
            g->area_anim_st   = AREANAME_ST_INACTIVE;
            g->area_anim_tick = 0;
        }
        break;
    }

    switch (g->vfx_ID) {
    default: break;
    case VFX_ID_SNOW: vfx_area_snow_update(g); break;
    }

    // every other tick to save some CPU cycles;
    // split between even and uneven frames
    if (g->tick & 1) {
        grass_animate(g);
    } else {
        deco_verlet_animate(g);
    }

    rec_i32 camr = {g->cam_center.x - CAM_WH - 32, g->cam_center.y - CAM_HH - 32, CAM_W + 64, CAM_H + 64};

    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_s *fa = &g->fluid_areas[n];
        rec_i32       rf = {fa->x, fa->y, fa->w, 16};

        // only update if a recent impact happened or it is in view
        if (fa->ticks_to_idle || overlap_rec(camr, rf)) {
            fluid_area_update(&g->fluid_areas[n]);
        }
    }

    particle_sys_update(g);
    coins_update(g);
    objs_animate(g);
    // hero_animate_ui(g);
}

void game_resume(g_s *g)
{
}

void game_paused(g_s *g)
{
#if PLTF_PD
    minimap_draw_pause(g);
    tex_s t = asset_tex(TEXID_PAUSE_TEX);
    pltf_pd_menu_image_upd(t.px, t.wword, t.w, t.h);
    if (g->minimap.state) {
        pltf_pd_menu_image_put(100);
    } else {
        pltf_pd_menu_image_put(0);
    }
#endif
}

i32 gameplay_time(g_s *g)
{
    return g->tick;
}

void *game_alloc_room(g_s *g, usize s, usize alignment)
{
    void *mem = marena_alloc_aligned(&g->memarena, s, alignment);
    mclr(mem, s);
    return mem;
}

void *game_alloc_aligned_f(void *ctx, usize s, usize alignment)
{
    return game_alloc_room((g_s *)ctx, s, alignment);
}

allocator_s game_allocator_room(g_s *g)
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
    savefile_s *s = g->savefile;
    if (s->tick == 0) { // new game

    } else { // continue
    }

    obj_delete(g, obj_get_owl(g));
    mclr(&g->owl, sizeof(owl_s));
    objs_cull_to_delete(g);

    obj_s *o = owl_create(g);
    owl_s *h = (owl_s *)o->heap;
    {
        mcpy(g->saveIDs, s->save, sizeof(g->saveIDs));
        mcpy(h->name, s->name, sizeof(h->name));
        mcpy(g->minimap.pins, s->pins, sizeof(g->minimap.pins));
        for (i32 n = 0; n < ARRLEN(s->map_visited); n++) {
            u32 v                            = s->map_visited[n];
            g->minimap.visited[(n << 1) + 0] = v;
            g->minimap.visited[(n << 1) + 1] = v;
        }
        g->tick             = s->tick;
        h->upgrades         = s->upgrades;
        g->minimap.n_pins   = s->n_map_pins;
        g->coins.n          = s->coins;
        o->health_max       = s->health_max;
        o->health           = o->health_max;
        h->stamina_upgrades = s->stamina;
        h->stamina_max      = h->stamina_upgrades * OWL_STAMINA_PER_CONTAINER;
        h->stamina          = h->stamina_max;
        mcpy(g->map_name, s->map_name, sizeof(s->map_name));
        mcpy(g->enemy_killed, s->enemy_killed, sizeof(s->enemy_killed));
    }

    game_load_map(g, s->map_name);

    obj_s *ocomp = 0;
    if (saveID_has(g, SAVEID_COMPANION_FOUND)) {
        ocomp = companion_create(g);
    }

    bool32 saveroom_pos_hero = 0;
    bool32 saveroom_pos_comp = 0;
    for (map_obj_each(g, i)) {
        if (i->hash == hash_str("saveroom_hero")) {
            obj_place_to_map_obj(o, i, 0, +1);
            saveroom_pos_hero = 1;
        }
        if (i->hash == hash_str("saveroom_comp") && ocomp) {
            obj_place_to_map_obj(ocomp, i, 0, +1);
            ocomp->pos.y -= 4;
            saveroom_pos_comp = 1;
        }
    }

    if (!saveroom_pos_hero) {
        o->pos.x = s->hero_pos.x - o->w / 2;
        o->pos.y = s->hero_pos.y - o->h;
    }
    if (!saveroom_pos_comp && ocomp) {
        ocomp->pos.x  = o->pos.x + 0;
        ocomp->pos.y  = o->pos.y - 30;
        ocomp->facing = o->facing;
    }

    cam_hard_set_positon(g, &g->cam);
    sfx_block_new(1); // disable sounds (foot steps etc.)
    objs_animate(g);
    sfx_block_new(0);
    if (saveroom_pos_hero) {
        cs_on_load_enter(g);
    }
    pltf_sync_timestep();
}

void game_update_savefile(g_s *g)
{
    savefile_s *s = g->savefile;
    owl_s      *h = &g->owl;
    {
        mcpy(s->save, g->saveIDs, sizeof(s->save));
        mcpy(s->name, h->name, sizeof(s->name));
        mcpy(s->pins, g->minimap.pins, sizeof(s->pins));
        mcpy(s->enemy_killed, g->enemy_killed, sizeof(s->enemy_killed));
        for (i32 n = 0; n < ARRLEN(s->map_visited); n++) {
            s->map_visited[n] = g->minimap.visited[(n << 1) + 0] |
                                g->minimap.visited[(n << 1) + 1];
        }
        s->tick       = g->tick;
        s->upgrades   = h->upgrades;
        s->n_map_pins = g->minimap.n_pins;
        s->coins      = coins_total(g);
        s->health_max = h->health_max;
    }
    pltf_sync_timestep();
}

bool32 game_save_savefile(g_s *g, v2_i32 pos)
{
    savefile_s *s = g->savefile;
    owl_s      *h = &g->owl;
    {
        game_update_savefile(g);
        str_cpy(s->map_name, g->map_name);
        s->hero_pos.x = pos.x;
        s->hero_pos.y = pos.y;
    }

    err32 res = 0;
    if (!(g->flags & GAME_FLAG_SPEEDRUN)) {
        res = savefile_w(g->save_slot, s);
    }
    pltf_sync_timestep();
    return (res == 0);
}

void game_on_solid_appear_ext(g_s *g, obj_s *s)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_OWL);
    if (ohero) {
        // TODO
        // hero_check_rope_intact(g, ohero);
    }

    if (s && (s->flags & OBJ_FLAG_SOLID)) {
        rec_i32 rs = obj_aabb(s);

        for (obj_each(g, o)) {
            bool32 add_ignored = overlap_rec(obj_aabb(o), rs) &&
                                 (o->flags & OBJ_FLAG_ACTOR) &&
                                 !obj_ignores_solid(o, s, 0);

            if (add_ignored) {
                assert(o->n_ignored_solids < ARRLEN(o->ignored_solids));

                o->ignored_solids[o->n_ignored_solids++] =
                    handle_from_obj(s);
            }
        }
    }

    for (obj_each(g, o)) {
        // obj_try_wiggle(g, o);
    }
}

void game_on_solid_appear(g_s *g)
{
    game_on_solid_appear_ext(g, 0);
}

void game_unlock_map(g_s *g)
{
    saveID_put(g, SAVEID_UNLOCKED_MAP);
#ifdef PLTF_PD
    // pltf_pd_menu_add("Map", game_open_map, g);
#endif
}

void objs_animate(g_s *g)
{
    for (obj_each(g, o)) {
        if (o->on_animate) {
            o->on_animate(g, o);
        }
    }
}

obj_s *obj_find_ID(g_s *g, i32 objID, obj_s *o)
{
    for (obj_s *i = o ? o->next : g->obj_head_busy; i; i = i->next) {
        if (i->ID == objID) {
            return i;
        }
    }
    return 0;
}

i32 game_owl_hitID_next(g_s *g)
{
    g->owl_hitID++;
    if (g->owl_hitID == 0) {
        g->owl_hitID = 1;
    }
    return (i32)g->owl_hitID;
}

void game_cue_area_music(g_s *g)
{
    DEBUG_LOG("Cue area music...\n");
    mus_cue(MUS_CHANNEL_MUSIC, g->music_ID, 500);
#if 0
    switch (g->music_ID) {
    case MUSIC_ID_NONE:
        mus_play_extv(0, 0, 0, 1000, 0, 0);
        break;
    case MUSIC_ID_CAVE:
        mus_play_extv("M_CAVE", 498083, 0, 1000, 0, 256);
        break;
    case MUSIC_ID_WATERFALL:
        mus_play_extv("M_WATERFALL", 226822, 0, 1000, 0, 256);
        break;
    case MUSIC_ID_SNOW:
        mus_play_extv("M_SNOW", 368146, 0, 1000, 0, 256);
        break;
    case MUSIC_ID_FOREST:
        mus_play_extv("M_FOREST", 769768, 0, 1000, 0, 256);
        break;
    case MUSIC_ID_ANCIENT_TREE:
        mus_play_extv("M_ANCIENT_TREE", 564480, 0, 1000, 0, 256);
        break;
    case MUSIC_ID_INTRO:
        mus_play_extv("M_INTRO", 325679, 0, 1000, 0, 256);
        break;
    }
#endif
    DEBUG_LOG("Cue area music done\n");
}

u8 *map_loader_room_mod(g_s *g, u8 *map_name)
{
#if 0
    if (0) {
    } else if (str_eq_nc(map_name, "L_41")) {
        return (u8 *)"L_41_Alt";
    }
#endif
    return map_name;
}

map_room_s *map_room_find(g_s *g, b8 transformed, const void *name)
{
    const void *n = (const void *)(transformed ? map_loader_room_mod(g, (u8 *)name) : name);

    for (i32 i = 0; i < g->n_map_rooms; i++) {
        map_room_s *mr = &g->map_rooms[i];
        if (str_eq_nc(mr->map_name, n)) {
            return mr;
        }
    }
    return 0;
}