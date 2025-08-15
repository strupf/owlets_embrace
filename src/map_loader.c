// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "app.h"
#include "core/assets.h"
#include "game.h"
#include "render.h"
#include "util/str.h"

void map_obj_load_misc(g_s *g, map_obj_s *mo);
void map_obj_parse(g_s *g, map_obj_s *o);

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} map_entry_s;

typedef struct map_header_s {
    u32         hash;
    i32         x;
    i32         y;
    u16         w;
    u16         h;
    u16         n_fg;
    u8          n_obj;
    u8          n_prop;
    u8          n_entries;
    map_entry_s entries[16];
} map_header_s;

typedef struct {
    i16 x;
    i16 y;
    u16 kwh; // width, height, scrolling factor packed
    u8  tx;  // tile data
    u8  ty;
} map_fg_s;

#define tileID_prop(X, Y) ((X) + (Y) * 64)
#define tileID_deco(X, Y) ((X) + (Y) * 128)

enum {
    MAP_PROP_NULL,
    MAP_PROP_INT,
    MAP_PROP_FLOAT,
    MAP_PROP_STRING,
    MAP_PROP_OBJ,
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

static inline void map_proptile_decode(u16 t, i32 *tx, i32 *ty, i32 *f)
{
    *f  = (t >> 14);
    *tx = t & B8(01111111);
    *ty = (t >> 7) & B8(01111111);
}

static map_prop_s      *map_prop_get(map_properties_s p, const char *name);
static map_properties_s map_obj_properties(map_obj_s *mo);

#define map_prop_strs(P, NAME, B) map_prop_str(P, NAME, B, sizeof(B))
static bool32 map_prop_str(map_properties_s p, const char *name, void *b, u32 bs);
static i32    map_prop_i32(map_properties_s p, const char *name);
static f32    map_prop_f32(map_properties_s p, const char *name);
static bool32 map_prop_bool(map_properties_s p, const char *name);
static v2_i16 map_prop_pt(map_properties_s p, const char *name);

void loader_load_terrain(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h, u32 *seed_visuals);
void loader_load_bgauto(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h);
void loader_load_bg(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h);

void game_load_map(g_s *g, u8 *map_name)
{
    // READ FILE ===============================================================
    u8       *map_name_mod = map_loader_room_mod(g, map_name);
    u32       map_hash     = wad_hash(map_name_mod);
    void     *f;
    wad_el_s *wad_el;
    if (!wad_open(map_hash, &f, &wad_el)) {
        pltf_log("Can't load map file! %u\n", map_hash);
        BAD_PATH
    }

    spm_push();
    map_header_s *hd = (map_header_s *)spm_alloc_aligned(wad_el->size, 4);
    pltf_file_r(f, hd, wad_el->size);
    map_properties_s mapp = {(void *)(hd + 1), hd->n_prop};

    const i32 w = hd->w;
    const i32 h = hd->h;

    u32 seed_visuals = map_hash;
    g->bg_offx       = rngsr_i32(&seed_visuals, 0, I16_MAX);
    g->bg_offy       = rngsr_i32(&seed_visuals, 0, I16_MAX);
    str_cpy(g->map_name, map_name_mod);
    g->tiles_x = w;
    g->tiles_y = h;
    g->pixel_x = w << 4;
    g->pixel_y = h << 4;
    assert((w * h) <= NUM_TILES);

    for (obj_each(g, o)) {
        if (o->ID != OBJID_OWL) {
            obj_delete(g, o);
        }
    }
    objs_cull_to_delete(g);

    mclr_field(g->boss);
    mclr(g->tiles, sizeof(tile_s) * w * h);
    mclr(g->fluid_streams, sizeof(u8) * w * h);
    mclr_static_arr(g->fg_el);
    mclr_static_arr(g->fluid_areas);
    for (i32 n = 0; n < NUM_TILELAYER; n++) {
        mclr(&g->rtiles[n], sizeof(u16) * w * h);
    }

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
    g->darken_bg_q12    = 0;
    g->darken_bg_add    = 0;
    g->n_save_points    = 0;
    g->n_fg             = 0;
    g->tick_animation   = 0;
    g->n_ropes          = 0;
    marena_reset(&g->memarena, 0);

    for (i32 n = 0; n < g->n_map_rooms; n++) {
        map_room_s *mn = &g->map_rooms[n];
        if (wad_hash(mn->map_name) == map_hash) {
            g->map_room_cur = mn;
            break;
        }
    }
    assert(g->map_room_cur);

    // PROPERTIES ==============================================================
    i32 area_ID       = map_prop_i32(mapp, "AREA_ID");
    i32 vfx_ID        = map_prop_i32(mapp, "VFX_ID");
    i32 music_ID      = map_prop_i32(mapp, "MUSIC_ID");
    i32 background_ID = map_prop_i32(mapp, "BACKGROUND_ID");

    for (i32 n = 0; n < hd->n_entries; n++) {
        map_door_s  *md = &g->map_doors[g->n_map_doors++];
        map_entry_s *me = &hd->entries[n];
        md->x           = me->x;
        md->y           = me->y;
        md->w           = me->w;
        md->h           = me->h;
    }

    switch (vfx_ID) {
    default: break;
    case VFX_ID_SNOW:
        vfx_area_snow_setup(g);
        break;
    }

    background_init_and_load_from_wad(g, background_ID, f);

    switch (area_ID) {
    case AREA_ID_DEEP_FOREST:
        str_cpy(g->areaname, "Deep Forest");
        break;
    case AREA_ID_SNOW_PEAKS:
        str_cpy(g->areaname, "Snow Peaks");
        break;
    }

    bool32 same_music = music_ID == g->music_ID;
    g->music_ID       = music_ID;
    g->area_ID        = area_ID;
    g->background_ID  = background_ID;
    g->vfx_ID         = vfx_ID;

    if (!g->previewmode && !same_music) {
        game_cue_area_music(g);
    }

    if (!g->previewmode) {
        g->area_anim_st   = 1;
        g->area_anim_tick = 0;
    }

    seed_visuals = 213;
    loader_load_terrain(g, f, wad_el, w, h, &seed_visuals);
    loader_load_bgauto(g, f, wad_el, w, h);
    loader_load_bg(g, f, wad_el, w, h);
    wad_rd_str(f, wad_el, "FLUIDS", g->fluid_streams);

    map_fg_s *mfg = (map_fg_s *)wad_r_spm_str(f, wad_el, "FOREGROUND");
    for (i32 n = 0; n < hd->n_fg; n++, mfg++) {
        foreground_el_s *fg_el = &g->fg_el[g->n_fg++];
        i32              tw    = B8(01111111) & (mfg->kwh >> 7);
        i32              th    = B8(01111111) & (mfg->kwh);
        i32              kq    = (mfg->kwh >> 14);
        fg_el->x               = mfg->x;
        fg_el->y               = mfg->y;
        fg_el->tx              = (i32)mfg->tx << 4;
        fg_el->ty              = (i32)mfg->ty << 4;
        fg_el->tw              = (i32)(1 + tw) << 4;
        fg_el->th              = (i32)(1 + th) << 4;
        switch (kq) {
        case 0: fg_el->k_q8 = 12; break;
        case 1: fg_el->k_q8 = 24; break;
        case 2: fg_el->k_q8 = 48; break;
        case 3: fg_el->k_q8 = 64; break;
        }
    }

    wad_el_s *e_objs = wad_seek_str(f, wad_el, "OBJS");
    g->map_objs      = game_alloc(g, e_objs->size, 4);
    g->map_objs_end  = (byte *)g->map_objs + e_objs->size;
    pltf_file_r(f, g->map_objs, e_objs->size);

    if (0) {
    } else if (hd->hash == wad_hash("L_BOSSPLANT")) {
        map_obj_s *mo = map_obj_find(g, "Bossplant");
        boss_init(g, BOSS_ID_PLANT, mo);
    }

    for (map_obj_each(g, o)) {
        if (!map_obj_bool(o, "Battleroom")) {
            map_obj_parse(g, o);
        }
    }

    spm_pop();
    pulleyblocks_setup(g);
    drillers_setup(g);
    pltf_sync_timestep();
}

void loader_do_terrain(g_s *g, u16 *tmem, i32 w, i32 h, u32 *seed_visuals)
{
    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            i32 k       = x + y * w;
            i32 tt      = tmem[k];
            i32 ttshape = map_terrain_shape(tt);

            switch (ttshape) {
            case TILE_CLIMBWALL: {
                g->tiles[k].shape               = TILE_CLIMBWALL;
                g->rtiles[TILELAYER_PROP_BG][k] = tileID_prop(7, 21);
                break;
            }
            case TILE_LADDER: {
                i32 ty = 19 + (y & 3);
                if (0 < y &&
                    map_terrain_shape(tmem[k - w]) != TILE_LADDER) {
                    ty = 18;
                }

                g->tiles[k].shape                   = TILE_LADDER;
                g->rtiles[TILELAYER_PROP_BG][k]     = tileID_prop(5, ty);
                g->rtiles[TILELAYER_PROP_BG][k - 1] = tileID_prop(4, ty);
                g->rtiles[TILELAYER_PROP_BG][k + 1] = tileID_prop(6, ty);
                break;
            }
            case TILE_LADDER_ONE_WAY: {
                g->tiles[k].shape               = TILE_LADDER_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k] = tileID_prop(2, 22);
                break;
            }
            case TILE_ONE_WAY: {
                g->tiles[k].shape               = TILE_ONE_WAY;
                g->rtiles[TILELAYER_PROP_BG][k] = tileID_prop(1, 22);
                break;
            }
            default: {
                i32 ttype         = map_terrain_type(tt);
                g->tiles[k].type  = ttype;
                g->tiles[k].shape = ttshape;
                break;
            }
            }
        }
    }
}

