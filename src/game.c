// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "app.h"
#include "render.h"

void game_tick_gameplay(g_s *g);

void game_init(g_s *g)
{
    g->savefile      = &APP->save;
    g->obj_head_free = &g->obj_raw[0];
    for (i32 n = 0; n < NUM_OBJ; n++) {
        obj_s *o = &g->obj_raw[n];
        if (n < NUM_OBJ - 1) {
            o->next = &g->obj_raw[n + 1];
        }
    }

    marena_init(&g->memarena, g->mem, sizeof(g->mem));
    wad_el_s *we      = 0;
    void     *f       = wad_open_str("WORLD", 0, &we);
    i32       n_rooms = 0;
    pltf_file_r(f, &n_rooms, sizeof(i32));

    assert(n_rooms <= GAME_N_ROOMS);
    g->n_map_rooms = 0;

    for (i32 k = 0; k < n_rooms; k++) {
        map_room_wad_s mrw = {0};
        pltf_file_r(f, &mrw, sizeof(map_room_wad_s));

        map_room_s mr = {0};
        mr.hash       = mrw.hash;
        mr.x          = mrw.x + 2050; // tiles, (32768 / 16) aligned to multiple of 25 (tiles per display width)
        mr.y          = mrw.y + 2055; // tiles, (32768 / 16) aligned to multiple of 15 (tiles per display height);
        mr.w          = mrw.w;
        mr.h          = mrw.h;
        mr.t          = tex_create(mr.w, mr.h, 0, app_allocator(), 0);
        pltf_file_r(f, mr.t.px, sizeof(u32) * mr.t.wword * mr.t.h);
        g->map_rooms[g->n_map_rooms++] = mr;
    }
    pltf_file_close(f);
}

