// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "dialog.h"
#include "game.h"

void dialog_action_exe(g_s *g, dialog_s *d, i32 ID1, i32 ID2)
{
    if (0) {

    } else if (0x80 <= ID1 && ID1 < 0xD0) {
        const char tag[6] = {hex_from_num(ID1 >> 4),
                             hex_from_num(ID1 & 0xF), ':',
                             hex_from_num(ID2 >> 4),
                             hex_from_num(ID2 & 0xF)};

        dialog_load_tag(g, tag);
        d->state = DIALOG_ST_WRITING;
        d->tick  = 0;
    } else if (ID1 == DIALOG_ID1_CLOSE) {
        dialog_close(g);
    } else if (ID1 == DIALOG_ID1_SHOP) {
    }
}