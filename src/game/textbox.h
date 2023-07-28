/* =============================================================================
* Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
* This source code is licensed under the GPLv3 license found in the
* LICENSE file in the root directory of this source tree.
============================================================================= */

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include "gamedef.h"

struct textbox_s {
        bool32    active;
        fntchar_s chars[256];
};

#endif