// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "savefile.h"
#include "game.h"
#include "os/os.h"

typedef struct {
        u32  playeritems;
        char currentmap[32];
        char playername[LENGTH_PLAYERNAME];
} savedata_s;

static OS_FILE *savefile_open(int saveID, const char *mode)
{
        if (0 <= saveID && saveID < NUM_SAVEFILES) {
                switch (saveID) {
                case 0: return os_fopen("savefile_0.sav", mode);
                case 1: return os_fopen("savefile_1.sav", mode);
                case 2: return os_fopen("savefile_2.sav", mode);
                }
        }

        return NULL;
}

bool32 savefile_exists(int saveID)
{
        OS_FILE *file = savefile_open(saveID, "r");
        if (!file) return 0;

        os_fclose(file);
        return 1;
}

bool32 savefile_write(int saveID, game_s *g)
{
        OS_FILE *file = savefile_open(saveID, "w");
        if (!file) {
                PRINTF("CAN'T WRITE SAVEFILE\n");
                return 0;
        }

        savedata_s sd  = {0};
        sd.playeritems = g->hero.aquired_items;
        os_memcpy(sd.playername, g->hero.playername, LENGTH_PLAYERNAME);

        if (os_fwrite(&sd, sizeof(savedata_s), 1, file) != 1) {
                os_fclose(file);
                PRINTF("ERROR WHILE WRITING SAVEFILE\n");
                return 0;
        }

        os_fclose(file);
        return 1;
}

bool32 savefile_load(int saveID, game_s *g)
{
        OS_FILE *file = savefile_open(saveID, "r");
        if (!file) {
                PRINTF("SAVEFILE %i DOES NOT EXIST\n", saveID);
                return 0;
        }

        savedata_s sd;
        if (os_fread(&sd, sizeof(savedata_s), 1, file) != 1) {
                PRINTF("SAVEFILE CORRUPTED\n");
                os_fclose(file);
                return 0;
        }

        g->hero.aquired_items = sd.playeritems;
        os_memcpy(g->hero.playername, sd.playername, LENGTH_PLAYERNAME);

        os_fclose(file);
        return 1;
}