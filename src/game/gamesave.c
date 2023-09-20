// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game.h"

enum {
        NUM_SAVEFILES = 3,
};

typedef struct {
        u32  playeritems;
        char area_filename[LEN_STR_AREA_FILENAME];
        char playername[LEN_STR_PLAYER_NAME];
} savedata_s;

static OS_FILE *savefile_open(int slotID, const char *mode);
static bool32   savefile_write(int slotID, savedata_s *sd);
static bool32   savefile_read(int slotID, savedata_s *sd);

bool32 game_savestate_new(game_s *g, int slotID)
{
        g->savestate_slotID = slotID;

        game_load_map(g, ASSET_PATH_MAPS "template.tmj");
}

bool32 game_savestate_load(game_s *g, int slotID)
{
        savedata_s sd = {0};
        if (!savefile_read(slotID, &sd)) {
                return;
        }

        game_load_map(g, sd.area_filename);
        g->hero.aquired_items = sd.playeritems;
        os_memcpy(g->hero.playername, sd.playername, sizeof(g->hero.playername));
}

bool32 game_savestate_save(game_s *g)
{
        int        slotID = g->savestate_slotID;
        savedata_s sd     = {0};
        sd.playeritems    = g->hero.aquired_items;
        os_memcpy(sd.playername, g->hero.playername, sizeof(g->hero.playername));
        os_memcpy(sd.area_filename, g->area_filename, sizeof(g->area_filename));

        return savefile_write(slotID, &sd);
}

bool32 game_savestate_exists(int slotID)
{
        OS_FILE *file = savefile_open(slotID, "r");
        if (!file) return 0;

        os_fclose(file);
        return 1;
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

static bool32 savefile_write(int slotID, savedata_s *sd)
{
        OS_FILE *file = savefile_open(slotID, "w");
        if (!file) {
                PRINTF("CAN'T WRITE SAVEFILE\n");
                return 0;
        }

        if (os_fwrite(sd, sizeof(savedata_s), 1, file) != 1) {
                os_fclose(file);
                PRINTF("ERROR WHILE WRITING SAVEFILE\n");
                return 0;
        }

        os_fclose(file);
        return 1;
}

static bool32 savefile_read(int slotID, savedata_s *sd)
{
        OS_FILE *file = savefile_open(slotID, "r");
        if (!file) {
                PRINTF("SAVEFILE %i DOES NOT EXIST\n", slotID);
                return 0;
        }

        if (os_fread(sd, sizeof(savedata_s), 1, file) != 1) {
                PRINTF("SAVEFILE CORRUPTED\n");
                os_fclose(file);
                return 0;
        }
        os_fclose(file);
        return 1;
}
