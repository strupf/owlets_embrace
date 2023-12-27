// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"
#include "render.h"
#include "rope.h"

u16 g_animated_tiles[65536];

static void backforeground_animate_grass(game_s *g);

void game_init(game_s *g)
{
    for (int i = 0; i < ARRLEN(g_animated_tiles); i++) {
        g_animated_tiles[i] = i;
    }

    g->cam.w    = SYS_DISPLAY_W;
    g->cam.h    = SYS_DISPLAY_H;
    g->cam.mode = CAM_MODE_FOLLOW_HERO;

    map_world_load(&g->map_world, "world.world");
}

static void gameplay_tick(game_s *g)
{
    g->herodata.itemselection_decoupled = 0;

    hero_crank_item_selection(&g->herodata);
    if (inp_debug_space()) {
        g->herodata.aquired_upgrades |= 1 << HERO_UPGRADE_HIGH_JUMP;
        g->herodata.aquired_upgrades |= 1 << HERO_UPGRADE_LONG_HOOK;
        g->herodata.itemselection_decoupled = 1;
    } else {
        g->herodata.aquired_upgrades &= ~(1 << HERO_UPGRADE_HIGH_JUMP);
        g->herodata.aquired_upgrades &= ~(1 << HERO_UPGRADE_LONG_HOOK);
    }

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s       *o       = g->obj_busy[i];
        const v2_i32 posprev = o->pos;

        switch (o->ID) {
        case OBJ_ID_BLOB: blob_on_update(g, o); break;
        case OBJ_ID_HERO: hero_on_update(g, o); break;
        case OBJ_ID_CRUMBLEBLOCK: crumbleblock_update(g, o); break;
        case OBJ_ID_CLOCKPULSE: clockpulse_update(g, o); break;
        case OBJ_ID_SHROOMY: shroomy_on_update(g, o); break;
        }
        o->posprev = posprev;
#ifdef SYS_DEBUG
        assert(o->magic == OBJ_MAGIC);
#endif
    }

    for (int i = 0; i < g->obj_nbusy; i++) { // integrate acc, vel and drag: adds tomove accumulator
        obj_s *o = g->obj_busy[i];
        if (o->flags & OBJ_FLAG_MOVER) {
            obj_apply_movement(o);
        }
    }

    for (int i = 0; i < g->obj_nbusy; i++) { // move objects by tomove
        obj_s *o = g->obj_busy[i];
        if (o->flags & OBJ_FLAG_SOLID) {
            solid_move(g, o, o->tomove);
            o->tomove.x = 0;
            o->tomove.y = 0;
        }
    }

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if (!(o->flags & OBJ_FLAG_ACTOR)) continue;
        if (actor_try_wiggle(g, o)) {
            actor_move(g, o, o->tomove);
        }
        o->tomove.x = 0;
        o->tomove.y = 0;
    }

    rec_i32 roombounds = {-256, -256, g->pixel_x + 256, g->pixel_y + 256};
    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if ((o->flags & OBJ_FLAG_KILL_OFFSCREEN) && !overlap_rec(obj_aabb(o), roombounds)) {
            obj_delete(g, o);
        }
    }

    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        hero_check_rope_intact(g, ohero);
    }
    objs_cull_to_delete(g);

#ifdef SYS_DEBUG
    for (int i = 0; i < g->obj_nbusy; i++) {
        assert(g->obj_busy[i]->magic == OBJ_MAGIC);
    }
#endif

    transition_check_hero_slide(&g->transition, g);
    ocean_update(&g->ocean);
    particles_update(g, &g->particles);
}

void game_tick(game_s *g)
{
    g->tick++;

    if (g->textbox.state != TEXTBOX_STATE_INACTIVE) {
        textbox_update(g, &g->textbox);
    } else if (!transition_finished(&g->transition)) {
        transition_update(&g->transition);
    } else {
        gameplay_tick(g);
    }

    cam_update(g, &g->cam);
    fade_update(&g->areaname.fade);
    enveffect_wind_update(&g->env_wind);
    enveffect_heat_update(&g->env_heat);
    backforeground_animate_grass(g);

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];

        switch (o->ID) {
        case OBJ_ID_HERO: hero_on_animate(g, o); break;
        case OBJ_ID_SWITCH: switch_on_animate(g, o); break;
        case OBJ_ID_TOGGLEBLOCK: toggleblock_on_animate(g, o); break;
        case OBJ_ID_SHROOMY: shroomy_on_animate(g, o); break;
        }
    }
}

void game_paused(game_s *g)
{
    render_pause(g);
}

void game_new_savefile(game_s *g, int slotID)
{
    game_load_map(g, "map_01");
    g->savefile_slotID = slotID;
    obj_s *oh          = hero_create(g);
    oh->pos.x          = 60;
    oh->pos.y          = 60;
}

void game_write_savefile(game_s *g)
{
    savefile_s sf = {0};

    hero_s *hero = &g->herodata;
    strcpy(sf.area_filename, g->areaname.filename);
    strcpy(sf.hero_name, hero->name);
    sf.aquired_items    = hero->aquired_items;
    sf.aquired_upgrades = hero->aquired_upgrades;
    sf.tick             = g->tick;
    sf.n_airjumps       = hero->n_airjumps;
    savefile_write(g->savefile_slotID, &sf);
}

