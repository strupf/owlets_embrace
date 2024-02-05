// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render/render.h"

void game_init(game_s *g)
{
    aud_mute(0);
    sys_set_volume(0.5f);

    g->cam.mode = CAM_MODE_FOLLOW_HERO;

    map_world_load(&g->map_world, "world.world");
#if 0
    for (int i = 0; i < 1024; i++) {
        if ((i & 7) == 0) sys_printf("\n");
        int r = (int)(cosf(M_PI * 2.f * (f32)i / 1024.f) * 64.5f);
        if (r < 0) {
            sys_printf("-");
        } else {
            sys_printf("+");
        }

        sys_printf("0x%02X, ", abs_i(r));
    }
#endif
}

static void gameplay_tick(game_s *g)
{
    g->events_frame                     = 0;
    g->herodata.itemselection_decoupled = 0;
    g->herodata.interactable            = obj_handle_from_obj(NULL);
    hero_crank_item_selection(&g->herodata);

    for (obj_each(g, o)) {
        const v2_i32 posprev = o->pos;

        if (0 < o->invincible_tick) {
            o->invincible_tick--;
        }
        if (o->enemy.invincible) {
            o->enemy.invincible--;
        }

        obj_game_update(g, o);
        o->posprev = posprev;
#ifdef SYS_DEBUG
        assert(o->magic == OBJ_MAGIC);
        assert((o->flags & (OBJ_FLAG_PLATFORM | OBJ_FLAG_SOLID)) !=
               (OBJ_FLAG_PLATFORM | OBJ_FLAG_SOLID));
#endif
    }

    for (obj_each(g, o)) { // integrate acc, vel and drag: adds tomove accumulator
        if (o->flags & OBJ_FLAG_MOVER) {
            obj_apply_movement(o);
        }
    }

    for (obj_each(g, o)) { // move objects by tomove
        if (!(o->flags & OBJ_FLAG_SOLID)) continue;
        solid_move(g, o, o->tomove);
        o->tomove.x = 0, o->tomove.y = 0;
    }

    for (obj_each(g, o)) {
        if (!(o->flags & OBJ_FLAG_ACTOR_PLATFORM)) continue;
        if ((o->flags & OBJ_FLAG_ACTOR_PLATFORM) == OBJ_FLAG_PLATFORM) {
            platform_move(g, o, o->tomove);
        } else if (actor_try_wiggle(g, o)) {
            actor_move(g, o, o->tomove);
        }

        o->tomove.x = 0, o->tomove.y = 0;
    }

    const rec_i32 roombounds = {0, 0, g->pixel_x, g->pixel_y};
    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_KILL_OFFSCREEN) &&
            !overlap_rec(obj_aabb(o), roombounds)) {
            obj_delete(g, o);
        }
    }

    objs_cull_to_delete(g);

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    // hero touching other objects
    if (ohero) {
        const rec_i32 heroaabb     = obj_aabb(ohero);
        bool32        herogrounded = obj_grounded(g, ohero);
        for (obj_each(g, o)) {
            switch (o->ID) {
            case OBJ_ID_SHROOMY: {
                if (herogrounded) break;
                rec_i32 rs = obj_aabb(o);
                rec_i32 ri;
                if (!intersect_rec(heroaabb, rs, &ri)) break;
                if (0 < ohero->vel_q8.y && heroaabb.y + heroaabb.h < rs.y + rs.h) {
                    ohero->vel_q8.y = -1500;
                    ohero->tomove.y -= ri.h;
                    shroomy_bounced_on(o);
                }
                break;
            }
            }
        }

        // touched hurting things?
        if (0 < ohero->invincible_tick) goto SKIP_HAZARDS;
        for (obj_each(g, o)) {
            if (!(o->flags & OBJ_FLAG_HURT_ON_TOUCH)) continue;
            if (!overlap_rec(heroaabb, obj_aabb(o))) continue;

            v2_i32 dt       = v2_sub(obj_pos_center(ohero), obj_pos_center(o));
            ohero->vel_q8.x = sgn_i(dt.x) * 1000;
            ohero->vel_q8.y = -1000;
            hero_hurt(g, ohero, &g->herodata, 1);
            g->events_frame |= EVENT_HERO_DAMAGE;
            snd_play_ext(SNDID_SWOOSH, 0.5f, 0.5f);
            break;
        }
    SKIP_HAZARDS:;
    }

    objs_cull_to_delete(g);

    // retrieve again in case the hero died
    ohero = obj_get_tagged(g, OBJ_TAG_HERO);

    collectibles_update(g);

    if (ohero) {
        const rec_i32 heroaabb = obj_aabb(ohero);
        hero_check_rope_intact(g, ohero);

        // collectibles
        for (obj_each(g, o)) {
            if (!(o->flags & OBJ_FLAG_COLLECTIBLE)) continue;
            if (!overlap_rec(heroaabb, obj_aabb(o))) continue;

            switch (o->ID) {
            case OBJ_ID_HEROUPGRADE:
                heroupgrade_on_collect(g, o, &g->herodata);
                break;
            case OBJ_ID_COLLECTIBLE:
                snd_play_ext(SNDID_COIN, 1.f, rngr_f32(0.8f, 1.2f));
                break;
            }

            obj_delete(g, o);
        }
    }

    objs_cull_to_delete(g);

