// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "world.h"
#include "game.h"

static const char *g_world_files[] = {
    ASSET_PATH_MAPS "ww.world"};

static world_def_s g_world_defs[ARRLEN(g_world_files)];

static void world_def_load_file(world_def_s *w, const char *filename);

void world_def_init()
{
        for (int n = 0; n < ARRLEN(g_world_files); n++) {
                world_def_s *w = &g_world_defs[n];
                world_def_load_file(w, g_world_files[n]);
        }
}

world_area_def_s *world_area_by_filename(const char *areafile)
{
        for (int n = 0; n < ARRLEN(g_world_files); n++) {
                world_def_s *w = &g_world_defs[n];
                for (int i = 0; i < w->n_areas; i++) {
                        world_area_def_s *wd = &w->areas[i];
                        if (streq(areafile, wd->filename)) {
                                return wd;
                        }
                }
        }
        return NULL;
}

world_def_s *world_area_parent(const char *areafile)
{
        for (int n = 0; n < ARRLEN(g_world_files); n++) {
                world_def_s *w = &g_world_defs[n];
                for (int i = 0; i < w->n_areas; i++) {
                        if (streq(areafile, w->areas[i].filename)) {
                                return w;
                        }
                }
        }
        return NULL;
}

world_area_def_s *world_get_area(world_def_s *w, world_area_def_s *curr_area, rec_i32 rec)
{
        for (int n = 0; n < w->n_areas; n++) {
                world_area_def_s *wd = &w->areas[n];
                if (wd != curr_area && overlap_rec_excl(wd->r, rec))
                        return wd;
        }
        return NULL;
}

static void world_def_load_file(world_def_s *w, const char *filename)
{
        w->n_areas = 0;
        os_spmem_push();

        char *txt = txt_read_file_alloc(filename, os_spmem_alloc);
        jsn_s jroot;
        jsn_root(txt, &jroot);
        os_strcpy(w->filename, filename);

        foreach_jsn_childk (jroot, "maps", jmap) {
                world_area_def_s *wd = &w->areas[w->n_areas++];
                os_strcpy(wd->filename, ASSET_PATH_MAPS);
                char namebuf[64];
                jsn_strk(jmap, "fileName", namebuf, sizeof(namebuf));
                os_strcat(wd->filename, namebuf);
                wd->r.x = jsn_intk(jmap, "x");
                wd->r.y = jsn_intk(jmap, "y");
                wd->r.w = jsn_intk(jmap, "width");
                wd->r.h = jsn_intk(jmap, "height");
        }

        os_spmem_pop();
}