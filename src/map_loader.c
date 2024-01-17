// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "game.h"
#include "render/render.h"
#include "util/str.h"

// set tiles automatically - www.cr31.co.uk/stagecast/wang/blob.html

typedef struct {
    u8 type;
    u8 shape;
} map_terraintile_s;

typedef struct {
    int w;
    int h;
    int n_obj;
    int n_prop;
    u32 bytes_prop;
    u32 bytes_tiles_bg;
    u32 bytes_tiles_terrain;
    u32 bytes_obj;
} map_header_s;

typedef struct {
    u8 *tiles;
    int w;
    int h;
} tilelayer_bg_s;

typedef struct {
    map_terraintile_s *tiles;
    int                w;
    int                h;
} tilelayer_terrain_s;

enum {
    MAP_PROP_INT,
    MAP_PROP_FLOAT,
    MAP_PROP_STRING,
    MAP_PROP_OBJ,
    MAP_PROP_BOOL,
    MAP_PROP_POINT,
    MAP_PROP_ARRAY,
};

typedef struct {
    u32  bytes;
    int  type;
    char name[32];
    union {
        i32    n; // array: num elements
        i32    i;
        f32    f;
        u32    b;
        u32    o;
        v2_i16 p;
    } u;
} map_prop_s;

typedef struct {
    int   n;
    void *p;
} map_properties_s;

typedef struct {
    char c[64];
} map_string_s;

#define RESERVED_TERRAIN_POS_Y 224 // in tiles
#define RESERVED_TERRAIN_X     32
#define RESERVED_TERRAIN_Y     32
#define NUM_RESERVED_TERRAIN   (RESERVED_TERRAIN_X * RESERVED_TERRAIN_Y)

enum {
    AUTOTILE_TYPE_NONE,
    AUTOTILE_TYPE_BRICK,
    AUTOTILE_TYPE_BRICK_SMALL,
    AUTOTILE_TYPE_DIRT,
    AUTOTILE_TYPE_STONE,
};

enum {
    TILE_FLIP_DIA = 1 << 0,
    TILE_FLIP_Y   = 1 << 1,
    TILE_FLIP_X   = 1 << 2,
};

static const u8 g_autotilemarch[256];

static void             map_autotile_background(game_s *g, tilelayer_bg_s tiles, int x, int y);
static void             map_autotile_terrain(game_s *g, int *n_ext, tilelayer_terrain_s tiles, int x, int y);
static map_prop_s      *map_prop_get(map_properties_s p, const char *name);
static map_properties_s map_obj_properties(map_obj_s *mo);

#define map_prop_strs(P, NAME, B) map_prop_str(P, NAME, B, sizeof(B))
static void   map_prop_str(map_properties_s p, const char *name, void *b, usize bs);
static i32    map_prop_i32(map_properties_s p, const char *name);
static f32    map_prop_f32(map_properties_s p, const char *name);
static bool32 map_prop_bool(map_properties_s p, const char *name);

static void map_obj_parse(game_s *g, map_obj_s *o)
{
    if (0) {
    } else if (str_eq_nc(o->name, "Switch")) {
        switch_load(g, o);
    } else if (str_eq_nc(o->name, "Heroupgrade")) {
        heroupgrade_load(g, o);
    } else if (str_eq_nc(o->name, "NPC")) {
        npc_load(g, o);
    } else if (str_eq_nc(o->name, "Crawler")) {
        crawler_load(g, o);
    } else if (str_eq_nc(o->name, "Sign")) {
        sign_load(g, o);
    } else if (str_eq_nc(o->name, "Ocean")) {
        int n_waterp = g->tiles_x * 2 + 1;

        watersurface_s *oceanwater = &g->ocean.surf;
        g->ocean.active            = 1;
        g->ocean.y                 = o->y + 16;
        oceanwater->particles      = (waterparticle_s *)marena_alloc(&g->arena, sizeof(waterparticle_s) * n_waterp);
        oceanwater->nparticles     = n_waterp;
        oceanwater->dampening_q12  = 4092;
        oceanwater->fneighbour_q16 = 2000;
        oceanwater->fzero_q16      = 100;
        oceanwater->loops          = 2;
    }
}