#ifdef SYS_DEBUG
    for (obj_each(g, o)) {
        assert(o->magic == OBJ_MAGIC);
    }
#endif

    transition_check_herodata_slide(&g->transition, g);
    particles_update(g, &g->particles);

    if (g->events_frame & EVENT_HIT_ENEMY) {
        g->freeze_ticks = max_i(g->freeze_ticks, 2);
    }
    if (g->events_frame & EVENT_HERO_DAMAGE) {
        g->freeze_ticks = max_i(g->freeze_ticks, 4);
    }
}

void game_tick(game_s *g)
{
    if (g->mainmenu_fade_in) {
        g->mainmenu_fade_in--;
    }

    if (g->freeze_ticks) {
        g->freeze_ticks--;
        return;
    }

    if (g->die_ticks) {
        g->die_ticks++;
        if (50 <= g->die_ticks) {
            game_load_savefile(g, g->savefile, g->savefile_slotID);
            g->die_ticks = 0;
            return;
        }
    }

    if (g->respawn_ticks) {
        g->respawn_ticks++;
        if (50 <= g->respawn_ticks) {
            g->respawn_ticks = 0;
            return;
        }
    }

    bool32 update_gameplay = 1;
    if (upgradehandler_in_progress(&g->heroupgrade)) {
        upgradehandler_tick(&g->heroupgrade);
        update_gameplay = 0;
    } else if (textbox_active(&g->textbox)) {
        textbox_update(g, &g->textbox);
        update_gameplay = 0;
    } else if (!transition_finished(&g->transition)) {
        transition_update(g, &g->transition);
        update_gameplay = !transition_blocks_gameplay(&g->transition);
    } else if (shop_active(g)) {
        shop_update(g);
        update_gameplay = 0;
    }

    if (update_gameplay) {
        gameplay_tick(g);

        for (obj_each(g, o)) {
            obj_game_animate(g, o);
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

    if (g->env_effects & ENVEFFECT_WIND) {
        enveffect_wind_update(&g->env_wind);
    }

    // every other tick to save some CPU cycles;
    // split between even and uneven frames
    if (sys_tick() & 1) {
        backforeground_animate_grass(g);
    } else {
        if (g->env_effects & ENVEFFECT_HEAT) {
            enveffect_heat_update(&g->env_heat);
        }
        if (g->env_effects & ENVEFFECT_CLOUD) {
            enveffect_cloud_update(&g->env_cloud);
        }
    }
}

void game_open_inventory(game_s *g)
{
}

void game_resume(game_s *g)
{
    g->herodata.itemselection_decoupled = 0;
}

void game_paused(game_s *g)
{
    g->herodata.itemselection_decoupled = 1;
    render_pause(g);
}

void game_new_savefile(game_s *g, int slotID)
{
    game_load_map(g, "map_01");
    g->savefile_slotID = slotID;
    obj_s *oh          = hero_create(g);
    oh->pos.x          = 300;
    oh->pos.y          = 60;
}

void game_write_savefile(game_s *g)
{
    savefile_s  sf   = {0};
    //
    herodata_s *hero = &g->herodata;
    strcpy(sf.area_filename, g->areaname.filename);
    strcpy(sf.hero_name, hero->name);
    sf.aquired_upgrades = hero->aquired_upgrades;
    sf.n_airjumps       = hero->n_airjumps;
    sf.health           = hero->health;
    //
    g->savefile         = sf;
    savefile_write(g->savefile_slotID, &sf);
}

void game_load_savefile(game_s *g, savefile_s sf, int slotID)
{
    g->savefile_slotID = slotID;
    g->savefile        = sf;
    game_load_map(g, sf.area_filename);

    mus_play("assets/mus/background.wav");

    herodata_s *hero       = &g->herodata;
    hero->aquired_upgrades = sf.aquired_upgrades;
    hero->n_airjumps       = sf.n_airjumps;
    hero->health           = sf.health;

#if 1
    obj_s *oh = hero_create(g);
    oh->pos.x = 300;
    oh->pos.y = 200;

    for (obj_each(g, o)) {
        if (o->ID == OBJ_ID_SAVEPOINT) {
            oh->pos.x = o->pos.x;
            oh->pos.y = o->pos.y - 20;
            break;
        }
    }
#endif

    obj_s *oj = juggernaut_create(g);
    oj->pos.x = 200;
    oj->pos.y = 50;

    inventory_add(&g->inventory, INVENTORY_ID_KEY_1, 1);
}

void game_on_trigger(game_s *g, int trigger)
{
    for (obj_each(g, o)) {
        obj_game_trigger(g, o, trigger);
    }
}

void game_put_grass(game_s *g, int tx, int ty)
{
    if (g->n_grass >= ARRLEN(g->grass)) return;
    grass_s *gr = &g->grass[g->n_grass++];
    *gr         = (grass_s){0};
    gr->pos.x   = tx * 16;
    gr->pos.y   = ty * 16;
    gr->type    = rngr_i32(0, 2);
}

static inline i32 ocean_height_logic_q6(i32 p, i32 t)
{
    return (sin_q6((p >> 2) + (t << 1) + 0x00) << 4) +
           (sin_q6((p >> 1) + (t << 1) + 0x80) << 3) +
           (sin_q6((p >> 0) - (t << 2) + 0x40) << 2);
}

int ocean_height(game_s *g, int pixel_x)
{
    i32 h = ocean_height_logic_q6(pixel_x, sys_tick());
    return (h >> 6) + g->ocean.y;
}

int ocean_render_height(game_s *g, int pixel_x)
{
    int p = pixel_x;
    int t = sys_tick();
    i32 y = ocean_height_logic_q6(p, t) +
            (sin_q6((p << 2) + (t << 4) + 0x20) << 1) +
            (sin_q6((p << 4) - (t << 5) + 0x04) << 0) +
            (sin_q6((p << 5) + (t << 6) + 0x10) >> 2);
    i32 h = (y >> 6) + g->ocean.y;
    return h;
}

int water_depth_rec(game_s *g, rec_i32 r)
{
    int f        = 0;
    int y_bottom = r.y + r.h;
    if (g->ocean.active) {
        int h1 = max_i(0, y_bottom - ocean_height(g, r.x));
        int h2 = max_i(0, y_bottom - ocean_height(g, r.x + r.w));
        f      = (h1 + h2) >> 1;
    }

    for (int i = 0; i < g->n_water; i++) {
        water_s *wa = &g->water[i];
        if (!overlap_rec(wa->area, r)) continue;
        f = max_i(f, y_bottom - wa->area.y);
    }
    return f;
}

bool32 tiles_solid(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 riarea;
    if (!intersect_rec(r, rgrid, &riarea)) return 0;

    int px0 = riarea.x;
    int py0 = riarea.y;
    int px1 = riarea.x + riarea.w - 1;
    int py1 = riarea.y + riarea.h - 1;
    int tx0 = px0 >> 4;
    int ty0 = py0 >> 4;
    int tx1 = px1 >> 4;
    int ty1 = py1 >> 4;

    for (int ty = ty0; ty <= ty1; ty++) {
        int y0 = (ty == ty0 ? py0 & 15 : 0); // px in tile (local)
        int y1 = (ty == ty1 ? py1 & 15 : 15);

        for (int tx = tx0; tx <= tx1; tx++) {
            int ID = g->tiles[tx + ty * g->tiles_x].collision;
            if (ID == TILE_EMPTY || NUM_TILE_BLOCKS <= ID) continue;
            if (ID == TILE_BLOCK) return 1;
            int x0 = (tx == tx0 ? px0 & 15 : 0);
            int x1 = (tx == tx1 ? px1 & 15 : 15);
            int mk = (0xFFFF >> x0) & (0xFFFF << (15 - x1));
            for (int py = y0; py <= y1; py++)
                if (g_pxmask_tab[ID * 16 + py] & mk) return 1;
        }
    }
    return 0;
}

bool32 tiles_solid_pt(game_s *g, int x, int y)
{
    if (!(0 <= x && x < g->pixel_x && 0 <= y && y < g->pixel_y)) return 0;

    int ID = g->tiles[(x >> 4) + (y >> 4) * g->tiles_x].collision;
    if (ID == TILE_EMPTY || NUM_TILE_BLOCKS <= ID) return 0;
    return (g_pxmask_tab[(ID << 4) + (y & 15)] & (0x8000 >> (x & 15)));
}

bool32 tile_one_way(game_s *g, rec_i32 r)
{
    rec_i32 rgrid = {0, 0, g->pixel_x, g->pixel_y};
    rec_i32 riarea;
    if (!intersect_rec(r, rgrid, &riarea)) return 0;

    int tx0 = (riarea.x) >> 4;
    int ty0 = (riarea.y) >> 4;
    int tx1 = (riarea.x + riarea.w - 1) >> 4;
    int ty1 = (riarea.y + riarea.h - 1) >> 4;

    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (g->tiles[tx + ty * g->tiles_x].collision == TILE_ONE_WAY)
                return 1;
        }
    }
    return 0;
}

