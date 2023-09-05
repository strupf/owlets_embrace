// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "game/obj/arrow.c"
#include "game/obj/blob.c"
#include "game/obj/door.c"
#include "game/obj/hero.c"
#include "game/obj/npc.c"
#include "game/obj/obj.c"
#include "game/obj/objset.c"
//
#include "game/autotiling.c"
#include "game/backforeground.c"
#include "game/cam.c"
#include "game/draw.c"
#include "game/game.c"
#include "game/gamedraw.c"
#include "game/gameinit.c"
#include "game/load.c"
#include "game/maptransition.c"
#include "game/pathmovement.c"
#include "game/rope.c"
#include "game/savefile.c"
#include "game/textbox.c"
#include "game/tilegrid.c"
#include "game/water.c"
//
#include "os/os_audio.c"
#include "os/os_fileio.c"
#include "os/os_graphics.c"
#include "os/os_inp.c"
#include "os/os_mem.c"
//
#include "util/memfunc.c"
//
#ifdef TARGET_PD
#include "os/backend_pd.c"
#else
#include "os/backend_rl.c"
#endif
//
#include "assets.c"
#include "main.c"