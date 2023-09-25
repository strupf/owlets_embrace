// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "savefile.h"
#include "game.h"

#define SAVEFILE_NAME(SLOTID) "savefile_" #SLOTID ".sav"

static OS_FILE *savefile_open(int slotID, const char *mode);
static bool32   savefile_write(int slotID, savefile_s *sf);
static bool32   savefile_read(int slotID, savefile_s *sf);

bool32 game_savefile_new(game_s *g, int slotID)
{
        g->savefile_slotID = slotID;

        game_load_map(g, ASSET_PATH_MAPS "template.tmj");
        return 1;
}

bool32 game_savefile_load(game_s *g, int slotID)
{
        savefile_s sf = {0};
        if (!savefile_read(slotID, &sf)) {
                return 0;
        }

        game_load_map(g, sf.area_filename);
        g->tick            = sf.tick;
        g->savefile_slotID = slotID;
        os_memcpy(g->area_filename, sf.area_filename, sizeof(sf.area_filename));
        os_memcpy(g->hero.hero_name, sf.hero_name, sizeof(sf.hero_name));

        objset_s set = {0};
        obj_get_with_ID(g, OBJ_ID_SAVEPOINT, &set);
        obj_listc_s list = objset_list(&set);
        for (int n = 0; n < list.n; n++) {
                obj_s *o = list.o[n];
                if (o->tiledID == sf.savepointID) {
                        g->hero.obj.o->pos = o->pos;
                        break;
                }
        }

        return 1;
}

bool32 game_savefile_save(game_s *g)
{
        savefile_s sf  = {0};
        sf.saveslotID  = g->savefile_slotID;
        sf.savepointID = g->savepointID;
        sf.tick        = g->tick;
        os_memcpy(sf.area_filename, g->area_filename, sizeof(sf.area_filename));
        os_memcpy(sf.hero_name, g->hero.hero_name, sizeof(sf.hero_name));
        return savefile_write(g->savefile_slotID, &sf);
}

bool32 game_savefile_exists(int slotID)
{
        OS_FILE *file = savefile_open(slotID, "r");
        if (!file) return 0;

        os_fclose(file);
        return 1;
}

bool32 game_savefile_preview(int slotID, savefile_s *sf)
{
        if (!savefile_read(slotID, sf)) {
                return 0;
        }
        return 1;
}

bool32 game_savefile_copy(int slotID_from, int slotID_to)
{
        NOT_IMPLEMENTED
        return 0;
}

bool32 game_savefile_delete(int slotID)
{
#if TARGET_PD
        NOT_IMPLEMENTED
#else
        if (0 <= slotID && slotID < NUM_SAVEFILES) {
                switch (slotID) {
                case 0: return remove(SAVEFILE_NAME(0)) == 0;
                case 1: return remove(SAVEFILE_NAME(1)) == 0;
                case 2: return remove(SAVEFILE_NAME(2)) == 0;
                }
        }
#endif
        return 0;
}

bool32 game_savefile_write_unused(int slotID)
{
        savefile_s sf = {0};
        sf.saveslotID = slotID;
        return savefile_write(slotID, &sf);
}

static OS_FILE *savefile_open(int slotID, const char *mode)
{
        if (0 <= slotID && slotID < NUM_SAVEFILES) {
                switch (slotID) {
                case 0: return os_fopen(SAVEFILE_NAME(0), mode);
                case 1: return os_fopen(SAVEFILE_NAME(1), mode);
                case 2: return os_fopen(SAVEFILE_NAME(2), mode);
                }
        }

        return NULL;
}

static bool32 savefile_write(int slotID, savefile_s *sf)
{
        OS_FILE *file = savefile_open(slotID, "w");
        if (!file) return 0; // cant access file

        size_t res = os_fwrite(sf, sizeof(savefile_s), 1, file);
        os_fclose(file);
        return (res == 1); // successfully written?
}

static bool32 savefile_read(int slotID, savefile_s *sf)
{
        OS_FILE *file = savefile_open(slotID, "r");
        if (!file) return 0; // file does not exist

        size_t res = os_fread(sf, sizeof(savefile_s), 1, file);
        os_fclose(file);
        return (res == 1); // save file corrupted?
}
