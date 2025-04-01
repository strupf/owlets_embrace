// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "app.h"
#include "core/assets.h"
#include "game.h"
#include "render.h"
#include "util/str.h"

void map_obj_parse(g_s *g, map_obj_s *o)
{
    if (0) {
    } else if (str_eq_nc(o->name, "Coin")) {
        coin_load(g, o);
    } else if (str_eq_nc(o->name, "Solidlever")) {
        solidlever_load(g, o);
    } else if (str_eq_nc(o->name, "Rotor")) {
        rotor_load(g, o);
    } else if (str_eq_nc(o->name, "Savepoint")) {
        savepoint_load(g, o);
    } else if (str_eq_nc(o->name, "Door")) {
        door_load(g, o);
    } else if (str_eq_nc(o->name, "Jumper")) {
        jumper_load(g, o);
    } else if (str_eq_nc(o->name, "Watercol")) {
        watercol_load(g, o);
    } else if (str_eq_nc(o->name, "Trampoline")) {
        trampoline_load(g, o);
    } else if (str_eq_nc(o->name, "Windarea")) {
        windarea_load(g, o);
    } else if (str_eq_nc(o->name, "Waterleaf")) {
        waterleaf_load(g, o);
    } else if (str_eq_nc(o->name, "Chest")) {
        chest_load(g, o);
    } else if (str_eq_nc(o->name, "Fallingblock")) {
        fallingblock_load(g, o);
    } else if (str_eq_nc(o->name, "Fallingstone")) {
        fallingstonespawn_load(g, o);
    } else if (str_eq_nc(o->name, "Light")) {
        light_load(g, o);
    } else if (str_eq_nc(o->name, "Ditherarea")) {
        ditherarea_load(g, o);
    } else if (str_eq_nc(o->name, "Stompfloor")) {
        stompable_block_load(g, o);
    } else if (str_eq_nc(o->name, "Staminarestorer")) {
        staminarestorer_load(g, o);
    } else if (str_eq_nc(o->name, "Flyblob")) {
        flyblob_load(g, o);
    } else if (str_eq_nc(o->name, "Switch")) {
        switch_load(g, o);
    } else if (str_eq_nc(o->name, "Budplant")) {
        budplant_load(g, o);
    } else if (str_eq_nc(o->name, "Steamplatform")) {
        steam_platform_load(g, o);
    } else if (str_eq_nc(o->name, "Hero_Powerup")) {
        hero_powerup_obj_load(g, o);
    } else if (str_eq_nc(o->name, "NPC")) {
        npc_load(g, o);
    } else if (str_eq_nc(o->name, "Crawler")) {
        crawler_load(g, o);
    } else if (str_eq_nc(o->name, "Pushblock")) {
        pushblock_load(g, o);
    } else if (str_eq_nc(o->name, "Toggleblock")) {
        toggleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Moving_Plat")) {
        movingplatform_load(g, o);
    } else if (str_eq_nc(o->name, "Crumbleblock")) {
        crumbleblock_load(g, o);
    } else if (str_eq_nc(o->name, "Teleport")) {
        teleport_load(g, o);
    } else if (str_eq_nc(o->name, "Stalactite")) {
        stalactite_load(g, o);
    } else if (str_eq_nc(o->name, "Flyer")) {
        flyer_load(g, o);
    } else if (str_eq_nc(o->name, "Clockpulse")) {
        clockpulse_load(g, o);
    } else if (str_eq_nc(o->name, "Triggerarea")) {
        triggerarea_load(g, o);
    } else if (str_eq_nc(o->name, "Hooklever")) {
        hooklever_load(g, o);
    } else if (str_eq_nc(o->name, "Cam_Attractor")) {
        camattractor_static_load(g, o);
    } else if (str_eq_nc(o->name, "Cam")) {
        cam_s *cam    = &g->cam;
        cam->locked_x = map_obj_bool(o, "Locked_X");
        cam->locked_y = map_obj_bool(o, "Locked_Y");
        if (cam->locked_x) {
            cam->pos_q8.x = (o->x + PLTF_DISPLAY_W / 2) << 8;
        }
        if (cam->locked_y) {
            cam->pos_q8.y = (o->y + PLTF_DISPLAY_H / 2) << 8;
        }
    } else if (str_eq_nc(o->name, "Fluid")) {
        rec_i32 rfluid = {o->x, o->y, o->w, o->h};
        i32     type   = map_obj_bool(o, "Lava") ? FLUID_AREA_LAVA
                                                 : FLUID_AREA_WATER;
        fluid_area_create(g, rfluid, type, map_obj_bool(o, "Surface"));
    }
}

