// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================
//
#include "game/obj/arrow.c"
#include "game/obj/blob.c"
#include "game/obj/boat.c"
#include "game/obj/crumbleblock.c"
#include "game/obj/door.c"
#include "game/obj/hero.c"
#include "game/obj/npc.c"
#include "game/obj/savepoint.c"
//
#include "game/cam.c"
#include "game/decoration.c"
#include "game/draw.c"
#include "game/fading.c"
#include "game/game.c"
#include "game/gamedraw.c"
#include "game/gameinit.c"
#include "game/load.c"
#include "game/load_autotiling.c"
#include "game/obj.c"
#include "game/pathmovement.c"
#include "game/room.c"
#include "game/rope.c"
#include "game/savefile.c"
#include "game/textbox.c"
#include "game/title.c"
#include "game/transition.c"
#include "game/water.c"
#include "game/world.c"
//
#include "os/os.c"
#include "os/os_audio.c"
#include "os/os_fileio.c"
#include "os/os_graphics.c"
#ifdef TARGET_PD
#include "os/backend_pd.c"
#else
#include "os/backend_rl.c"
#endif
//
#include "util/memfunc.c"
//
#include "assets.c"
//
#include "game/game.h"
#include "os/os_internal.h"
