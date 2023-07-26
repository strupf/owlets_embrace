#include "game.c"
#include "os.c"
#include "os/os_audio.c"
#include "os/os_fileio.c"
#include "os/os_graphics.c"

#if defined(TARGET_DESKTOP)
#include "os/os_raylib.c"
#elif defined(TARGET_PD)
#include "os/os_playdate.c"
#endif