// =============================================================================
// Copyright (C) 2023, Strupf (the.strupf@proton.me). All rights reserved.
// =============================================================================

#include "sys/sys_types.h"
//
#if !SYS_CONFIG_ONLY_BACKEND
// include files
#include "app.c"
#include "assets.c"
#include "aud.c"
#include "cam.c"
#include "game.c"
#include "gfx.c"
#include "hero.c"
#include "inp.c"
#include "map_loader.c"
#include "obj.c"
#include "render.c"
#include "rope.c"
#include "savefile.c"
#include "spm.c"
#include "spriteanim.c"
#include "textbox.c"
#include "title.c"
#include "transition.c"
#include "util/json.c"
#include "util/mem.c"
#endif

// include minimal engine files only
// -> standalone game loop
#include "sys/sys.c"
#if defined(SYS_PD)
#include "sys/sys_backend_pd.c"  // <- main function
#elif defined(SYS_SDL)           //
#include "sys/sys_backend_sdl.c" // <- main function
#endif