bool32 game_traversable(game_s *g, rec_i32 r)
{
    if (tiles_solid(g, r)) return 0;

    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(o)))
            return 0;
    }
    return 1;
}

bool32 game_traversable_pt(game_s *g, int x, int y)
{
    if (tiles_solid_pt(g, x, y)) return 0;

    v2_i32 p = {x, y};
    for (obj_each(g, o)) {
        if ((o->flags & OBJ_FLAG_SOLID) && overlap_rec_pnt(obj_aabb(o), p))
            return 0;
    }
    return 1;
}

bounds_2D_s game_tilebounds_rec(game_s *g, rec_i32 r)
{
    v2_i32 pmin = {r.x, r.y};
    v2_i32 pmax = {r.x + r.w, r.y + r.h};
    return game_tilebounds_pts(g, pmin, pmax);
}

bounds_2D_s game_tilebounds_pts(game_s *g, v2_i32 p0, v2_i32 p1)
{
    bounds_2D_s b = {
        max_i(p0.x, 0) >> 4, // div 16
        max_i(p0.y, 0) >> 4,
        min_i(p1.x, g->pixel_x - 1) >> 4,
        min_i(p1.y, g->pixel_y - 1) >> 4,
    };
    return b;
}

bounds_2D_s game_tilebounds_tri(game_s *g, tri_i32 t)
{
    v2_i32 pmin = v2_min3(t.p[0], t.p[1], t.p[2]);
    v2_i32 pmax = v2_max3(t.p[0], t.p[1], t.p[2]);
    return game_tilebounds_pts(g, pmin, pmax);
}