void game_load_map(game_s *g, const char *mapfile)
{
    g->obj_ndelete         = 0;
    g->obj_head_busy       = NULL;
    g->obj_head_free       = &g->obj_raw[0];
    g->obj_head_free->next = NULL;

    for (int n = 0; n < NUM_OBJ; n++) {
        obj_s *o         = &g->obj_raw[n];
        o->UID.index     = n;
        o->UID.gen       = 1;
        o->next          = g->obj_head_free;
        g->obj_head_free = o;
    }

    for (int n = 0; n < NUM_OBJ_TAGS; n++) {
        g->obj_tag[n] = NULL;
    }
    g->n_grass      = 0;
    g->n_ropes      = 0;
    g->n_decal_fg   = 0;
    g->n_decal_bg   = 0;
    g->particles.n  = 0;
    g->ocean.active = 0;
    g->env_effects  = 0;
    marena_init(&g->arena, g->mem, sizeof(g->mem));

    strcpy(g->areaname.filename, mapfile);
    g->map_worldroom = map_world_find_room(&g->map_world, mapfile);
    memset(g->tiles, 0, sizeof(g->tiles));
    memset(g->rtiles, 0, sizeof(g->rtiles));
    fade_start(&g->areaname.fade, 30, 200, 100, NULL, NULL, NULL);

    // READ FILE ===============================================================
    map_header_s header = {0};

    FILEPATH_GEN(filepath, FILEPATH_MAP, mapfile);
    str_append(filepath, ".map");
    void *mapf = sys_file_open(filepath, SYS_FILE_R);
    sys_file_read(mapf, &header, sizeof(map_header_s));

    const int w = header.w;
    const int h = header.h;
    g->tiles_x  = w;
    g->tiles_y  = h;
    g->pixel_x  = w << 4;
    g->pixel_y  = h << 4;

    // PROPERTIES ==============================================================
    spm_push();
    map_properties_s mp = {0};
    mp.p                = spm_alloc(header.bytes_prop);
    mp.n                = header.n_prop;
    sys_file_read(mapf, mp.p, header.bytes_prop);
    map_prop_strs(mp, "Name", g->areaname.label);
    prerender_area_label(g);
    if (map_prop_bool(mp, "Effect_Wind"))
        g->env_effects |= ENVEFFECT_WIND;
    if (map_prop_bool(mp, "Effect_Heat"))
        g->env_effects |= ENVEFFECT_HEAT;

    spm_pop();

    // TERRAIN =================================================================
    spm_push();
    tilelayer_terrain_s layer_terrain = {0};

    layer_terrain.w     = w;
    layer_terrain.h     = h;
    layer_terrain.tiles = (map_terraintile_s *)spm_alloc(header.bytes_tiles_terrain);
    sys_file_read(mapf, layer_terrain.tiles, header.bytes_tiles_terrain);

    int n_ext = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int               k  = x + y * w;
            map_terraintile_s tt = layer_terrain.tiles[k];
            switch (tt.shape) {
            case TILE_LADDER: {
                g->tiles[k].collision              = TILE_LADDER;
                g->rtiles[TILELAYER_TERRAIN][k].tx = 464 / 16;
                g->rtiles[TILELAYER_TERRAIN][k].ty = 48 / 16;
            } break;
            case TILE_ONE_WAY: {
                g->tiles[k].collision = TILE_ONE_WAY;

                int    tilex    = 480 / 16;
                bool32 oneway_l = (0 < x + 0 && layer_terrain.tiles[k - 1].shape == TILE_ONE_WAY);
                bool32 oneway_r = (x + 1 < w && layer_terrain.tiles[k + 1].shape == TILE_ONE_WAY);
                if (oneway_l && !oneway_r) tilex++;
                if (oneway_r && !oneway_l) tilex--;
                g->rtiles[TILELAYER_TERRAIN][k].tx = tilex;
            } break;
            default:
                map_autotile_terrain(g, &n_ext, layer_terrain, x, y);
                break;
            }
        }
    }

    spm_pop();

    // BACKGROUND ==============================================================
    spm_push();
    tilelayer_bg_s layer_bg = {0};

    layer_bg.w     = w;
    layer_bg.h     = h;
    layer_bg.tiles = (u8 *)spm_alloc(header.bytes_tiles_bg);
    sys_file_read(mapf, layer_bg.tiles, header.bytes_tiles_bg);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            map_autotile_background(g, layer_bg, x, y);
        }
    }
    spm_pop();

    // OBJECTS =================================================================
    spm_push();
    char *op = (char *)spm_alloc(header.bytes_obj);
    sys_file_read(mapf, op, header.bytes_obj);

    for (int n = 0; n < header.n_obj; n++) {
        map_obj_s *o = (map_obj_s *)op;

        map_obj_parse(g, o);

        op += o->bytes;
    }
    spm_pop();
    sys_file_close(mapf);
}

