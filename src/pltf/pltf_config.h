// =============================================================================
// Copyright 2024, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

#ifndef PLTF_CONFIG_H
#define PLTF_CONFIG_H

#define PLTF_USE_STEREO   0
#define PLTF_SHOW_FPS     0 // show FPS, UPS, logic time and draw time
#define PLTF_EDIT_PD      0 // edit PD code files? -> enable PLTF_PD_HW code paths
#define PLTF_ENABLE_DEBUG 1

#if !defined(PLTF_RELEASE) && !PLTF_ENABLE_DEBUG
#define PLTF_RELEASE 1
#endif
#if PLTF_RELEASE
#define PLTF_DEBUG      0
#define PLTF_ENABLE_LOG 0
#else
#define PLTF_DEBUG      1
#define PLTF_ENABLE_LOG 1
#endif

#endif