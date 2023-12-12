// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys/sys_types.h"

#if !SYS_CONFIG_ONLY_BACKEND
#include "core/app.c"
#include "core/assets.c"
#include "core/aud.c"
#include "core/gfx.c"
#include "core/inp.c"
#include "core/spm.c"

//
#include "util/json.c"
#include "util/mem.c"
//
#include "obj/crumbleblock.c"
#include "obj/hero.c"
#include "obj/obj.c"
//
#include "cam.c"
#include "fade.c"
#include "game.c"
#include "inventory.c"
#include "mainmenu.c"
#include "map_loader.c"
#include "render.c"
#include "rope.c"
#include "savefile.c"
#include "spriteanim.c"
#include "textbox.c"
#include "transition.c"
#include "water.c"
#endif

// include minimal engine files only
// -> standalone game loop
#include "sys/sys.c"
#if defined(SYS_PD)
#include "sys/sys_backend_pd.c"  // <- main function
#elif defined(SYS_SDL)           //
#include "sys/sys_backend_sdl.c" // <- main function
#endif