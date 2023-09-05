// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "autotiling.h"
#include "util/tmj.h"

// x and y coordinate IDs in blob pattern
// used to look up tile after marching squares (x, y, x, y, ...)
// x = index * 2, y = index * 2 + 1
// pattern according to:
// opengameart.org/content/seamless-tileset-template
// this pattern allows some duplicated border tiles to include
// more variety later on
static const u8 blob_duplicate[] = {
    0, 1, // 17 vertical
    0, 3, // 17
    0, 4, // 17
    1, 4, // 31 left
    1, 6, // 31
    5, 3, // 31
    1, 0, // 68 horizontal
    3, 0, // 68
    4, 0, // 68
    4, 1, // 124 top
    6, 1, // 124
    3, 5, // 124
    2, 7, // 199 bot
    3, 7, // 199
    4, 2, // 199
    2, 4, // 241 right
    7, 2, // 241
    7, 3, // 241
    2, 6, // 255 mid
    3, 6, // 255
    6, 2, // 255
    6, 3, // 255
};

static const u8 blobpattern[256 * 2] = {
    0, 6, // 0
    7, 6, // 1
    0, 0, // 2
    0, 0, // 3
    0, 7, // 4
    0, 5, // 5
    0, 0, // 6
    3, 4, // 7
    0, 0, // 8
    0, 0, // 9
    0, 0, // 10
    0, 0, // 11
    0, 0, // 12
    0, 0, // 13
    0, 0, // 14
    0, 0, // 15
    7, 0, // 16
    0, 1, // 17
    0, 0, // 18
    0, 0, // 19
    0, 0, // 20
    0, 2, // 21
    0, 0, // 22
    3, 2, // 23
    0, 0, // 24
    0, 0, // 25
    0, 0, // 26
    0, 0, // 27
    1, 1, // 28
    1, 3, // 29
    0, 0, // 30
    1, 4, // 31
    0, 0, // 32
    0, 0, // 33
    0, 0, // 34
    0, 0, // 35
    0, 0, // 36
    0, 0, // 37
    0, 0, // 38
    0, 0, // 39
    0, 0, // 40
    0, 0, // 41
    0, 0, // 42
    0, 0, // 43
    0, 0, // 44
    0, 0, // 45
    0, 0, // 46
    0, 0, // 47
    0, 0, // 48
    0, 0, // 49
    0, 0, // 50
    0, 0, // 51
    0, 0, // 52
    0, 0, // 53
    0, 0, // 54
    0, 0, // 55
    0, 0, // 56
    0, 0, // 57
    0, 0, // 58
    0, 0, // 59
    0, 0, // 60
    0, 0, // 61
    0, 0, // 62
    0, 0, // 63
    6, 7, // 64
    6, 6, // 65
    0, 0, // 66
    0, 0, // 67
    1, 0, // 68
    5, 7, // 69
    0, 0, // 70
    1, 7, // 71
    0, 0, // 72
    0, 0, // 73
    0, 0, // 74
    0, 0, // 75
    0, 0, // 76
    0, 0, // 77
    0, 0, // 78
    0, 0, // 79
    5, 0, // 80
    7, 5, // 81
    0, 0, // 82
    0, 0, // 83
    2, 0, // 84
    5, 6, // 85
    0, 0, // 86
    1, 2, // 87
    0, 0, // 88
    0, 0, // 89
    0, 0, // 90
    0, 0, // 91
    3, 1, // 92
    3, 3, // 93
    0, 0, // 94
    1, 5, // 95
    0, 0, // 96
    0, 0, // 97
    0, 0, // 98
    0, 0, // 99
    0, 0, // 100
    0, 0, // 101
    0, 0, // 102
    0, 0, // 103
    0, 0, // 104
    0, 0, // 105
    0, 0, // 106
    0, 0, // 107
    0, 0, // 108
    0, 0, // 109
    0, 0, // 110
    0, 0, // 111
    4, 3, // 112
    7, 1, // 113
    0, 0, // 114
    0, 0, // 115
    2, 3, // 116
    2, 1, // 117
    0, 0, // 118
    4, 5, // 119
    0, 0, // 120
    0, 0, // 121
    0, 0, // 122
    0, 0, // 123
    4, 1, // 124
    5, 1, // 125
    0, 0, // 126
    5, 4, // 127
    0, 0, // 128
    0, 0, // 129
    0, 0, // 130
    0, 0, // 131
    0, 0, // 132
    0, 0, // 133
    0, 0, // 134
    0, 0, // 135
    0, 0, // 136
    0, 0, // 137
    0, 0, // 138
    0, 0, // 139
    0, 0, // 140
    0, 0, // 141
    0, 0, // 142
    0, 0, // 143
    0, 0, // 144
    0, 0, // 145
    0, 0, // 146
    0, 0, // 147
    0, 0, // 148
    0, 0, // 149
    0, 0, // 150
    0, 0, // 151
    0, 0, // 152
    0, 0, // 153
    0, 0, // 154
    0, 0, // 155
    0, 0, // 156
    0, 0, // 157
    0, 0, // 158
    0, 0, // 159
    0, 0, // 160
    0, 0, // 161
    0, 0, // 162
    0, 0, // 163
    0, 0, // 164
    0, 0, // 165
    0, 0, // 166
    0, 0, // 167
    0, 0, // 168
    0, 0, // 169
    0, 0, // 170
    0, 0, // 171
    0, 0, // 172
    0, 0, // 173
    0, 0, // 174
    0, 0, // 175
    0, 0, // 176
    0, 0, // 177
    0, 0, // 178
    0, 0, // 179
    0, 0, // 180
    0, 0, // 181
    0, 0, // 182
    0, 0, // 183
    0, 0, // 184
    0, 0, // 185
    0, 0, // 186
    0, 0, // 187
    0, 0, // 188
    0, 0, // 189
    0, 0, // 190
    0, 0, // 191
    0, 0, // 192
    2, 2, // 193
    0, 0, // 194
    0, 0, // 195
    0, 0, // 196
    4, 7, // 197
    0, 0, // 198
    2, 7, // 199
    0, 0, // 200
    0, 0, // 201
    0, 0, // 202
    0, 0, // 203
    0, 0, // 204
    0, 0, // 205
    0, 0, // 206
    0, 0, // 207
    0, 0, // 208
    7, 4, // 209
    0, 0, // 210
    0, 0, // 211
    0, 0, // 212
    6, 5, // 213
    0, 0, // 214
    5, 5, // 215
    0, 0, // 216
    0, 0, // 217
    0, 0, // 218
    0, 0, // 219
    0, 0, // 220
    4, 4, // 221
    0, 0, // 222
    5, 2, // 223
    0, 0, // 224
    0, 0, // 225
    0, 0, // 226
    0, 0, // 227
    0, 0, // 228
    0, 0, // 229
    0, 0, // 230
    0, 0, // 231
    0, 0, // 232
    0, 0, // 233
    0, 0, // 234
    0, 0, // 235
    0, 0, // 236
    0, 0, // 237
    0, 0, // 238
    0, 0, // 239
    0, 0, // 240
    7, 2, // 241
    0, 0, // 242
    0, 0, // 243
    0, 0, // 244
    4, 6, // 245
    0, 0, // 246
    6, 4, // 247
    0, 0, // 248
    0, 0, // 249
    0, 0, // 250
    0, 0, // 251
    0, 0, // 252
    2, 5, // 253
    0, 0, // 254
    2, 6, // 255
};