typedef struct {
    u16 tile;
    u16 x;
    u16 y;
} map_prop_tile_s;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} map_entry_s;

typedef struct map_header_s {
    u32         hash;
    u8          name[32];
    u8          music[8];
    i32         x;
    i32         y;
    u16         w;
    u16         h;
    u16         n_obj;
    u8          n_prop;
    u8          n_entries;
    map_entry_s entries[16];
} map_header_s;

typedef struct {
    u8 *t;
    i32 w;
    i32 h;
} tilelayer_u8;

typedef struct {
    u16 *t;
    i32  w;
    i32  h;
} tilelayer_u16;

static inline i32 map_terrain_pack(i32 type, i32 shape)
{
    return ((type << 8) | shape);
}

static inline i32 map_terrain_type(u16 t)
{
    return (t >> 8);
}

static inline i32 map_terrain_shape(u16 t)
{
    return (t & B16(00000000, 11111111));
}

#define tileID_prop(X, Y) ((X) + (Y) * 64)
#define tileID_deco(X, Y) ((X) + (Y) * 128)

enum {
    MAP_PROP_NULL,
    MAP_PROP_INT,
    MAP_PROP_FLOAT,
    MAP_PROP_STRING,
    MAP_PROP_OBJ,
    MAP_PROP_BOOL,
    MAP_PROP_POINT,
    MAP_PROP_ARRAY
};

typedef struct {
    u16 bytes;
    u16 type;
    u8  name[24];
    union {
        i32 n; // array: num elements
        i32 i;
        f32 f;
        u32 b;
        u32 o;
        u32 p;
    } u;
} map_prop_s;

typedef struct {
    void *p;
    i32   n;
} map_properties_s;

typedef struct {
    u8 c[64];
} map_string_s;

enum {
    TILE_FLIP_DIA = 1 << 0,
    TILE_FLIP_Y   = 1 << 1,
    TILE_FLIP_X   = 1 << 2,
};

extern const v2_i8 g_autotile_coords[256];

static inline void map_proptile_decode(u16 t, i32 *tx, i32 *ty, i32 *f)
{
    *f  = (t >> 14);
    *tx = t & B8(01111111);
    *ty = (t >> 7) & B8(01111111);
}

static void             map_at_background(g_s *g, tilelayer_u8 tiles, i32 x, i32 y);
static void             map_at_terrain(g_s *g, tilelayer_u16 tiles, i32 x, i32 y);
static map_prop_s      *map_prop_get(map_properties_s p, const char *name);
static map_properties_s map_obj_properties(map_obj_s *mo);
//
static bool32           at_types_blending(i32 a, i32 b);

#define map_prop_strs(P, NAME, B) map_prop_str(P, NAME, B, sizeof(B))
static bool32 map_prop_str(map_properties_s p, const char *name, void *b, u32 bs);
static i32    map_prop_i32(map_properties_s p, const char *name);
static f32    map_prop_f32(map_properties_s p, const char *name);
static bool32 map_prop_bool(map_properties_s p, const char *name);
static v2_i16 map_prop_pt(map_properties_s p, const char *name);

void loader_load_terrain(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h);
void loader_load_bgauto(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h);
void loader_load_bg(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h);

