// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

#ifdef TARGET_PD
static LCDBitmap *menubm;

void menufunction(void *arg)
{
        PRINTF("print\n");
}
#endif

static void load_tileatlas(int texID);
static void load_tileanimations(const char *filename);

void game_init(game_s *g)
{
#ifdef TARGET_PD
        menubm = PD->graphics->newBitmap(400, 240, kColorWhite);
        PD->system->addMenuItem("Dummy", menufunction, NULL);
        PD->system->setMenuImage(menubm, 0);
#endif
        for (int n = 0; n < ARRLEN(g_tileIDs); n++) {
                g_tileIDs[n] = n;
        }
        load_tileanimations("assets/tileanimations.json");

        gfx_set_inverted(1);

        load_tileatlas(TEXID_TILESET);
        tex_put(TEXID_FONT_DEFAULT, tex_load("assets/font_mono_8.json"));
        tex_put(TEXID_FONT_DEBUG, tex_load("assets/font_debug.json"));
        tex_put(TEXID_TEXTBOX, tex_load("assets/textbox.json"));
        tex_put(TEXID_ITEMS, tex_load("assets/items.json"));
        tex_put(TEXID_PARTICLE, tex_load("assets/particle.json"));
        tex_put(TEXID_SOLID, tex_load("assets/solid.json"));
        tex_put(TEXID_HERO, tex_load("assets/player.json"));

        tex_s tclouds = tex_load("assets/clouds.json");
        tex_put(TEXID_CLOUDS, tclouds);

        // apply some "grey pattern"
        for (int y = 0; y < tclouds.h; y++) {
                for (int x = 0; x < tclouds.w; x++) {
                        if (((x + y) % 2 == 0) || x % 2 == 0 || y % 4 == 0) {
                                int i = (x >> 3) + y * tclouds.w_byte;
                                int b = (x & 7);
                                tclouds.px[i] &= ~(1 << (7 - b)); // clear bit
                        }
                }
        }

        fnt_put(FNTID_DEFAULT, fnt_load("assets/fnt/font_default.json"));
        fnt_put(FNTID_DEBUG, fnt_load("assets/fnt/font_debug.json"));

        g->rng    = 213;
        g->cam.w  = 400;
        g->cam.h  = 240;
        g->cam.wh = g->cam.w / 2;
        g->cam.hh = g->cam.h / 2;

        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ALIVE];
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_ACTOR];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_ACTOR);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_SOLID];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_SOLID);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_NEW_AREA_COLLIDER];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_NEW_AREA_COLLIDER);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_PICKUP];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_PICKUP);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_INTERACT];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_INTERACT);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_MOVABLE];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_MOVABLE);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_THINK_1];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_THINK_1);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_THINK_2];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_THINK_2);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }
        {
                objbucket_s *b = &g->objbuckets[OBJ_BUCKET_KILL_OFFSCREEN];
                b->op_func[0]  = OBJFLAGS_OP_AND;
                b->op_flag[0]  = objflags_create(OBJ_FLAG_KILL_OFFSCREEN);
                b->cmp_func    = OBJFLAGS_CMP_NZERO;
        }

        game_load_map(g, "assets/map/template.tmj");
}

static void load_tileatlas(int texID)
{
        static const char *filenames[] = {"assets/tiles_0.json",
                                          "assets/tiles_1.json",
                                          "assets/tiles_2.json",
                                          "assets/tiles_3.json",
                                          "assets/tiles_4.json",
                                          "assets/tiles_5.json",
                                          "assets/tiles_6.json",
                                          "assets/tiles_7.json",
                                          "assets/tiles_8.json",
                                          "assets/tiles_9.json"};
        enum {
                TILEATLAS_TILES_X = 32,
                TILEATLAS_W       = TILEATLAS_TILES_X * 16,
                TILEATLAS_H       = 32 * ARRLEN(filenames) * 16,
                TILEATLAS_W_BYTE  = TILEATLAS_TILES_X * 2,
        };

        tex_s t = tex_create(TILEATLAS_W, TILEATLAS_H, 1);
        tex_put(texID, t);

        int y_global = 0;
        for (int n = 0; n < ARRLEN(filenames); n++) {
                os_spmem_push();
                char *txtbuf = txt_read_file_alloc(filenames[n], os_spmem_alloc);
                jsn_s j;
                jsn_root(txtbuf, &j);
                i32 w = jsn_intk(j, "width");
                i32 h = jsn_intk(j, "height");
                ASSERT(w == TILEATLAS_W && h == TILEATLAS_W);
                char *c = jsn_strkptr(j, "data");

                // i = 0: parse pixel data block (black/white)
                // i = 1: parse transparency block (opaque, transparent)
                for (int i = 0; i < 2; i++) {
                        for (int y = 0; y < h; y++) {
                                int yy = (y_global + y) * TILEATLAS_W_BYTE;
                                for (int x = 0; x < TILEATLAS_W_BYTE; x++) {
                                        int c1 = (char_hex_to_int(*c++)) << 4;
                                        int c2 = (char_hex_to_int(*c++));
                                        int cc = c1 | c2;
                                        if (i)
                                                t.mk[x + yy] = cc;
                                        else
                                                t.px[x + yy] = cc;
                                }
                        }
                }
                y_global += h;
                os_spmem_pop();
        }
}

static void load_tileanimations(const char *filename)
{
        os_spmem_push();
        char *txt = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s jroot;
        jsn_root(txt, &jroot);
        int k = 0;
        foreach_jsn_childk (jroot, "animations", janim) {
                tile_animation_s *ta = &g_tileanimations[k++];
                foreach_jsn_childk (janim, "IDs", ji) {
                        int i                 = jsn_int(ji);
                        ta->IDs[ta->frames++] = i;
                }
                ta->ID    = jsn_intk(janim, "ID");
                ta->ticks = jsn_intk(janim, "ticks");
        }

        os_spmem_pop();
}