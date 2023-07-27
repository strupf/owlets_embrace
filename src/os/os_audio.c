#include "os_internal.h"

enum {
        OS_AUDIO_MEM = 0x10000
};

static struct {
        snd_s snd_tab[NUM_SNDID];

        memarena_s      mem;
        ALIGNAS(4) char mem_raw[OS_AUDIO_MEM];
} g_snd;

void os_audio_init()
{
        memarena_init(&g_snd.mem, g_snd.mem_raw, OS_AUDIO_MEM);
}

snd_s snd_get(int ID)
{
        ASSERT(0 <= ID && ID < NUM_SNDID);
        return g_snd.snd_tab[ID];
}