void loader_load_terrain(g_s *g, void *f, wad_el_s *wad_el, i32 w, i32 h, u32 *seed_visuals)
{
    spm_push();
    u16 *tmem = (u16 *)wad_rd_spm_str(f, wad_el, "TERRAIN");
    loader_do_terrain(g, tmem, w, h, seed_visuals);
    autotile_terrain(g->tiles, w, h, 0, 0);

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
    u8 *tmem = (u8 *)wad_rd_spm_str(f, wad_el, "BGAUTO");
    autotilebg(g, tmem);
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
    return map_prop_i32(p, name);
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

map_obj_ref_s *map_obj_ref(map_obj_s *mo, const char *name)
{
    map_prop_s *prop = map_prop_get(map_obj_properties(mo), name);
    if (!prop || prop->type != MAP_PROP_OBJ) return 0;
    return (map_obj_ref_s *)(prop + 1);
}

map_obj_s *map_obj_find(g_s *g, const char *name)
{
    for (map_obj_each(g, o)) {
        if (str_eq_nc(o->name, name)) {
            return o;
        }
    }
    return 0;
}

void obj_place_to_map_obj(obj_s *o, map_obj_s *mo, i32 a_x, i32 a_y)
{
    switch (a_x) {
    default:
    case +0: o->pos.x = mo->x + (mo->w - o->w) / 2; break;
    case -1: o->pos.x = mo->x; break;
    case +1: o->pos.x = mo->x + mo->w - o->w; break;
    }
    switch (a_y) {
    default:
    case +0: o->pos.y = mo->y + (mo->h - o->h) / 2; break;
    case -1: o->pos.y = mo->y; break;
    case +1: o->pos.y = mo->y + mo->h - o->h; break;
    }
}