enum {
        TILESHAPE_BLOCK,
        TILESHAPE_SLOPE_45,
        TILESHAPE_SLOPE_LO,
        TILESHAPE_SLOPE_HI,
};

enum { // max tile variations
        NUM_VARIANTS_MID    = 4,
        NUM_VARIANTS_BORDER = 3,
};

typedef struct {
        int num_mid;    // number of tile variations for the center tile
        int num_border; // number of tile variations for the border tiles
} autotile_info_s;

static const autotile_info_s autotile_info[NUM_AUTOTILE_TYPES] = {
    0 // no variations for now
};

// set tiles automatically - www.cr31.co.uk/stagecast/wang/blob.html
typedef struct {
        u32 *arr;
        int  w;
        int  h;
        int  x;
        int  y;
} autotiling_s;

bool32 is_autotile(u32 tileID)
{
        int t = (tileID & 0xFFFFFFFU);
        return (0 < t && t < TMJ_TILESET_FGID);
}

// checks if there is a fitting tiletype in the direction of
// sx and sy to consider as a neighbour tile during marching squares
static bool32 autotile_fits(autotiling_s tiling, int sx, int sy, int ttype)
{
        int u = tiling.x + sx;
        int v = tiling.y + sy;

        // tiles on the edge of a room always have a neighbour
        if (!(0 <= u && u < tiling.w && 0 <= v && v < tiling.h)) return 1;

        u32 tileID = tiling.arr[u + v * tiling.w];
        if (tileID == 0) return 0;          // empty tile
        if (!is_autotile(tileID)) return 0; // no autotile
        u32 tileID_nf   = (tileID & 0xFFFFFFFu) - 1;
        int terraintype = (tileID_nf / 16);
        int tileshape   = (tileID_nf % 16);
        int flags       = tmj_decode_flipping_flags(tileID);

        // slopes are considered to be a neighbour only
        // from a handful of directions, also depending on flipped x/y

        // combine sx and sy into a lookup index
        // ((sx + 1) << 2) | (sy + 1)
        static const int adjtab[] = {
            0x1, // sx = -1, sy = -1
            0x5, // sx = -1, sy =  0
            0x4, // sx = -1, sy = +1
            0,
            0x3, // sx =  0, sy = -1
            0,   // sx =  0, sy =  0
            0xC, // sx =  0, sy = +1
            0,
            0x2, // sx = +1, sy = -1
            0xA, // sx = +1, sy =  0
            0x8, // sx = +1, sy = +1
        };

        // bitmask indicating if a shape is considered adjacent
        // eg. slope going upwards: /| 01 = 0111
        //                         /_| 11
        // index: (shape << 3) | (flipping flags - xyz = 3 variants)
        static const int adjacency[] = {
            0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, // block
            0x7, 0xD, 0xB, 0xE, 0x7, 0xD, 0xB, 0xE, // slope 45
            0x3, 0xC, 0x3, 0xC, 0x5, 0x5, 0xA, 0xA, // slope LO
            0x7, 0xD, 0xB, 0xE, 0x7, 0xD, 0xB, 0xE  // slope HI
        };

        int i = (tileshape << 3) | flags;
        int l = ((sx + 1) << 2) | ((sy + 1));
        return (adjacency[i] & adjtab[l]) == adjtab[l];
}