void game_tick(g_s *g, inp_state_s inpstate)
{
    g->inp.p = g->inp.c;
    g->inp.c = inpstate;

#if PLTF_DEV_ENV
    i32 ab = -1;
#if 0
    if (pltf_sdl_jkey(SDL_SCANCODE_X)) {
        g->hero.charms = 1 - g->hero.charms;
    }

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
#endif
    if (0 <= ab) {
        if (8 <= ab) {
            i32 dt                   = ab == 8 ? -1 : +1;
            g->hero.stamina_upgrades = max_i32(g->hero.stamina_upgrades + dt, 0);
            g->hero.stamina          = hero_stamina_max(obj_get_hero(g));
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

#endif

    if (g->hurt_lp_tick) {
        g->hurt_lp_tick--;

        // works for now
        switch (g->hurt_lp_tick) {
        case 80 - 3 * 0: aud_set_lowpass(1); break;
        case 80 - 3 * 1: aud_set_lowpass(2); break;
        case 80 - 3 * 2: aud_set_lowpass(3); break;
        case 80 - 3 * 3: aud_set_lowpass(4); break;
        case 80 - 3 * 4: aud_set_lowpass(5); break;
        case 80 - 3 * 5: aud_set_lowpass(6); break;
        case 0 + 6 * 5: aud_set_lowpass(5); break;
        case 0 + 6 * 4: aud_set_lowpass(4); break;
        case 0 + 6 * 3: aud_set_lowpass(3); break;
        case 0 + 6 * 2: aud_set_lowpass(2); break;
        case 0 + 6 * 1: aud_set_lowpass(1); break;
        case 0 + 6 * 0: aud_set_lowpass(0); break;
        }
    }

    bool32 tick_gp = 1;
    if (0 < g->freeze_tick) {
        g->freeze_tick--;
        tick_gp = 0;
    }

    if (g->dialog.state) {
        dialog_update(g);
    }

    if (g->cuts.on_update) {
        g->cuts.tick++;
        g->cuts.on_update(g, &g->cuts);
    }

    if (g->block_update) {
        tick_gp = 0;
    }

    if (g->minimap.state) {
        tick_gp = 0;
        minimap_update(g);
    }

    g->tick_animation++;
    if (tick_gp) {
        g->tick++;
        game_tick_gameplay(g);
    }

    game_anim(g);
}

void game_tick_gameplay(g_s *g)
{
    g->n_hitbox_tmp = 0;
    g->events_frame = 0;

    boss_update(g);
    battleroom_on_update(g);

    obj_s *ohero = obj_get_hero(g);
    if (ohero && !(ohero->flags & OBJ_FLAG_DONT_UPDATE)) {
        hero_s *h = (hero_s *)&g->hero;

        bool32 control_hero = !g->block_hero_control && !h->squish;
        inp_s  hinp         = {0};
        if (control_hero) {
            hinp = g->inp;
        }
        hero_on_update(g, ohero, hinp);
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

                if (enemy->explode_on_death) {
                    switch (htick) {
                    case 7: {
                        snd_play(SNDID_ENEMY_EXPLO, 1.f, rngr_f32(0.9f, 1.1f));
                        break;
                    }
                    case 4: {
                        objanim_create(g, pos, OBJANIMID_ENEMY_EXPLODE);
                        break;
                    }
                    }
                }

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
            obj_s *i = obj_from_obj_handle(o->ignored_solids[n]);

            if (i && (i->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(i)))
                continue;
            o->ignored_solids[n] = o->ignored_solids[--o->n_ignored_solids];
        }
    }

    objs_cull_to_delete(g);
    if (hero_present_and_alive(g, &ohero)) {
        inp_s heroinp = inp_cur();
        hero_post_update(g, ohero, heroinp);

        if (ohero->health == 0) {
            cs_gameover_enter(g);
        }
    }
    objs_cull_to_delete(g);

    if (g->events_frame & EVENT_HERO_DAMAGE) {
        g->freeze_tick  = max_i32(g->freeze_tick, 8);
        g->hurt_lp_tick = HERO_HURT_LP_TICKS;
    }
    if (g->events_frame & EVENT_HERO_DEATH) {
        g->freeze_tick  = max_i32(g->freeze_tick, 25);
        g->hurt_lp_tick = HERO_HURT_LP_TICKS;
    }
    if (g->events_frame & EVENT_HIT_ENEMY) {
        g->freeze_tick = max_i32(g->freeze_tick, 2);
    }
}

void game_anim(g_s *g)
{
    cam_update(g, &g->cam);
    g->cam_center    = cam_pos_px_center(g, &g->cam);
    g->darken_bg_q12 = clamp_i32(g->darken_bg_q12 + g->darken_bg_add, 0, 4096);

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

    switch (g->area.fx_type) {
    case AFX_SNOW: areafx_snow_update(g, &g->area.fx.snow); break;
    case AFX_HEAT: areafx_heat_update(g, &g->area.fx.heat); break;
    }

    // every other tick to save some CPU cycles;
    // split between even and uneven frames
    if (g->tick & 1) {
        grass_animate(g);
    } else {
        deco_verlet_animate(g);
    }

    rec_i32 camr = {g->cam_center.x - CAM_WH - 32,
                    g->cam_center.y - CAM_HH - 32,
                    CAM_W + 64,
                    CAM_H + 64};

    for (i32 n = 0; n < g->n_fluid_areas; n++) {
        fluid_area_s *fa = &g->fluid_areas[n];
        rec_i32       rf = {fa->x, fa->y, fa->w, 16};

        // only update if a recent impact happened or it is in view
        if (fa->ticks_to_idle || overlap_rec(camr, rf)) {
            fluid_area_update(&g->fluid_areas[n]);
        }
    }

    particle_sys_update(g);
    area_update(g, &g->area);
    coins_update(g);
    objs_animate(g);
    hero_animate_ui(g);
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

void *game_alloc(g_s *g, usize s, usize alignment)
{
    void *mem = marena_alloc_aligned(&g->memarena, s, alignment);
    mclr(mem, s);
    return mem;
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
    savefile_s *s = g->savefile;
    if (s->tick == 0) { // new game

    } else { // continue
    }

    obj_delete(g, obj_get_hero(g));
    mclr(&g->hero, sizeof(hero_s));
    objs_cull_to_delete(g);

    obj_s  *o = hero_create(g);
    hero_s *h = (hero_s *)o->heap;
    {
        mcpy(g->save_events, s->save, sizeof(g->save_events));
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
        h->stamina_upgrades = s->stamina;
        h->stamina_pieces   = s->stamina_pieces;
    }

    game_load_map(g, s->map_hash);
    o->pos.x = s->hero_pos.x - o->w / 2;
    o->pos.y = s->hero_pos.y - o->h;

    if (save_event_exists(g, SAVE_EV_COMPANION_FOUND)) {
        companion_spawn(g, o);
    }
    cam_init_level(g, &g->cam);
    aud_allow_playing_new_snd(0); // disable sounds (foot steps etc.)
    objs_animate(g);
    aud_allow_playing_new_snd(1);
    pltf_sync_timestep();
}

void game_update_savefile(g_s *g)
{
    savefile_s *s = g->savefile;
    hero_s     *h = &g->hero;
    {
        mcpy(s->save, g->save_events, sizeof(s->save));
        mcpy(s->name, h->name, sizeof(s->name));
        mcpy(s->pins, g->minimap.pins, sizeof(s->pins));
        for (i32 n = 0; n < ARRLEN(s->map_visited); n++) {
            s->map_visited[n] = g->minimap.visited[(n << 1) + 0] |
                                g->minimap.visited[(n << 1) + 1];
        }
        s->tick           = g->tick;
        s->upgrades       = h->upgrades;
        s->n_map_pins     = g->minimap.n_pins;
        s->coins          = coins_total(g);
        s->stamina        = h->stamina_upgrades;
        s->stamina_pieces = h->stamina_pieces;
    }
    pltf_sync_timestep();
}

bool32 game_save_savefile(g_s *g, v2_i32 pos)
{
    savefile_s *s = g->savefile;
    hero_s     *h = &g->hero;
    {
        game_update_savefile(g);
        s->map_hash   = g->map_hash;
        s->hero_pos.x = pos.x;
        s->hero_pos.y = pos.y;
    }

    err32 res = 0;
    if (!g->speedrun) {
        res = savefile_w(g->save_slot, s);
    }
    pltf_sync_timestep();
    return (res == 0);
}

void game_on_trigger(g_s *g, i32 trigger)
{
    if (!trigger) return;

    if (g->cuts.on_trigger) {
        g->cuts.on_trigger(g, &g->cuts, trigger);
    }

    switch (trigger) {
    case TRIGGER_BOSS_PLANT:
        boss_plant_wake_up(g);
        break;
    case 9000:
        cs_demo_1_enter(g);
        break;
    case 9001:
        cs_demo_2_enter(g);
        break;
    case 9005:
        cs_demo_3_enter(g);
        break;
    case TRIGGER_CS_UPGRADE:
        cs_powerup_enter(g);
        break;
    }

    pltf_log("trigger %i\n", trigger);

    for (obj_each(g, o)) {
        if (o->on_trigger) {
            o->on_trigger(g, o, trigger);
        }
    }
}

void game_on_solid_appear_ext(g_s *g, obj_s *s)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        hero_check_rope_intact(g, ohero);
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
                    obj_handle_from_obj(s);
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

bool32 hero_attackbox_o(g_s *g, obj_s *o, hitbox_s box);

bool32 hero_attackboxes(g_s *g, hitbox_s *boxes, i32 nb)
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
            res |= hero_attackbox_o(g, o, boxes[strongest]);
        }
    }
    return res;
}