void game_load_map(g_s *g, u32 map_hash)
{
    for (obj_each(g, o)) {
        if (o->ID != OBJID_HERO) {
            obj_delete(g, o);
        }
    }
    objs_cull_to_delete(g);

    mclr_static_arr(g->tiles);
    mclr_static_arr(g->rtiles);
    mclr_static_arr(g->fluid_streams);
    mclr_static_arr(g->fluid_areas);
    mclr_field(g->particle_sys);
    mclr_field(g->ghook);
    mclr_field(g->battleroom);

    g->coins.n += g->coins.n_change;
    g->coins.n_change   = 0;
    g->n_grass          = 0;
    g->n_deco_verlet    = 0;
    g->cam.locked_x     = 0;
    g->cam.locked_y     = 0;
    g->cam.has_trg      = 0;
    g->cam.trg_fade_q12 = 0;
    g->n_fluid_areas    = 0;
    g->n_map_doors      = 0;
    g->n_map_pits       = 0;

    // g->areaname.fadeticks = 1;

    marena_reset(&g->memarena, 0);

    // READ FILE ===============================================================

    void     *f;
    wad_el_s *wad_el;
    if (!wad_open(map_hash, &f, &wad_el)) {
        pltf_log("Can't load map file! %u\n", map_hash);
        BAD_PATH
    }

    for (i32 n = 0; n < g->n_map_rooms; n++) {
        map_room_s *mn = &g->map_rooms[n];
        if (mn->hash == map_hash) {
            g->map_room_cur = mn;
            break;
        }
    }
    assert(g->map_room_cur);

    spm_push();
    map_header_s *hd = (map_header_s *)spm_alloc_aligned(wad_el->size, 4);
    pltf_file_r(f, hd, wad_el->size);
    map_properties_s mapp   = {(void *)(hd + 1), hd->n_prop};
    i32              areaID = map_prop_i32(mapp, "Area_ID");
    i32              areafx = map_prop_i32(mapp, "ENV_VFX");

    for (i32 n = 0; n < hd->n_entries; n++) {
        map_door_s  *md = &g->map_doors[g->n_map_doors++];
        map_entry_s *me = &hd->entries[n];
        md->x           = me->x;
        md->y           = me->y;
        md->w           = me->w;
        md->h           = me->h;
    }

    tex_s *tbg = &APP->assets.tex[TEXID_BG_PARALLAX];

    switch (areaID) {
    default: break;
    case 7: tex_from_wad(f, 0, "T_BG001", game_allocator(g), tbg); break; // forest
    case 5: tex_from_wad(f, 0, "T_BG002", game_allocator(g), tbg); break; // case
    case 3: tex_from_wad(f, 0, "T_BG003", game_allocator(g), tbg); break; // mountain
    case AREA_ID_SAVE:
        tex_from_wad(f, 0, "T_BGSAVE01", game_allocator(g), tbg);
        break; // save
    }

    i32 areafxID = 0;

    switch (areafx) {
    default: break;
    case 1: areafxID = AFX_SNOW; break;
    case 2: areafxID = AFX_HEAT; break;
    }
    area_setup(g, &g->area, areaID, areafxID);

    const i32 w = hd->w;
    const i32 h = hd->h;
    g->map_hash = map_hash;
    g->tiles_x  = w;
    g->tiles_y  = h;
    g->pixel_x  = w << 4;
    g->pixel_y  = h << 4;
    assert((w * h) <= NUM_TILES);

    // PROPERTIES ==============================================================
    // mcpy(g->areaname.label, hd.name, 32);
    mcpy(g->mapname, hd->name, 32);
    mcpy(g->musicname, hd->music, 8);

    prerender_area_label(g);
    loader_load_terrain(g, f, wad_el, w, h);
    loader_load_bgauto(g, f, wad_el, w, h);
    loader_load_bg(g, f, wad_el, w, h);
    wad_rd_str(f, wad_el, "FLUIDS", g->fluid_streams);

    byte *objmem = (byte *)wad_rd_spm_str(f, wad_el, "OBJS");
    {
        // check and initialize battle room
        battleroom_s *br              = &g->battleroom;
        byte         *obj_ptr         = objmem;
        usize         size_br_enemies = 0;

        for (i32 n = 0; n < hd->n_obj; n++) {
            map_obj_s *o = (map_obj_s *)obj_ptr;
            if (map_obj_bool(o, "Battleroom")) {
                // preview how many battleroom enemies there are
                br->n_enemies++;
                size_br_enemies += o->bytes;
            }
            if (str_eq_nc(o->name, "Battleroom")) {
                i32 br_saveID = map_obj_i32(o, "saveID");
                if (save_event_exists(g, br_saveID)) {
                    pltf_log("Battleroom already done\n");
                    break;
                }

                pltf_log("Init battleroom\n");
                br->state  = BATTLEROOM_IDLE;
                br->saveID = br_saveID;
                br->r.x    = o->x;
                br->r.y    = o->y;
                br->r.w    = o->w;
                br->r.h    = o->h;

                if (map_obj_has_nonnull_prop(o, "campos")) {
                    br->has_camfocus = 1;
                    v2_i16 campos    = map_obj_pt(o, "campos");
                    br->camfocus.x   = campos.x * 16;
                    br->camfocus.y   = campos.y * 16;
                }
            }
            obj_ptr += o->bytes;
        }

        if (br->state) {
            // copy battleroom enemy data
            obj_ptr         = objmem; // reset obj pointer
            br->mem_enemies = game_alloc(g, size_br_enemies, 4);
            byte *brmem     = (byte *)br->mem_enemies;
            for (i32 n = 0; n < hd->n_obj; n++) {
                map_obj_s *o = (map_obj_s *)obj_ptr;
                if (map_obj_bool(o, "Battleroom")) {
                    mcpy(brmem, o, o->bytes);
                    brmem += o->bytes;
                }
                obj_ptr += o->bytes;
            }
        }
    }
    {
        byte *obj_ptr = objmem;
        for (i32 n = 0; n < hd->n_obj; n++) {
            map_obj_s *o = (map_obj_s *)obj_ptr;
            if (!map_obj_bool(o, "Battleroom")) {
                map_obj_parse(g, o);
            }
            obj_ptr += o->bytes;
        }
    }

    spm_pop();
    pltf_sync_timestep();
}