static int autotile_bg_is(tilelayer_bg_s tiles, int x, int y, int sx, int sy)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || tiles.w <= u || v < 0 || tiles.h <= v) return 1;
    return (0 < tiles.tiles[u + v * tiles.w]);
}

static int autotile_terrain_is(tilelayer_terrain_s tiles, int x, int y, int sx, int sy)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || tiles.w <= u || v < 0 || tiles.h <= v) return 1;

    map_terraintile_s b = tiles.tiles[u + v * tiles.w];
    if (b.type == AUTOTILE_TYPE_NONE) return 0;

    switch (b.shape) {
    case TILE_BLOCK: return 1;
    case TILE_SLOPE_45_0: return (sy == -1 || sx == -1);
    case TILE_SLOPE_45_1: return (sy == +1 || sx == -1);
    case TILE_SLOPE_45_2: return (sy == -1 || sx == +1);
    case TILE_SLOPE_45_3: return (sy == +1 || sx == +1);
    case TILE_SLOPE_LO_0: return (sy == -1 && sx <= 0);
    case TILE_SLOPE_LO_1: return (sy == -1 && sx >= 0);
    case TILE_SLOPE_LO_2: return (sy == +1 && sx <= 0);
    case TILE_SLOPE_LO_3: return (sy == +1 && sx >= 0);
    case TILE_SLOPE_LO_4: return (sx == -1 && sy <= 0);
    case TILE_SLOPE_LO_5: return (sx == +1 && sy <= 0);
    case TILE_SLOPE_LO_6: return (sx == -1 && sy >= 0);
    case TILE_SLOPE_LO_7: return (sx == +1 && sy >= 0);
    case TILE_SLOPE_HI_0: return (sy <= 0 && (sx <= 0));
    case TILE_SLOPE_HI_1: return (sy <= 0 && (sx >= 0));
    case TILE_SLOPE_HI_2: return (sy >= 0 && (sx <= 0));
    case TILE_SLOPE_HI_3: return (sy >= 0 && (sx >= 0));
    case TILE_SLOPE_HI_4: return (sx <= 0 && (sy <= 0));
    case TILE_SLOPE_HI_5: return (sx >= 0 && (sy <= 0));
    case TILE_SLOPE_HI_6: return (sx <= 0 && (sy >= 0));
    case TILE_SLOPE_HI_7: return (sx >= 0 && (sy >= 0));
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
    AT_NW = B8(10000000),
};

static void map_autotile_background(game_s *g, tilelayer_bg_s tiles, int x, int y)
{
    int index = x + y * tiles.w;
    int tile  = tiles.tiles[index];
    if (tile == 0) return;

    flags32 march = 0;
    if (autotile_bg_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_bg_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_bg_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_bg_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_bg_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_bg_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_bg_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_bg_is(tiles, x, y, -1, -1)) march |= AT_NW;

    int      coords = g_autotilemarch[march];
    rtile_s *rtile  = &g->rtiles[TILELAYER_BG][index];
    rtile->tx       = coords & 15;
    rtile->ty       = ((tile - 1) * 8) + (coords >> 4);
}

int terrain_mapper_entry(tilelayer_terrain_s tiles, int x, int y)
{
    if (!(0 <= x && x < tiles.w && 0 <= y && y < tiles.h)) return 0;
    map_terraintile_s tile = tiles.tiles[x + y * tiles.w];
    if (tile.shape != TILE_BLOCK) return 0;
    return tile.type;
}

