// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "map_loader.h"
#include "game.h"
#include "util/str.h"

// set tiles automatically - www.cr31.co.uk/stagecast/wang/blob.html

typedef struct {
    int  fgid;
    char source[64];
} tm_tileset_s;

typedef struct {
    u16 ID;
    u8  flip;
    u8  tileset;
} tm_tile_s;

typedef struct {
    tm_tileset_s sets[8];
    int          n_sets;
    tm_tile_s   *t;
    int          w;
    int          h;
} tm_tilelayer_s;

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

static void load_obj(game_s *g, json_s jobj);
static void load_autotiles(game_s *g, tm_tilelayer_s tl);

static bool32 tmj_property(json_s j, const char *prop, json_s *jout)
{
    for (json_each (j, "properties", it)) {
        char       *s = jsonk_strp(it, "name", NULL);
        const char *c = prop;
        while (1) {
            if (*c == '\0') {
                return (json_key(it, "value", jout) == JSON_SUCCESS ? 1 : 0);
            }
            if (*c != *s) break;
            s++;
            c++;
        }
    }
    return 0;
}

static i32 tmj_property_i32(json_s j, const char *prop)
{
    json_s p;
    return (tmj_property(j, prop, &p) ? json_i32(p) : 0);
}

static char *tmj_property_str(json_s j, const char *prop, char *buf, usize bufsize)
{
    json_s p;
    return (tmj_property(j, prop, &p) ? json_str(p, buf, bufsize) : NULL);
}

#define tmj_property_strs(J, PROP, BUF) tmj_property_str(J, PROP, BUF, sizeof(BUF))

void map_world_load(map_world_s *world, const char *worldfile)
{
    if (!world || !worldfile || worldfile[0] == '\0') return;

    spm_push();

    char filepath[64];
    str_cpy(filepath, FILEPATH_WORLD);
    str_append(filepath, worldfile);

    char *txt;
    if (txt_load(filepath, spm_alloc, &txt) != TXT_SUCCESS) {
        sys_printf("couldn't load txt %s\n", worldfile);
        spm_pop();
        BAD_PATH
        return;
    }

    json_s jroot;
    if (json_root(txt, &jroot) != JSON_SUCCESS) {
        sys_printf("couldn't find json root %s\n", worldfile);
        spm_pop();
        BAD_PATH
        return;
    }

    for (json_each (jroot, "maps", jmap)) {
        map_room_s *r = &world->rooms[world->n_rooms++];
        char        buf[64];
        jsonk_strs(jmap, "fileName", buf);

        int i = str_len(buf) - 1; // split off subfolder folder/map_01 -> map_01
        for (; i >= 0; i--) {
            if (buf[i] == '/')
                break;
        }

        str_cpy(r->filename, &buf[i + 1]);
        r->r.x = jsonk_i32(jmap, "x");
        r->r.y = jsonk_i32(jmap, "y");
        r->r.w = jsonk_u32(jmap, "width");
        r->r.h = jsonk_u32(jmap, "height");
    }

    spm_pop();
}

map_room_s *map_world_overlapped_room(map_world_s *world, rec_i32 r)
{
    for (int i = 0; i < world->n_rooms; i++) {
        map_room_s *room = &world->rooms[i];
        if (room == world->roomcur) continue;
        if (overlap_rec(room->r, r))
            return room;
    }
    return NULL;
}

map_room_s *map_world_find_room(map_world_s *world, const char *mapfile)
{
    for (int i = 0; i < world->n_rooms; i++) {
        map_room_s *room = &world->rooms[i];
        if (str_eq(room->filename, mapfile))
            return room;
    }
    return NULL;
}