void loader_load_terrain(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h)
{
    spm_push();
    u16 *tmem = (u16 *)wad_rd_spm_str(f, wad_el, "TERRAIN");

    tilelayer_u16 layer = {tmem, w, h};

    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            i32 k       = x + y * w;
            i32 tt      = tmem[k];
            i32 ttshape = map_terrain_shape(tt);

            switch (ttshape) {
            case TILE_CLIMBWALL: {
                g->tiles[k].collision           = TILE_CLIMBWALL;
                g->rtiles[TILELAYER_PROP_BG][k] = tileID_prop(7, 21);
                break;
            }
            case TILE_LADDER: {
                i32 ty = 19 + (y & 3);
                if (0 < y &&
                    map_terrain_shape(tmem[k - w]) != TILE_LADDER) {
                    ty = 18;
                }

                g->tiles[k].collision               = TILE_LADDER;
                g->rtiles[TILELAYER_PROP_BG][k]     = tileID_prop(5, ty);
                g->rtiles[TILELAYER_PROP_BG][k - 1] = tileID_prop(4, ty);
                g->rtiles[TILELAYER_PROP_BG][k + 1] = tileID_prop(6, ty);
                break;
            }
            case TILE_LADDER_ONE_WAY: {
                g->tiles[k].collision           = TILE_LADDER_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k] = tileID_prop(2, 22);
                break;
            }
            case TILE_ONE_WAY: {
                g->tiles[k].collision           = TILE_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k] = tileID_prop(1, 22);
                break;
            }
            default: map_at_terrain(g, layer, x, y); break;
            }
        }
    }

    // fill in bottomless pits
    // pit = no solid tiles at the bottom but no valid door
    i32 pit_started = 0, px1 = -1, pxw = -1;
    for (i32 x = 0; x < w; x++) {
        i32 ttshape = map_terrain_shape(tmem[x + (h - 1) * w]);
        b32 issolid = (TILE_BLOCK <= ttshape && ttshape <= TILE_SLOPE_45_3);

        for (i32 n = 0; n < g->n_map_doors; n++) {
            map_door_s *md = &g->map_doors[n];
            if (md->y == (h - 1) && md->x <= x && x < (md->x + md->w)) {
                issolid |= 1;
                break;
            }
        }

        if (pit_started) {
            if (issolid || x == (w - 1)) {
                pit_started   = 0;
                map_pit_s *mp = &g->map_pits[g->n_map_pits++];
                mp->x         = px1;
                mp->w         = pxw;
            } else {
                pxw++;
            }
        } else if (!issolid) {
            pit_started = 1;
            px1         = x;
            pxw         = 1;
        }
    }
    spm_pop();
}

void loader_load_bgauto(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h)
{
    spm_push();
    u8          *tmem  = (u8 *)wad_rd_spm_str(f, wad_el, "BGAUTO");
    tilelayer_u8 layer = {tmem, w, h};

    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            map_at_background(g, layer, x, y);
        }
    }
    spm_pop();
}

void loader_load_bg(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h)
{
    spm_push();
    u16 *tmem = (u16 *)wad_rd_spm_str(f, wad_el, "BGTILES");

    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            i32 i = x + y * w;
            i32 t = tmem[i];
            if (!t) continue;

            i32 tx, ty, fl;
            map_proptile_decode(t, &tx, &ty, &fl);
            g->rtiles[TILELAYER_BG_TILE][i] = tileID_deco(tx, ty);
        }
    }
    spm_pop();
}

