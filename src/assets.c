// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "assets.h"
#include "game/game.h"

static void load_tileanimations(const char *filename);
static void load_tileatlas(int texID);
static void load_prerendering();

void assets_load()
{
        load_tileanimations("assets/tileanimations.json");
        load_tileatlas(TEXID_TILESET);
        tex_put_load(TEXID_FONT_DEFAULT, "assets/font_mono_.json");
        tex_put_load(TEXID_FONT_DEBUG, "assets/font_debug.json");
        tex_put_load(TEXID_TEXTBOX, "assets/textbox.json");
        tex_put_load(TEXID_ITEMS, "assets/items.json");
        tex_put_load(TEXID_PARTICLE, "assets/particle.json");
        tex_put_load(TEXID_SOLID, "assets/misctex.json");
        tex_put_load(TEXID_HERO, "assets/player.json");
        tex_put_load(TEXID_UI, "assets/ui.json");
        tex_put_load(TEXID_HOOK, "assets/hook.json");
        tex_put_load(TEXID_CLOUDS, "assets/clouds.json");
        tex_put_load(TEXID_TITLESCREEN, "assets/titlescreen.json");
        tex_put_load(TEXID_TITLE, "assets/title.json");
        tex_put_load(TEXID_PARALLAX_FOREST, "assets/background.json");
        tex_put_load(TEXID_TEST, "assets/autotiles_bricks.json");
        // tex_put_load(TEXID_PLANTS, "assets/plants.json");
        tex_put(TEXID_ITEM_SELECT_CACHE, tex_create(ITEM_FRAME_SIZE, ITEM_FRAME_SIZE, 1));
        tex_put(TEXID_TILEMAP_CACHE, tex_create(512, 256, 1));

        fnt_put_load(FNTID_DEFAULT, "assets/fnt/font_default.json");
        fnt_put_load(FNTID_DEBUG, "assets/fnt/font_debug.json");

        snd_put_load(SNDID_DEFAULT, "assets/snd/sample.wav");
        snd_put_load(SNDID_JUMP, "assets/snd/jump.wav");
        snd_put_load(SNDID_TYPEWRITE, "assets/snd/speak.wav");
        snd_put_load(SNDID_HERO_LAND, "assets/snd/land.wav");
        snd_put_load(SNDID_STEP, "assets/snd/step.wav");
        snd_put_load(SNDID_HOOKATTACH, "assets/snd/hookattach.wav");
        snd_put_load(SNDID_BOW, "assets/snd/bow.wav");
        snd_put_load(SNDID_SWORD, "assets/snd/sword.wav");

        tex_place_outline(tex_get(TEXID_HERO), (rec_i32){0, 0, 64 * 4, 64 * 6}, 0, 1);
        load_prerendering();
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
                char *txt = txt_read_file_alloc(filenames[n], os_spmem_alloc);
                jsn_s j;
                jsn_root(txt, &j);
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

static void load_prerendering()
{
        // tex_s t = tex_get(TEXID_PLANTS);
}