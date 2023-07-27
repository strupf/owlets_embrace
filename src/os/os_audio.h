#ifndef OS_AUDIO_H
#define OS_AUDIO_H

#include "os_types.h"

typedef struct {
        int x;
} snd_s;

enum {
        SNDID_DEFAULT = 0,
        //
        NUM_SNDID
};

snd_s snd_get(int ID);

#endif