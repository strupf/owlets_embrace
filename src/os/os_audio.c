#include "os_audio.h"
#include "os_mem.h"

#define OS_AUDIO_MEM 0x10000

static struct {
        memarena_s      mem;
        ALIGNAS(4) char mem_raw[OS_AUDIO_MEM];
} g_snd;

void os_audio_init()
{
        memarena_init(&g_snd.mem, g_snd.mem_raw, OS_AUDIO_MEM);
}