// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "savefile.h"
#include "game.h"
#include "sys/sys.h"

#define SAVEFILE_NAME(SLOTID) "savefile_" #SLOTID ".sav"

static void *savefile_open(int slotID, int mode);

bool32 savefile_copy(int slotID_from, int slotID_to)
{
    savefile_s sf;
    if (savefile_read(slotID_from, &sf)) {
        return savefile_write(slotID_to, &sf);
    }
    return 0;
}

bool32 savefile_delete(int slotID)
{
    switch (slotID) {
    case 0: return sys_file_remove(SAVEFILE_NAME(0)) == 0;
    case 1: return sys_file_remove(SAVEFILE_NAME(1)) == 0;
    case 2: return sys_file_remove(SAVEFILE_NAME(2)) == 0;
    }
    BAD_PATH
    return 0;
}

bool32 savefile_write(int slotID, savefile_s *sf)
{
    if (!(0 <= slotID && slotID <= 2)) return 0;

    void *file = savefile_open(slotID, SYS_FILE_W);
    if (!file) return 0; // cant access file

    size_t res = sys_file_write(file, sf, sizeof(savefile_s));
    sys_file_close(file);
    return (res == 1); // successfully written?
}

bool32 savefile_read(int slotID, savefile_s *sf)
{
    if (!(0 <= slotID && slotID <= 2)) return 0;

    void *file = savefile_open(slotID, SYS_FILE_R);
    if (!file) { // file does not exist
        savefile_s sf_empty = {0};
        *sf                 = sf_empty;
        return 0;
    }

    size_t res = sys_file_read(file, sf, sizeof(savefile_s));
    sys_file_close(file);
    sys_printf("read: %i %i\n", res, sizeof(savefile_s));
    return (res == sizeof(savefile_s)); // save file corrupted?
}

bool32 savefile_exists(int slotID)
{
    void *file = savefile_open(slotID, SYS_FILE_R);
    if (file) {
        sys_file_close(file);
        return 1;
    }
    return 0;
}

static void *savefile_open(int slotID, int mode)
{
    switch (slotID) {
    case 0: return sys_file_open(SAVEFILE_NAME(0), mode);
    case 1: return sys_file_open(SAVEFILE_NAME(1), mode);
    case 2: return sys_file_open(SAVEFILE_NAME(2), mode);
    }
    BAD_PATH
    return NULL;
}