#ifndef SOUTERRAIN_H
#define SOUTERRAIN_H

#include "sav_lib.h"

struct game_state
{
    b32 IsInitialized;

    memory_arena RootArena;
    memory_arena WorldArena;
    memory_arena ResourceArena;
    memory_arena TrArenaA;
    memory_arena TrArenaB;

    sav_texture TestTex;
};

#endif