static void map_autotile_terrain(game_s *g, int *n_ext, tilelayer_terrain_s tiles, int x, int y)
{
    int               index = x + y * tiles.w;
    map_terraintile_s tile  = tiles.tiles[index];
    rtile_s          *rtile = &g->rtiles[TILELAYER_TERRAIN][index];

#if 0
    if (tile.type == 0 && *n_ext < NUM_RESERVED_TERRAIN) {
        return; // disabled
        int ntiles[8] = {0};
        ntiles[0]     = terrain_mapper_entry(tiles, x - 1, y + 0);
        ntiles[1]     = terrain_mapper_entry(tiles, x + 1, y + 0);
        ntiles[2]     = terrain_mapper_entry(tiles, x + 0, y - 1);
        ntiles[3]     = terrain_mapper_entry(tiles, x + 0, y + 1);
        ntiles[4]     = terrain_mapper_entry(tiles, x - 1, y - 1);
        ntiles[5]     = terrain_mapper_entry(tiles, x - 1, y + 1);
        ntiles[6]     = terrain_mapper_entry(tiles, x + 1, y - 1);
        ntiles[7]     = terrain_mapper_entry(tiles, x + 1, y + 1);

        // check if any is nonzero
        int ntilescomb = 0;
        for (int n = 0; n < 8; n++)
            ntilescomb |= ntiles[n];
        if (ntilescomb == 0) return;

        int tID         = *n_ext;
        *n_ext          = *n_ext + 1;
        int       tposx = (tID % RESERVED_TERRAIN_X);
        int       tposy = (tID / RESERVED_TERRAIN_X) + RESERVED_TERRAIN_POS_Y;
        v2_i32    tpos  = {tposx << 4, tposy << 4};
        gfx_ctx_s ctx   = gfx_ctx_default(asset_tex(TEXID_TILESET_TERRAIN));
        texrec_s  trs   = {0};
        trs.t           = ctx.dst;
        trs.r.w         = 8;
        trs.r.h         = 8;
        for (int n = 0; n < 4; n++) {
            if (ntiles[n] == 0) continue;
            trs.r.x      = 416;
            trs.r.y      = (ntiles[n] - 1) * 128;
            v2_i32 ttpos = tpos;

            switch (n) {
            case 0:
                trs.r.w = 8;
                trs.r.h = 16;
                trs.r.y += 16;
                break;
            case 1:
                trs.r.w = 8;
                trs.r.h = 16;
                trs.r.y += 16;
                trs.r.x += 8;
                ttpos.x += 8;
                break;
            case 2:
                trs.r.w = 16;
                trs.r.h = 8;
                trs.r.y += 16;
                trs.r.x += 16;
                break;
            case 3:
                ttpos.y += 8;
                trs.r.w = 16;
                trs.r.h = 8;
                trs.r.y += 16 + 8;
                trs.r.x += 16;
                break;
            }

            gfx_spr(ctx, trs, ttpos, 0, 0);
        }
        rtile->tx = tposx;
        rtile->ty = tposy;
        return;
    }
#endif

    flags32 march = 0;
    if (autotile_terrain_is(tiles, x, y, +0, -1)) march |= AT_N;
    if (autotile_terrain_is(tiles, x, y, +1, +0)) march |= AT_E;
    if (autotile_terrain_is(tiles, x, y, +0, +1)) march |= AT_S;
    if (autotile_terrain_is(tiles, x, y, -1, +0)) march |= AT_W;
    if (autotile_terrain_is(tiles, x, y, +1, -1)) march |= AT_NE;
    if (autotile_terrain_is(tiles, x, y, +1, +1)) march |= AT_SE;
    if (autotile_terrain_is(tiles, x, y, -1, +1)) march |= AT_SW;
    if (autotile_terrain_is(tiles, x, y, -1, -1)) march |= AT_NW;

    int xcoord = 0;
    int ycoord = (tile.type - 1) * 8;
    int coords = g_autotilemarch[march];

    switch (tile.shape) {
    case TILE_BLOCK: {
        switch (march) {
        case 17: { // vertical
        } break;
        case 31: { // left border

        } break;
        case 199: { // bot border

        } break;
        case 241: { // right border

        } break;
        case 68: { // horizontal

        } break;
        case 124: { // top border

        } break;
        case 255: { // mid

        } break;
        }

        xcoord = coords & 15;
        ycoord += coords >> 4;
        g->tiles[index].collision = TILE_BLOCK;

        if (0 < y) {
            map_terraintile_s above = tiles.tiles[x + (y - 1) * tiles.w];
            if (above.type == 0) {
                game_put_grass(g, x, y - 1);
            }
        }

    } break;
    case TILE_SLOPE_45_0:
    case TILE_SLOPE_45_1:
    case TILE_SLOPE_45_2:
    case TILE_SLOPE_45_3: {
        static const int nmasks[4 * 3] = {AT_E, AT_S, AT_SE, // masks for checking neighbours: SHAPE - X | Y | DIAGONAL
                                          AT_E, AT_N, AT_NE,
                                          AT_W, AT_S, AT_SW,
                                          AT_W, AT_N, AT_NW};

        const int *nm = &nmasks[(tile.shape - TILE_SLOPE_45) * 3];
        int        xn = (march & nm[0]) != 0;                      // x neighbour
        int        yn = (march & nm[1]) != 0;                      // y neighbour
        int        cn = (march & nm[2]) != 0;                      // diagonal neighbour
        xcoord        = 8 + (xn && yn && cn ? 4 : xn | (yn << 1)); // slope image index
        ycoord += (tile.shape - TILE_SLOPE_45);

        g->tiles[index].collision = tile.shape;
    } break;
    }

    rtile->tx = xcoord;
    rtile->ty = ycoord;
}