void autotile_calc(game_s *g, u32 *arr, int w, int h,
                   int x, int y, int n, int layerID)
{
        autotiling_s tiling  = {arr, w, h, x, y};
        static u32   rngseed = 145;

        u32 tileID    = tiling.arr[n];
        u32 t         = (tileID & 0xFFFFFFFu) - 1;
        int ttype     = (t / 16);
        int tileshape = (t % 16);
        u32 flags     = tmj_decode_flipping_flags(tileID);

        int ytype; // y tile position in huge tileset texture
        if (layerID == TILE_LAYER_MAIN) {
                ytype = ttype;
        } else {
                // rendering autotiles don't contain slopes
                // -> two tile graphics next to each other
                ytype = (NUM_AUTOTILE_MAIN + ((ttype - NUM_AUTOTILE_MAIN) >> 1));
        }

        ASSERT((layerID == TILE_LAYER_MAIN && ttype < NUM_AUTOTILE_MAIN) ||
               (layerID == TILE_LAYER_BG && NUM_AUTOTILE_MAIN <= ttype && ttype < NUM_AUTOTILE_BG));

        // calculate tileset coordinates
        int tx = 0;
        int ty = 0;
        switch (tileshape) {
        case TILESHAPE_BLOCK: {
                if (layerID == TILE_LAYER_MAIN) g->tiles[n] = TILE_BLOCK;

                int m = 0;
                // edges
                if (autotile_fits(tiling, -1, +0, ttype)) m |= 0x40; // left
                if (autotile_fits(tiling, +1, +0, ttype)) m |= 0x04; // right
                if (autotile_fits(tiling, +0, -1, ttype)) m |= 0x01; // top
                if (autotile_fits(tiling, +0, +1, ttype)) m |= 0x10; // down

                // corners only if there are the two corresponding edge neighbours
                if ((m & 0x41) == 0x41 && autotile_fits(tiling, -1, -1, ttype))
                        m |= 0x80; // top left
                if ((m & 0x50) == 0x50 && autotile_fits(tiling, -1, +1, ttype))
                        m |= 0x20; // bot left
                if ((m & 0x05) == 0x05 && autotile_fits(tiling, +1, -1, ttype))
                        m |= 0x02; // top right
                if ((m & 0x14) == 0x14 && autotile_fits(tiling, +1, +1, ttype))
                        m |= 0x08; // bot right

                // index into blob_duplicate[] for tile variations
                int blob_index = 0;
                switch (m) {
                case 31: blob_index = 3; break;
                case 68: blob_index = 6; break;
                case 124: blob_index = 9; break;
                case 199: blob_index = 12; break;
                case 241: blob_index = 15; break;
                case 255: blob_index = 18; break;
                }

                // check if tile variation -> random variation
                switch (m) {
                case 17:    // vertical beam
                case 31:    // left border
                case 68:    // horizontal beam
                case 124:   // top border
                case 199:   // bottom border
                case 241:   // right border
                case 255: { // center tile
                        autotile_info_s a = autotile_info[ttype];
                        int             vars;
                        if (m == 255)
                                vars = CLAMP(a.num_mid, 1, NUM_VARIANTS_MID);
                        else
                                vars = CLAMP(a.num_border, 1, NUM_VARIANTS_BORDER);
                        blob_index += rngs_max_u32(&rngseed, vars - 1);

                        // get coordinates from variation table
                        tx = blob_duplicate[blob_index * 2];
                        ty = blob_duplicate[blob_index * 2 + 1];
                } break;
                default:
                        // get coordinates from normal blob table
                        tx = blobpattern[m * 2];
                        ty = blobpattern[m * 2 + 1];
                        break;
                }

                if (layerID == TILE_LAYER_BG && (ttype & 1))
                        tx += 8;
        } break;
        case TILESHAPE_SLOPE_45: {
                ASSERT(layerID == TILE_LAYER_MAIN); // background tiles don't have slopes

                flags %= 4; // last 4 transformations are the same
                g->tiles[n] = TILE_SLOPE_45 + flags;

                /* only look for neighbours in 2 directions based on flipping
                 *    0     1     2     3
                 *         ___         ___
                 *    /|   \ |   |\    | /
                 *   /_|    \|   |_\   |/
                 */
                int sx = 0;
                int sy = 0;
                switch (flags) {
                case 0: sx = +1, sy = +1; break;
                case 1: sx = +1, sy = -1; break;
                case 2: sx = -1, sy = +1; break;
                case 3: sx = -1, sy = -1; break;
                }

                // neighbour in x, y and corner (only if x and y)
                bool32 xn = autotile_fits(tiling, sx, +0, ttype);
                bool32 yn = autotile_fits(tiling, +0, sy, ttype);
                bool32 cn = xn && yn ? autotile_fits(tiling, sx, sy, ttype) : 0;

                // choose appropiate variant
                int z = 4 - (cn ? 4 : ((yn > 0) << 1) | (xn > 0));
                tx    = 18 + z;
                ty    = 0 + flags;
        } break;
        case TILESHAPE_SLOPE_LO: {
                ASSERT(layerID == TILE_LAYER_MAIN); // background tiles don't have slopes

                // TODO: not autotiled
                tx          = 8;
                ty          = 0 + flags;
                g->tiles[n] = TILE_SLOPE_LO + flags;
        } break;
        case TILESHAPE_SLOPE_HI: {
                ASSERT(layerID == TILE_LAYER_MAIN); // background tiles don't have slopes

                // TODO: not autotiled
                tx          = 13;
                ty          = 0 + flags;
                g->tiles[n] = TILE_SLOPE_HI + flags;
        } break;
        }

        g->rtiles[n].ID[layerID] = tileID_encode(tx, ty + ytype * 8);
}