int tick_to_index_freq(int tick, int n_frames, int freqticks)
{
    int i = ((tick * n_frames) / freqticks) % n_frames;
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

// operate on 16x16 tiles
// these are the collision masks per pixel row of a tile
const int g_pxmask_tab[32 * 16] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // empty
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // block
    0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF, // SLOPES 45
    0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF, 0x00FF, 0x007F, 0x003F, 0x001F, 0x000F, 0x0007, 0x0003, 0x0001,
    0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF,
    0xFFFF, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF0, 0xFFE0, 0xFFC0, 0xFF80, 0xFF00, 0xFE00, 0xFC00, 0xF800, 0xF000, 0xE000, 0xC000, 0x8000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0003, 0x000F, 0x003F, 0x00FF, 0x03FF, 0x0FFF, 0x3FFF, 0xFFFF, // SLOPES LO
    0xFFFF, 0x3FFF, 0x0FFF, 0x03FF, 0x00FF, 0x003F, 0x000F, 0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xC000, 0xF000, 0xFC00, 0xFF00, 0xFFC0, 0xFFF0, 0xFFFC, 0xFFFF,
    0xFFFF, 0xFFFC, 0xFFF0, 0xFFC0, 0xFF00, 0xFC00, 0xF000, 0xC000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0001, 0x0001, 0x0003, 0x0003, 0x0007, 0x0007, 0x000F, 0x000F, 0x001F, 0x001F, 0x003F, 0x003F, 0x007F, 0x007F, 0x00FF, 0x00FF, // rotated
    0x00FF, 0x00FF, 0x007F, 0x007F, 0x003F, 0x003F, 0x001F, 0x001F, 0x000F, 0x000F, 0x0007, 0x0007, 0x0003, 0x0003, 0x0001, 0x0001,
    0x8000, 0x8000, 0xC000, 0xC000, 0xE000, 0xE000, 0xF000, 0xF000, 0xF800, 0xF800, 0xFC00, 0xFC00, 0xFE00, 0xFE00, 0xFF00, 0xFF00,
    0xFF00, 0xFF00, 0xFE00, 0xFE00, 0xFC00, 0xFC00, 0xF800, 0xF800, 0xF000, 0xF000, 0xE000, 0xE000, 0xC000, 0xC000, 0x8000, 0x8000,
    0x0003, 0x000F, 0x003F, 0x00FF, 0x03FF, 0x0FFF, 0x3FFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, // SLOPES HI
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x3FFF, 0x0FFF, 0x03FF, 0x00FF, 0x003F, 0x000F, 0x0003,
    0xC000, 0xF000, 0xFC00, 0xFF00, 0xFFC0, 0xFFF0, 0xFFFC, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFC, 0xFFF0, 0xFFC0, 0xFF00, 0xFC00, 0xF000, 0xC000,
    0x01FF, 0x01FF, 0x03FF, 0x03FF, 0x07FF, 0x07FF, 0x0FFF, 0x0FFF, 0x1FFF, 0x1FFF, 0x3FFF, 0x3FFF, 0x7FFF, 0x7FFF, 0xFFFF, 0xFFFF, // rotated
    0xFFFF, 0xFFFF, 0x7FFF, 0x7FFF, 0x3FFF, 0x3FFF, 0x1FFF, 0x1FFF, 0x0FFF, 0x0FFF, 0x07FF, 0x07FF, 0x03FF, 0x03FF, 0x01FF, 0x01FF,
    0xFF80, 0xFF80, 0xFFC0, 0xFFC0, 0xFFE0, 0xFFE0, 0xFFF0, 0xFFF0, 0xFFF8, 0xFFF8, 0xFFFC, 0xFFFC, 0xFFFE, 0xFFFE, 0xFFFF, 0xFFFF,
    0xFFFF, 0xFFFF, 0xFFFE, 0xFFFE, 0xFFFC, 0xFFFC, 0xFFF8, 0xFFF8, 0xFFF0, 0xFFF0, 0xFFE0, 0xFFE0, 0xFFC0, 0xFFC0, 0xFF80, 0xFF80,
    //
};