void game_load_map(game_s *g, const char *mapfile)
{
    g->obj_ndelete = 0;
    g->obj_nbusy   = 0;
    g->obj_nfree   = NUM_OBJ;
    for (int n = 0; n < NUM_OBJ; n++) {
        obj_generic_s *o = &g->obj_raw[n];
        o->o.index       = n;
        o->o.gen         = 1;
#ifdef SYS_DEBUG
        o->magic = OBJ_GENERIC_MAGIC;
#endif
        g->obj_free_stack[n] = (obj_s *)o;
    }

    for (int n = 0; n < NUM_OBJ_TAGS; n++) {
        g->obj_tag[n] = NULL;
    }
    g->n_grass = 0;
    g->n_ropes = 0;
    marena_init(&g->arena, g->mem, sizeof(g->mem));

    g->ocean.ocean.particles      = (waterparticle_s *)marena_alloc(&g->arena, sizeof(waterparticle_s) * 1024);
    g->ocean.ocean.nparticles     = 1024;
    g->ocean.ocean.dampening_q12  = 4070;
    g->ocean.ocean.fneighbour_q16 = 8000;
    g->ocean.ocean.fzero_q16      = 20;
    g->ocean.ocean.loops          = 1;
    g->ocean.y                    = 300;

    g->ocean.water.particles      = (waterparticle_s *)marena_alloc(&g->arena, sizeof(waterparticle_s) * 4096);
    g->ocean.water.nparticles     = 4096;
    g->ocean.water.dampening_q12  = 4060;
    g->ocean.water.fneighbour_q16 = 2000;
    g->ocean.water.fzero_q16      = 100;
    g->ocean.water.loops          = 3;
    g->ocean.water.p              = (v2_i32){0};

    strcpy(g->area_filename, mapfile);

    spm_push();

    char filepath[64];
    str_cpy(filepath, FILEPATH_MAP);
    str_append(filepath, mapfile);

    char *txt;
    if (txt_load(filepath, spm_alloc, &txt) != TXT_SUCCESS) {
        sys_printf("+++ couldn't load txt %s\n", mapfile);
        BAD_PATH
        spm_pop();
        return;
    }

    json_s jroot;
    if (json_root(txt, &jroot) != JSON_SUCCESS) {
        sys_printf("+++ couldn't find json root %s\n", mapfile);
        BAD_PATH
        spm_pop();
        return;
    }

    g->map_world.roomcur = map_world_find_room(&g->map_world, mapfile);

    int w         = jsonk_u32(jroot, "width");
    int h         = jsonk_u32(jroot, "height");
    int num_tiles = w * h;
    assert(num_tiles <= NUM_TILES);

    memset(g->tiles, 0, sizeof(g->tiles));
    memset(g->rtiles, 0, sizeof(g->rtiles));

    g->area_name_ticks = AREA_NAME_TICKS;
    g->tiles_x         = w;
    g->tiles_y         = h;
    g->pixel_x         = w << 4;
    g->pixel_y         = h << 4;

    tmj_property_strs(jroot, "name", g->area_name);

    char musicfile[64];
    tmj_property_strs(jroot, "music", musicfile);
    mus_fade_to(musicfile, 60, 60);

    tm_tilelayer_s tl = {0};
    tl.w              = w;
    tl.h              = h;

    for (json_each (jroot, "tilesets", jtileset)) {
        tm_tileset_s *ts = &tl.sets[tl.n_sets++];
        ts->fgid         = jsonk_u32(jtileset, "firstgid");
        jsonk_strs(jtileset, "source", ts->source);
    }

    for (json_each (jroot, "layers", jlayer)) {
        spm_push();

        char name[16] = {0};
        jsonk_strs(jlayer, "name", name);
        char *type = jsonk_strp(jlayer, "type", NULL);

        switch (*type) {
        case 'i': {

        } break;
        case 't': {
            tl.t = (tm_tile_s *)spm_alloc(sizeof(tm_tile_s) * num_tiles);

            int i = 0;
            for (json_each (jlayer, "data", jtile)) {
                tm_tile_s tile   = {0};
                u32       tileID = json_u32(jtile);

                if (tileID != 0) {
                    u32 ID      = tileID & 0xFFFFFFFU;
                    int tileset = -1;
                    for (int n = tl.n_sets - 1; n >= 0; n--) {
                        tm_tileset_s *ts = &tl.sets[n];
                        if (ts->fgid <= ID) {
                            tileset = n;
                            ID -= ts->fgid;
                            break;
                        }
                    }

                    tile.ID      = ID;
                    tile.flip    = tileID >> 29;
                    tile.tileset = tileset;
                }

                tl.t[i++] = tile;
            }

            if (str_eq(name, "tile_0")) {
                load_autotiles(g, tl);
            }

        } break;
        case 'o': {
            for (json_each (jlayer, "objects", jobj)) {
                load_obj(g, jobj);
            }
        } break;
        }

        spm_pop();
    }
    spm_pop();
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

static int type_from_tiled(tm_tile_s b)
{
    return b.ID / 8;
}

static int shape_from_tiled(tm_tile_s b)
{
    switch (b.ID % 8) {
    case 0: return TILE_BLOCK; // block
    case 1:                    // slope 45
        switch (b.flip) {
        case 0:
        case 1: return TILE_SLOPE_45_0;
        case 2:
        case 3: return TILE_SLOPE_45_1;
        case 4:
        case 5: return TILE_SLOPE_45_2;
        case 6:
        case 7: return TILE_SLOPE_45_3;
        }
        return 0;
    case 2: // slope lo
        switch (b.flip) {
        case 0: return TILE_SLOPE_LO_0;
        case 1: return TILE_SLOPE_LO_1;
        case 2: return TILE_SLOPE_LO_2;
        case 3: return TILE_SLOPE_LO_3;
        case 4: return TILE_SLOPE_LO_4;
        case 5: return TILE_SLOPE_LO_5;
        case 6: return TILE_SLOPE_LO_6;
        case 7: return TILE_SLOPE_LO_7;
        }
        return 0;
    case 3: // slope hi
        switch (b.flip) {
        case 0: return TILE_SLOPE_HI_0;
        case 1: return TILE_SLOPE_HI_1;
        case 2: return TILE_SLOPE_HI_2;
        case 3: return TILE_SLOPE_HI_3;
        case 4: return TILE_SLOPE_HI_4;
        case 5: return TILE_SLOPE_HI_5;
        case 6: return TILE_SLOPE_HI_6;
        case 7: return TILE_SLOPE_HI_7;
        }
        return 0;
    }
    return 0;
}

// returns true if tile looked at from x/y in direction sx/sy is "solid"
// x/y: tile currently looked at
// sx/sy direction we are looking in
// ai: current tile ID
// af: current tile flipping
static int autotile_is(tm_tilelayer_s t, int x, int y, int sx, int sy, tm_tile_s a)
{
    int u = x + sx;
    int v = y + sy;
    if (u < 0 || t.w <= u || v < 0 || t.h <= v) return 1;

    tm_tile_s b     = t.t[u + v * t.w];
    int       btype = type_from_tiled(b);
    if (btype == AUTOTILE_TYPE_NONE) return 0;

    switch (shape_from_tiled(b)) {
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

static flags32 autotile_march(tm_tilelayer_s t, int x, int y, tm_tile_s a)
{
    flags32 m = 0;
    if (autotile_is(t, x, y, +0, -1, a)) m |= AT_N;
    if (autotile_is(t, x, y, +1, +0, a)) m |= AT_E;
    if (autotile_is(t, x, y, +0, +1, a)) m |= AT_S;
    if (autotile_is(t, x, y, -1, +0, a)) m |= AT_W;
    if (autotile_is(t, x, y, +1, -1, a)) m |= AT_NE;
    if (autotile_is(t, x, y, +1, +1, a)) m |= AT_SE;
    if (autotile_is(t, x, y, -1, +1, a)) m |= AT_SW;
    if (autotile_is(t, x, y, -1, -1, a)) m |= AT_NW;
    return m;
}

static void load_obj(game_s *g, json_s jobj)
{
    obj_s *o = NULL;
    char   name[64];
    jsonk_strs(jobj, "name", name);

    if (0) {
    } else if (str_eq(name, "Sign")) {
        o        = obj_create(g);
        o->ID    = OBJ_ID_SIGN;
        o->pos.x = jsonk_i32(jobj, "x");
        o->pos.y = jsonk_i32(jobj, "y");
        o->w     = jsonk_u32(jobj, "width");
        o->h     = jsonk_u32(jobj, "height");
        o->flags |= OBJ_FLAG_INTERACTABLE;
        tmj_property_strs(jobj, "dialog", o->filename);
    }
}

static void load_autotiles(game_s *g, tm_tilelayer_s tl)
{
    u32 rngseed = 213;

    for (int y = 0; y < tl.h; y++) {
        for (int x = 0; x < tl.w; x++) {
            int       n     = x + y * tl.w;
            tm_tile_s t     = tl.t[n];
            int       ttype = type_from_tiled(t);

            if (ttype == AUTOTILE_TYPE_NONE) {
                g->tiles[n].collision = TILE_EMPTY;
                continue;
            }

            int tshape = shape_from_tiled(t);

            flags32 m      = autotile_march(tl, x, y, t);
            int     xcoord = 0;
            int     ycoord = (ttype - 1) * 8;
            int     coords = g_autotilemarch[m];

            switch (tshape) {
            case TILE_BLOCK: {
                switch (m) {
                case 17: { // vertical
                    int k = rngs_u32(&rngseed) % 4;
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
                g->tiles[n].collision = TILE_BLOCK;
            } break;
            case TILE_SLOPE_45_0:
            case TILE_SLOPE_45_1:
            case TILE_SLOPE_45_2:
            case TILE_SLOPE_45_3: {
                static const int nmasks[4 * 3] = {AT_E, AT_S, AT_SE, // masks for checking neighbours: SHAPE - X | Y | DIAGONAL
                                                  AT_E, AT_N, AT_NE,
                                                  AT_W, AT_S, AT_SW,
                                                  AT_W, AT_N, AT_NW};

                int *nm = &nmasks[(tshape - TILE_SLOPE_45) * 3];
                int  xn = (m & nm[0]) != 0;                          // x neighbour
                int  yn = (m & nm[1]) != 0;                          // y neighbour
                int  cn = (m & nm[2]) != 0;                          // diagonal neighbour
                xcoord  = 8 + (xn && yn && cn ? 4 : xn | (yn << 1)); // slope image index
                ycoord += (tshape - TILE_SLOPE_45);

                g->tiles[n].collision = tshape;
            } break;
            }

            g->rtiles[n].layer[0] = rtile_pack(xcoord, ycoord);
        }
    }
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