static map_prop_s *map_prop_get(map_properties_s p, const char *name)
{
    if (p.p == NULL) return NULL;
    char *ptr = (char *)p.p;
    for (int n = 0; n < p.n; n++) {
        map_prop_s *prop = (map_prop_s *)ptr;
        if (str_eq_nc(prop->name, name)) {
            return prop;
        }
        ptr += prop->bytes;
    }
    return NULL;
}

static void map_prop_str(map_properties_s p, const char *name, void *b, usize bs)
{
    if (!b || bs == 0) return;
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_STRING) return;
    char *s       = (char *)(prop + 1);
    char *d       = (char *)b;
    usize written = 0;
    while (1) {
        if (bs <= written) break;
        *d = *s;
        if (*s == '\0') break;
        s++;
        d++;
        written++;
    }
    ((char *)b)[bs - 1] = '\0';
}

static i32 map_prop_i32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_INT) return 0;
    return prop->u.i;
}

static f32 map_prop_f32(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_FLOAT) return 0.f;
    return prop->u.f;
}

static bool32 map_prop_bool(map_properties_s p, const char *name)
{
    map_prop_s *prop = map_prop_get(p, name);
    if (prop == NULL || prop->type != MAP_PROP_BOOL) return 0;
    return prop->u.b;
}

static map_properties_s map_obj_properties(map_obj_s *mo)
{
    map_properties_s p = {0};
    p.p                = (void *)(mo + 1);
    p.n                = mo->n_prop;
    return p;
}

void map_obj_str(map_obj_s *mo, const char *name, void *b, usize bs)
{
    map_prop_str(map_obj_properties(mo), name, b, bs);
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

void map_world_load(map_world_s *world, const char *worldfile)
{
    if (!world || !worldfile || worldfile[0] == '\0') return;

    FILEPATH_GEN(filepath, FILEPATH_MAP, worldfile);
    void *f = sys_file_open(filepath, SYS_FILE_R);
    sys_file_read(f, world, sizeof(map_world_s));
    sys_file_close(f);
}

map_worldroom_s *map_world_overlapped_room(map_world_s *world, map_worldroom_s *cur, rec_i32 r)
{
    for (int i = 0; i < world->n_rooms; i++) {
        map_worldroom_s *room = &world->rooms[i];
        if (room == cur) continue;
        rec_i32 rr = {room->x, room->y, room->w, room->h};
        if (overlap_rec(rr, r))
            return room;
    }
    return NULL;
}

map_worldroom_s *map_world_find_room(map_world_s *world, const char *mapfile)
{
    for (int i = 0; i < world->n_rooms; i++) {
        map_worldroom_s *room = &world->rooms[i];
        if (str_eq(room->filename, mapfile))
            return room;
    }
    return NULL;
}

// exported from autotiles_marching.xlsx
// maps marchID -> x + y * 16
static const u8 g_autotilemarch[256] = {
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x27,
    0x57,
    0x27,
    0x57,
    0x02,
    0x12,
    0x02,
    0x65,
    0x27,
    0x57,
    0x27,
    0x57,
    0x03,
    0x21,
    0x03,
    0x13,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x07,
    0x77,
    0x07,
    0x77,
    0x73,
    0x75,
    0x73,
    0x45,
    0x34,
    0x54,
    0x34,
    0x54,
    0x06,
    0x56,
    0x06,
    0x23,
    0x34,
    0x54,
    0x34,
    0x54,
    0x04,
    0x22,
    0x04,
    0x31,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x17,
    0x70,
    0x17,
    0x70,
    0x01,
    0x72,
    0x01,
    0x43,
    0x10,
    0x37,
    0x10,
    0x37,
    0x11,
    0x20,
    0x11,
    0x60,
    0x10,
    0x37,
    0x10,
    0x37,
    0x55,
    0x30,
    0x55,
    0x35,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x27,
    0x46,
    0x27,
    0x46,
    0x02,
    0x44,
    0x02,
    0x62,
    0x27,
    0x46,
    0x27,
    0x46,
    0x03,
    0x33,
    0x03,
    0x25,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x07,
    0x66,
    0x07,
    0x66,
    0x73,
    0x64,
    0x73,
    0x24,
    0x34,
    0x16,
    0x34,
    0x16,
    0x06,
    0x26,
    0x06,
    0x32,
    0x34,
    0x16,
    0x34,
    0x16,
    0x04,
    0x52,
    0x04,
    0x14};