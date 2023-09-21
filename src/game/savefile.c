// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "savefile.h"
#include "game.h"

typedef struct {
        char area_filename[LEN_STR_AREA_FILENAME];
        char playername[LEN_STR_PLAYER_NAME];
} savefile_s;

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
        os_memcpy(g->hero.playername, sf.playername, sizeof(g->hero.playername));
        return 1;
}

bool32 game_savefile_save(game_s *g)
{
        savefile_s sf = {0};
        os_memcpy(sf.playername, g->hero.playername, sizeof(g->hero.playername));
        os_memcpy(sf.area_filename, g->area_filename, sizeof(g->area_filename));

        int slotID = g->savefile_slotID;
        return savefile_write(slotID, &sf);
}

bool32 game_savefile_exists(int slotID)
{
        OS_FILE *file = savefile_open(slotID, "r");
        if (!file) return 0;

        os_fclose(file);
        return 1;
}

bool32 game_savefile_try_load_preview(int slotID, savefile_preview_s *p)
{
        savefile_s sf = {0};
        if (!savefile_read(slotID, &sf)) {
                return 0;
        }

        savefile_preview_s preview = {0};

        *p = preview;
        return 1;
}

bool32 game_savefile_copy(int slotID_from, int slotID_to)
{
        NOT_IMPLEMENTED
        return 0;
}

bool32 game_savefile_delete(int slotID)
{
        NOT_IMPLEMENTED
        return 0;
}

static OS_FILE *savefile_open(int slotID, const char *mode)
{
        if (0 <= slotID && slotID < NUM_SAVEFILES) {
                switch (slotID) {
                case 0: return os_fopen("savefile_0.sav", mode);
                case 1: return os_fopen("savefile_1.sav", mode);
                case 2: return os_fopen("savefile_2.sav", mode);
                }
        }

        return NULL;
}

static bool32 savefile_write(int slotID, savefile_s *sf)
{
        OS_FILE *file = savefile_open(slotID, "w");
        if (!file) return 0; // cant access file

        int res = os_fwrite(sf, sizeof(savefile_s), 1, file);
        os_fclose(file);
        return (res == 1); // successfully written?
}

static bool32 savefile_read(int slotID, savefile_s *sf)
{
        OS_FILE *file = savefile_open(slotID, "r");
        if (!file) return 0; // file does not exist

        int res = os_fread(sf, sizeof(savefile_s), 1, file);
        os_fclose(file);
        return (res == 1); // save file corrupted?
}