static bool32 autotile_bg_is(tilelayer_u8 tiles, i32 x, i32 y, i32 sx, i32 sy)
{
    i32 u = x + sx;
    i32 v = y + sy;
    if (!(0 <= u && u < tiles.w && 0 <= v && v < tiles.h)) return 1;
    return (0 < tiles.t[u + v * tiles.w]);
}

static bool32 at_type_dark(i32 t)
{
    return (TILE_TYPE_DARK_BEG <= t && t <= TILE_TYPE_DARK_END);
}

static bool32 at_type_bright(i32 t)
{
    return (TILE_TYPE_BRIGHT_BEG <= t && t <= TILE_TYPE_BRIGHT_END);
}

// if tile type a connects to a neighbour tiletype b
static bool32 at_types_blending(i32 a, i32 b)
{
    if (a == b) return 1;
    if (b == TILE_TYPE_INVISIBLE_NON_CONNECTING) return 0;
    if (b == TILE_TYPE_INVISIBLE_CONNECTING) return 1;
    if (a == TILE_TYPE_THORNS) return 1;
    if (at_type_dark(a) && at_type_dark(b)) return 1;
    if (at_type_bright(a) && at_type_bright(b)) return 1;

    return 0;
}

static bool32 autotile_terrain_is(tilelayer_u16 tiles, i32 x, i32 y, i32 sx, i32 sy)
{
    i32 u = x + sx;
    i32 v = y + sy;
    if (!(0 <= u && u < tiles.w && 0 <= v && v < tiles.h)) return 1;

    i32 a = tiles.t[x + y * tiles.w];
    i32 b = tiles.t[u + v * tiles.w];
    if (!at_types_blending(map_terrain_type(a), map_terrain_type(b))) return 0;

    switch (map_terrain_shape(b)) {
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0: return ((sx == -1 && sy == +0) ||
                                  (sx == +0 && sy == -1) ||
                                  (sx == -1 && sy == -1));
    case TILE_SLOPE_45_1: return ((sx == -1 && sy == +0) ||
                                  (sx == +0 && sy == +1) ||
                                  (sx == -1 && sy == +1));
    case TILE_SLOPE_45_2: return ((sx == +1 && sy == +0) ||
                                  (sx == +0 && sy == -1) ||
                                  (sx == +1 && sy == -1));
    case TILE_SLOPE_45_3: return ((sx == +1 && sy == +0) ||
                                  (sx == +0 && sy == +1) ||
                                  (sx == +1 && sy == +1));
    default: break;
    }
    return 0;
}

// bits for marching squares (neighbours)
// 128  1 2
//  64 XX 4
//  32 16 8
enum {
    AT_N  = B8(00000001),
    AT_E  = B8(00000100),
    AT_S  = B8(00010000),
    AT_W  = B8(01000000),
    AT_NE = B8(00000010),
    AT_SE = B8(00001000),
    AT_SW = B8(00100000),
    AT_NW = B8(10000000)
};

static void map_at_background(g_s *g, tilelayer_u8 tiles, i32 x, i32 y)
{
    i32 i    = x + y * tiles.w;
    i32 tile = tiles.t[i];
    if (tile == 0) return;

    u32 march = 0;
    if (autotile_bg_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_bg_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_bg_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_bg_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_bg_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_bg_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_bg_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_bg_is(tiles, x, y, -1, -1)) march |= AT_NW;

    v2_i8 coords = g_autotile_coords[march];
    i32   tileID = (i32)coords.x + ((i32)coords.y + tile * 8) * 8;

    g->rtiles[TILELAYER_BG][i] = tileID;
}

static u32 map_marching_squares(tilelayer_u16 tiles, i32 x, i32 y)
{
    if (!(0 <= x && x < tiles.w && 0 <= y && y < tiles.h)) return 0xFF;
    u32 tile = tiles.t[x + y * tiles.w];
    if (map_terrain_type(tile) < 3) return 0;

    u32 march = 0;
    if (autotile_terrain_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_terrain_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_terrain_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_terrain_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_terrain_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_terrain_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_terrain_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_terrain_is(tiles, x, y, -1, -1)) march |= AT_NW;
    return march;
}