void game_load_savefile(game_s *g, savefile_s sf, int slotID)
{
    g->savefile_slotID = slotID;
    game_load_map(g, sf.area_filename);
    g->tick = sf.tick;

    obj_s *oh = hero_create(g);
    oh->pos.x = 50;
    oh->pos.y = 10;

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if (o->ID == OBJ_ID_SAVEPOINT) {
            oh->pos.x = o->pos.x;
            oh->pos.y = o->pos.y - 20;
            break;
        }
    }

    g->herodata.aquired_items    = sf.aquired_items;
    g->herodata.aquired_upgrades = sf.aquired_upgrades;
    g->herodata.n_airjumps       = sf.n_airjumps;

    obj_s *cp        = clockpulse_create(g);
    cp->subtimer     = 100;
    cp->trigger_on_1 = 4;
    for (int n = 0; n < 10; n++) {
        g->tiles[n + 3 + 9 * g->tiles_x].collision = TILE_ONE_WAY;
    }

    obj_s *cs = shroomy_create(g);
    cs->pos.y = 80;
    cs->pos.x = 100;
    /*
    obj_s *cb = blob_create(g);
    cb->pos.x = 100;
    cb->pos.y = 100;
    */
}

static void backforeground_animate_grass(game_s *g)
{
    for (int n = 0; n < g->n_grass; n++) {
        grass_s *gr = &g->grass[n];
        rec_i32  r  = {gr->pos.x, gr->pos.y, 16, 16};

        for (int n = 0; n < g->obj_nbusy; n++) {
            obj_s *o = g->obj_busy[n];
            if ((o->flags & OBJ_FLAG_MOVER) && overlap_rec(r, obj_aabb(o))) {
                gr->v_q8 += o->vel_q8.x >> 6;
            }
        }

#define GRASS_F (30000 * 150)
        int f1 = -((gr->x_q8 * 4) >> 8);
        int f2 = rngr_i32(-GRASS_F, +GRASS_F) >> (13 + 8);
        gr->v_q8 += f1 + f2;
        gr->x_q8 += gr->v_q8;
        gr->x_q8 = clamp_i(gr->x_q8, -256, +256);
        gr->v_q8 = (gr->v_q8 * 245) >> 8;
    }
}

obj_s *obj_closest_interactable(game_s *g, v2_i32 pos)
{
    u32    interactable_dt = pow2_i32(INTERACTABLE_DIST); // max distance
    obj_s *interactable    = NULL;
    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if (o->flags & OBJ_FLAG_INTERACTABLE) {
            u32 d = v2_distancesq(pos, o->pos);
            if (d < interactable_dt) {
                interactable_dt = d;
                interactable    = o;
            }
        }
    }
    return interactable;
}

void game_on_trigger(game_s *g, int trigger)
{
    sys_printf("trigger: %i\n", trigger);
    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if (o->trigger != trigger) continue;

        switch (o->ID) {
        case OBJ_ID_TOGGLEBLOCK: toggleblock_on_trigger(g, o); break;
        }
    }
}

solid_rec_list_s game_solid_recs(game_s *g)
{
    static rec_i32 solidrecs[NUM_OBJ];

    solid_rec_list_s l = {0};
    l.recs             = solidrecs;

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        if (o->flags & OBJ_FLAG_SOLID) {
            rec_i32 r     = obj_aabb(o);
            l.recs[l.n++] = r;
        }
    }
    return l;
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

    for (int n = 0; n < g->obj_nbusy; n++) {
        obj_s *o = g->obj_busy[n];
        if ((o->flags & OBJ_FLAG_SOLID) && overlap_rec(r, obj_aabb(o)))
            return 0;
    }
    return 1;
}

bool32 game_traversable_pt(game_s *g, int x, int y)
{
    if (tiles_solid_pt(g, x, y)) return 0;

    v2_i32 p = {x, y};
    for (int n = 0; n < g->obj_nbusy; n++) {
        obj_s *o = g->obj_busy[n];
        if ((o->flags & OBJ_FLAG_SOLID) && overlap_rec_pnt(obj_aabb(o), p))
            return 0;
    }
    return 1;
}

static void obj_apply_hitboxes(game_s *g, obj_s *o, hitbox_s *boxes, int nb)
{
    rec_i32 aabb = obj_aabb(o);
    if (aabb.w * aabb.h == 0) { // point object
        aabb.w = 1;
        aabb.h = 1;
    }

    for (int n = 0; n < nb; n++) {
        hitbox_s h = boxes[n];

        if (!overlap_rec(h.r, aabb)) continue;

        switch (o->ID) {
        case OBJ_ID_SWITCH:
            if (!(h.flags & HITBOX_FLAG_HERO)) break;
            switch_on_interact(g, o);
            return;
        }
    }
}

void game_apply_hitboxes(game_s *g, hitbox_s *boxes, int n_boxes)
{
    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
        obj_apply_hitboxes(g, o, boxes, n_boxes);
    }
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

int tick_now(game_s *g)
{
    return g->tick;
}

int tick_to_index_freq(int tick, int n_frames, int freqticks)
{
    int i = ((tick * n_frames) / freqticks) % n_frames;
    assert(0 <= i && i < n_frames);
    return i;
}

obj_s *obj_solid_create(game_s *g)
{
    obj_s *o = obj_create(g);
    o->ID    = OBJ_ID_SOLID;
    o->flags |= OBJ_FLAG_SOLID;
    o->w = 32;
    o->h = 24;

    return o;
}

void game_on_solid_appear(game_s *g)
{
    obj_s *ohero = obj_get_tagged(g, OBJ_TAG_HERO);
    if (ohero) {
        hero_check_rope_intact(g, ohero);
    }

    for (int i = 0; i < g->obj_nbusy; i++) {
        obj_s *o = g->obj_busy[i];
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
const tri_i32 tilecolliders[GAME_NUM_TILECOLLIDERS] = {
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