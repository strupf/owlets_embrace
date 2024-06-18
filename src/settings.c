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

bool32 settings_save(settings_s *s)
{
    void *f = pltf_file_open_w(SETTINGS_FILENAME);
    if (!f) return 0;
    u32 ver = GAME_VERSION;
    pltf_file_w(f, &ver, sizeof(u32));
    pltf_file_w(f, s, sizeof(settings_s));
    pltf_file_close(f);
    return 1;
}

bool32 settings_load(settings_s *s)
{
    if (!s) return 0;
    void *f = pltf_file_open_r(SETTINGS_FILENAME);
    if (!f) return 0;
    u32 ver = 0;
    pltf_file_r(f, &ver, sizeof(u32));
    pltf_file_r(f, s, sizeof(settings_s));
    pltf_file_close(f);
    return 1;
}