bool32 hero_attackbox(g_s *g, hitbox_s box)
{
    return hero_attackboxes(g, &box, 1);
}

bool32 hero_attackbox_o(g_s *g, obj_s *o, hitbox_s box)
{
    switch (o->ID) {
    default: break;
    case OBJID_SWITCH: {
        switch_on_interact(g, o);
        break;
    }
    case OBJID_CRUMBLEBLOCK: {
        if (box.flags & HITBOX_FLAG_POWERSTOMP) {
            crumbleblock_break(g, o);
        }
        break;
    }
    case OBJID_STOMPABLE_BLOCK: {
        if (box.flags & HITBOX_FLAG_POWERSTOMP) {
            stompable_block_break(g, o);
        }
        break;
    }
    case OBJID_GEMPILE: {
        if (o->substate != box.hitID) {
            gempile_on_hit(g, o);
            o->substate = box.hitID;
            return 1;
        }
        break;
    }
    }

    bool32 do_hit = (o->flags & OBJ_FLAG_ENEMY) &&
                    0 < o->health &&
                    (!box.hitID || box.hitID != o->enemy.hero_hitID);

    if (do_hit) {
        g->freeze_tick      = max_i32(g->freeze_tick, 3);
        o->enemy.hero_hitID = box.hitID;
        enemy_hurt(g, o, box.damage);
        return 1;
    }
    return 0;
}

void hitbox_tmp_cir(g_s *g, i32 x, i32 y, i32 r)
{
    hitbox_tmp_s h                   = {0};
    h.type                           = HITBOX_TMP_CIR;
    h.x                              = x;
    h.y                              = y;
    h.cir_r                          = r;
    g->hitbox_tmp[g->n_hitbox_tmp++] = h;
}

void game_unlock_map(g_s *g)
{
    save_event_register(g, SAVE_EV_UNLOCKED_MAP);
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

i32 game_hero_hitID_next(g_s *g)
{
    g->hero_hitID++;
    if (g->hero_hitID == 0) {
        g->hero_hitID = 1;
    }
    return (i32)g->hero_hitID;
}

void game_darken_bg(g_s *g, i32 speed)
{
    g->darken_bg_add = speed;
}

void game_cue_area_music(g_s *g)
{
    switch (g->musicID) {
    case 0: mus_play_extv(0, 0, 0, 1000, 0, 0); break;
    case 1: mus_play_extv("M_CAVE", 498083, 0, 1000, 0, 256); break;
    case 2: mus_play_extv("M_WATERFALL", 226822, 0, 1000, 0, 256); break;
    }
}

bool32 snd_cam_param(g_s *g, f32 vol_max, v2_i32 pos, i32 r,
                     f32 *vol, f32 *pan)
{
    if (vol) {
        i32 l = max_i32(r - v2_i32_distance_appr(g->cam_center, pos), 0);
        *vol  = vol_max * (f32)l / (f32)r;
    }
    return 1;
}