static bool32 map_dual_border(tilelayer_u16 tiles, i32 x, i32 y,
                              i32 sx, i32 sy,
                              i32 type, u32 march)
{
    // tile types without dual tiles
    switch (map_terrain_type(tiles.t[x + y * tiles.w])) {
    case TILE_TYPE_DARK_STONE: break;
    default: return 0;
    }

    i32 u = x + sx;
    i32 v = y + sy;
    if (!(0 <= u && u < tiles.w && 0 <= v && v < tiles.h)) return 0;
    i32 t = tiles.t[u + v * tiles.w];
    if (map_terrain_type(t) == 6) return 0;

    u32 seed = ((x | u) + ((y | v)));
    u32 r    = rngs_u32(&seed);
    if (r < 0x80000000U) return 0;

    if (t != map_terrain_pack(type, TILE_BLOCK)) return 0;
    return (march == map_marching_squares(tiles, u, v));
}

static void map_at_terrain(g_s *g, tilelayer_u16 tiles, i32 x, i32 y)
{
    i32     index  = x + y * tiles.w;
    i32     tile   = tiles.t[index];
    tile_s *rtile  = &g->tiles[index];
    i32     ttype  = map_terrain_type(tile);
    i32     tshape = map_terrain_shape(tile);

    switch (ttype) {
    case TILE_TYPE_INVISIBLE_NON_CONNECTING:
    case TILE_TYPE_INVISIBLE_CONNECTING: rtile->collision = tshape;
    case 0: return;
    default: break;
    }

    rtile->type = ttype;

    switch (ttype) {
    case TILE_TYPE_THORNS:
        rtile->collision = TILE_SPIKES;
        break;
    default:
        rtile->collision = tshape;
        break;
    }

    u32    m      = map_marching_squares(tiles, x, y);
    v2_i32 tcoord = {0, ((i32)ttype - 2) << 3};
    v2_i8  coords = g_autotile_coords[m];

    switch (tshape) {
    case TILE_BLOCK: {
        i32 n_vari = 1; // number of variations in that tileset

        switch (ttype) {
        case TILE_TYPE_DARK_STONE:
        case TILE_TYPE_DARK_LEAVES:
        case TILE_TYPE_BRIGHT_STONE:
        case TILE_TYPE_BRIGHT_SNOW:
        case TILE_TYPE_THORNS: n_vari = 3; break;
        default: n_vari = 1; break;
        }

        i32 vari = rngr_i32(0, n_vari - 1);
        switch (m) { // coordinates of variation tiles
        case 17: {   // vertical
            static const v2_i8 altc_17[3] = {{7, 3}, {7, 4}, {7, 6}};
            coords                        = altc_17[vari];
            break;
        }
        case 31: { // left border
            static const v2_i8 altc_31[3] = {{5, 3}, {0, 4}, {0, 5}};

            if (0) {
            } else if ((y & 1) == 0 && map_dual_border(tiles, x, y, 0, -1, ttype, m)) {
                coords.x = 9, coords.y = 5;
            } else if ((y & 1) == 1 && map_dual_border(tiles, x, y, 0, +1, ttype, m)) {
                coords.x = 8, coords.y = 5;
            } else {
                coords = altc_31[vari];
            }
            break;
        }
        case 199: { // bot border
            static const v2_i8 altc199[3] = {{4, 2}, {1, 6}, {3, 6}};

            if (0) {
            } else if ((x & 1) == 0 && map_dual_border(tiles, x, y, -1, 0, ttype, m)) {
                coords.x = 9, coords.y = 7;
            } else if ((x & 1) == 1 && map_dual_border(tiles, x, y, +1, 0, ttype, m)) {
                coords.x = 8, coords.y = 7;
            } else {
                coords = altc199[vari];
            }
            break;
        }
        case 241: { // right border
            static const v2_i8 altc241[3] = {{6, 1}, {6, 3}, {2, 4}};

            if (0) {
            } else if ((y & 1) == 0 && map_dual_border(tiles, x, y, 0, -1, ttype, m)) {
                coords.x = 9, coords.y = 6;
            } else if ((y & 1) == 1 && map_dual_border(tiles, x, y, 0, +1, ttype, m)) {
                coords.x = 8, coords.y = 6;
            } else {
                coords = altc241[vari];
            }
            break;
        }
        case 68: { // horizontal
            static const v2_i8 altc_68[3] = {{3, 7}, {4, 7}, {6, 7}};
            coords                        = altc_68[vari];
            break;
        }
        case 124: { // top border
            static const v2_i8 altc124[3] = {{4, 0}, {5, 0}, {3, 5}};

            if (0) {
            } else if ((x & 1) == 0 && map_dual_border(tiles, x, y, -1, 0, ttype, m)) {
                coords.x = 9, coords.y = 4;
            } else if ((x & 1) == 1 && map_dual_border(tiles, x, y, +1, 0, ttype, m)) {
                coords.x = 8, coords.y = 4;
            } else {
                coords = altc124[vari];
            }
            break;
        }
        case 255: { // mid
            static const v2_i8 altc255[3] = {{4, 1}, {1, 4}, {5, 1}};
            coords                        = altc255[0];
            break;
        }
        }

        tcoord.x += coords.x;
        tcoord.y += coords.y;

        if (0 < y && (ttype == 3) &&
            map_terrain_type(tiles.t[x + (y - 1) * tiles.w]) == 0) {
            grass_put(g, x, y - 1);
        }
        break;
    }
    case TILE_SLOPE_45_0:
    case TILE_SLOPE_45_1:
    case TILE_SLOPE_45_2:
    case TILE_SLOPE_45_3: {
        // Y row of tile
        static const u8 shapei[4]    = {0, 2, 1, 3};
        // masks for checking neighbours: SHAPE - X | Y | DIAGONAL
        static const u8 nmasks[4][3] = {{AT_E, AT_S, AT_SE},
                                        {AT_E, AT_N, AT_NE},
                                        {AT_W, AT_S, AT_SW},
                                        {AT_W, AT_N, AT_NW}};

        i32 i  = tshape - TILE_SLOPE_45_0;
        i32 xn = (m & nmasks[i][0]) != 0; // neighbour x
        i32 yn = (m & nmasks[i][1]) != 0; // neighbour y
        i32 cn = (m & nmasks[i][2]) != 0; // neighbour dia

        tcoord.y += shapei[i]; // index shape
        tcoord.x += 8;
        if (xn && yn && cn) { // index variant
            tcoord.x += 3;
        } else if (xn && yn) {
            tcoord.x += 2;
        } else if (yn) {
            tcoord.x += 1;
        }

        break;
    }
    }
    rtile->ty = (i32)tcoord.x + (i32)tcoord.y * 12; // new tile layout
}

