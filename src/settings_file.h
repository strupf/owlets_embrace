// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef SETTINGS_FILE_H
#define SETTINGS_FILE_H

#include "pltf/pltf_types.h"
#include "settings.h"

#define SETTINGS_FILE_VERSION 0

enum {
    SETTINGS_FILE_ERR_BAD_ARG  = 1 << 0,
    SETTINGS_FILE_ERR_OPEN     = 1 << 1,
    SETTINGS_FILE_ERR_RW       = 1 << 2,
    SETTINGS_FILE_ERR_VERSION  = 1 << 3,
    SETTINGS_FILE_ERR_CHECKSUM = 1 << 4,
    SETTINGS_FILE_ERR_CLOSE    = 1 << 5,
    SETTINGS_FILE_ERR_MISC     = 1 << 6,
};

typedef struct {
    settings_s s;
} settings_file_s;

#endif