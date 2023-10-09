// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef ASSETS_H
#define ASSETS_H

enum tex_id {
        TEXID_DISPLAY,
        TEXID_FONT_DEFAULT,
        TEXID_FONT_DEBUG,
        TEXID_TILESET,
        TEXID_TEXTBOX,
        TEXID_ITEMS,
        TEXID_CLOUDS,
        TEXID_PARTICLE,
        TEXID_SOLID,
        TEXID_HERO,
        TEXID_UI,
        TEXID_HOOK,
        TEXID_LAYER_1,
        TEXID_TITLESCREEN,
        TEXID_ITEM_SELECT_CACHE,
        TEXID_TITLE,
        //
        NUM_TEXID
};

enum snd_id {
        SNDID_DEFAULT,
        SNDID_JUMP,
        SNDID_TYPEWRITE,
        SNDID_HERO_LAND,
        SNDID_STEP,
        SNDID_HOOKATTACH,
        SNDID_BOW,
        //
        NUM_SNDID
};

enum fnt_id {
        FNTID_DEFAULT,
        FNTID_DEBUG,
        //
        NUM_FNTID
};

void assets_load();

#endif