static map_prop_s *map_prop_get(map_properties_s p, const char *name)
{
    if (!p.p) return 0;

    byte *ptr = (byte *)p.p;
    for (i32 n = 0; n < p.n; n++) {
        map_prop_s *prop = (map_prop_s *)ptr;
        if (str_eq_nc(prop->name, name)) {
            return prop;
        }
        ptr += prop->bytes;
    }
    // pltf_log("No property: %s\n", name);
    return 0;
}

static bool32 map_prop_str(map_properties_s p, const char *name, void *b, u32 bs)
{
    if (!b || bs == 0) return 0;
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_STRING) return 0;
    char *s       = (char *)(prop + 1);
    char *d       = (char *)b;
    i32   written = 0;
    while (written < (i32)bs) {
        *d = *s;
        if (*s == '\0') break;
        s++;
        d++;
        written++;
    }
    ((char *)b)[bs - 1] = '\0';
    return 1;
}

static i32 map_prop_i32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_INT) return 0;
    return prop->u.i;
}

static f32 map_prop_f32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_FLOAT) return 0.f;
    return prop->u.f;
}

static bool32 map_prop_bool(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_BOOL) return 0;
    return prop->u.b;
}

static v2_i16 map_prop_pt(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (!prop || prop->type != MAP_PROP_POINT) return CINIT(v2_i16){0};
    v2_i16 pt = {(i16)(prop->u.p & 0xFFFFU), (i16)(prop->u.p >> 16)};
    return pt;
}

static map_properties_s map_obj_properties(map_obj_s *mo)
{
    map_properties_s p = {mo + 1, mo->n_prop};
    return p;
}

bool32 map_obj_has_nonnull_prop(map_obj_s *mo, const char *name)
{
    map_prop_s *prop = map_prop_get(map_obj_properties(mo), name);
    if (!prop) return 0;
    return (prop->type != MAP_PROP_NULL);
}

bool32 map_obj_str(map_obj_s *mo, const char *name, void *b, u32 bs)
{
    return map_prop_str(map_obj_properties(mo), name, b, bs);
}

i32 map_obj_i32(map_obj_s *mo, const char *name)
{
    return map_prop_i32(map_obj_properties(mo), name);
}

f32 map_obj_f32(map_obj_s *mo, const char *name)
{
    return map_prop_f32(map_obj_properties(mo), name);
}

bool32 map_obj_bool(map_obj_s *mo, const char *name)
{
    return map_prop_bool(map_obj_properties(mo), name);
}

