// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_CONFIG_H
#define PLTF_CONFIG_H

#ifndef PLTF_RELEASE
#define PLTF_RELEASE 1
#endif

#if PLTF_RELEASE
#define PLTF_DEBUG      0
#define PLTF_ENABLE_LOG 0
#else
#define PLTF_DEBUG      1
#define PLTF_ENABLE_LOG 1
#endif

#define PLTF_EDIT_PD 0 // edit PD code files? -> enable PLTF_PD_HW code paths

#endif