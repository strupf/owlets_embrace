// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "settings.h"

#define SETTINGS_FILENAME "settings.dat"

settings_s settings_default()
{
    settings_s s = {0};

    return s;
}

bool32 settings_save(settings_s s)
{
    void *f = pltf_file_open(SETTINGS_FILENAME, PLTF_FILE_W);
    if (!f) return 0;
    s.game_version = GAME_VERSION;
    pltf_file_w(f, &s, sizeof(s));
    pltf_file_close(f);
    return 1;
}

bool32 settings_load(settings_s *s)
{
    if (!s) return 0;
    void *f = pltf_file_open(SETTINGS_FILENAME, PLTF_FILE_R);
    if (!f) return 0;
    u32 ver = pltf_file_r(f, s, sizeof(u32));
    pltf_file_r(f, s, sizeof(settings_s) - sizeof(u32));
    pltf_file_close(f);
    return 1;
}