v2_i16 map_obj_pt(map_obj_s *mo, const char *name)
{
    return map_prop_pt(map_obj_properties(mo), name);
}

void *map_obj_arr(map_obj_s *mo, const char *name, i32 *num)
{
    map_prop_s *prop = map_prop_get(map_obj_properties(mo), name);
    if (!prop || prop->type != MAP_PROP_ARRAY) return 0;
    *num = prop->u.n;
    return (prop + 1);
}

#if 1
const v2_i8 g_autotile_coords[256] = {
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {7, 2},
    {7, 5},
    {7, 2},
    {7, 5},
    {2, 0},
    {2, 1},
    {2, 0},
    {5, 6},
    {7, 2},
    {7, 5},
    {7, 2},
    {7, 5},
    {3, 0},
    {1, 2},
    {3, 0},
    {3, 1},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {7, 0},
    {7, 7},
    {7, 0},
    {7, 7},
    {3, 7},
    {5, 7},
    {3, 7},
    {5, 4},
    {4, 3},
    {4, 5},
    {4, 3},
    {4, 5},
    {6, 0},
    {6, 5},
    {6, 0},
    {3, 2},
    {4, 3},
    {4, 5},
    {4, 3},
    {4, 5},
    {4, 0},
    {2, 2},
    {4, 0},
    {1, 3},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 0},
    {0, 7},
    {0, 0},
    {0, 7},
    {1, 0},
    {2, 7},
    {1, 0},
    {3, 4},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {1, 1},
    {0, 2},
    {1, 1},
    {0, 6},
    {0, 1},
    {7, 3},
    {0, 1},
    {7, 3},
    {5, 5},
    {0, 3},
    {5, 5},
    {5, 3},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {7, 2},
    {6, 4},
    {7, 2},
    {6, 4},
    {2, 0},
    {4, 4},
    {2, 0},
    {2, 6},
    {7, 2},
    {6, 4},
    {7, 2},
    {6, 4},
    {3, 0},
    {3, 3},
    {3, 0},
    {5, 2},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {7, 0},
    {6, 6},
    {7, 0},
    {6, 6},
    {3, 7},
    {4, 6},
    {3, 7},
    {4, 2},
    {4, 3},
    {6, 1},
    {4, 3},
    {6, 1},
    {6, 0},
    {6, 2},
    {6, 0},
    {2, 3},
    {4, 3},
    {6, 1},
    {4, 3},
    {6, 1},
    {4, 0},
    {2, 5},
    {4, 0},
    {4, 1}};
#else
// clang-format off
static const v2_i8 g_autotile_coords[256] = {
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {7, 2}, {7, 5}, {7, 2}, {7, 5}, {2, 0}, {2, 1}, {2, 0}, {5, 6},
    {7, 2}, {7, 5}, {7, 2}, {7, 5}, {3, 0}, {1, 2}, {3, 0}, {3, 1},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {7, 0}, {7, 7}, {7, 0}, {7, 7}, {3, 7}, {5, 7}, {3, 7}, {5, 4},
    {4, 3}, {4, 5}, {4, 3}, {4, 5}, {6, 0}, {6, 5}, {6, 0}, {3, 2},
    {4, 3}, {4, 5}, {4, 3}, {4, 5}, {4, 0}, {2, 2}, {4, 0}, {1, 3},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {7, 1}, {0, 7}, {7, 1}, {0, 7}, {1, 0}, {2, 7}, {1, 0}, {3, 4},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {1, 1}, {0, 2}, {1, 1}, {0, 6},
    {0, 1}, {7, 3}, {0, 1}, {7, 3}, {5, 5}, {0, 3}, {5, 5}, {5, 3},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {7, 2}, {6, 4}, {7, 2}, {6, 4}, {2, 0}, {4, 4}, {2, 0}, {2, 6},
    {7, 2}, {6, 4}, {7, 2}, {6, 4}, {3, 0}, {3, 3}, {3, 0}, {5, 2},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {7, 0}, {6, 6}, {7, 0}, {6, 6}, {3, 7}, {4, 6}, {3, 7}, {4, 2},
    {4, 3}, {6, 1}, {4, 3}, {6, 1}, {6, 0}, {6, 2}, {6, 0}, {2, 3},
    {4, 3}, {6, 1}, {4, 3}, {6, 1}, {4, 0}, {2, 5}, {4, 0}, {4, 1}};
// clang-format on
#endif