// triangle coordinates
// x0 y0 x1 y1 x2 y2
const i32 tilecolliders[GAME_NUM_TILECOLLIDERS * 6] = {
    // dummy triangles
    0, 0, 0, 0, 0, 0, // empty
    0, 0, 0, 0, 0, 0, // solid
    // slope 45
    0, 16, 16, 16, 16, 0, // 2
    0, 0, 16, 0, 16, 16,  // 3
    0, 0, 0, 16, 16, 16,  // 4
    0, 0, 0, 16, 16, 0,   // 5
    // slope lo
    0, 16, 16, 16, 16, 8, // 6
    0, 0, 16, 0, 16, 8,   // 7
    0, 8, 0, 16, 16, 16,  // 8
    0, 0, 16, 0, 0, 8,    // 9
    16, 0, 16, 16, 8, 16, // 10
    8, 0, 16, 0, 16, 16,  // 11
    0, 0, 0, 16, 8, 16,   // 12
    0, 0, 8, 0, 0, 16,    // 13
    // slopes hi
    0, 16, 16, 16, 16, 8, // 14
    0, 16, 16, 16, 16, 8,
    0, 0, 16, 0, 16, 8, // 15
    0, 0, 16, 0, 16, 8,
    0, 8, 0, 16, 16, 16, // 16
    0, 8, 0, 16, 16, 16,
    0, 0, 16, 0, 0, 8, // 17
    0, 0, 16, 0, 0, 8,
    16, 0, 16, 16, 8, 16, // 18
    16, 0, 16, 16, 8, 16,
    8, 0, 16, 0, 16, 16, // 19
    8, 0, 16, 0, 16, 16,
    0, 0, 0, 16, 8, 16, // 20
    0, 0, 0, 16, 8, 16,
    0, 0, 8, 0, 0, 16, // 21
    0, 0, 8, 0, 0, 16,
    //
};