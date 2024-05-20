#ifndef SOU_PATH_FINDING_H
#define SOU_PATH_FINDING_H

#include "va_util.h"
#include "va_memarena.h"
#include "va_linmath.h"

struct path_result
{
    b32 FoundPath;
    vec2i *Path;
    int PathSteps;
};

struct world;

internal_func path_result CalculatePath(world *World, vec2i Start, vec2i End, memory_arena *TrArena, memory_arena *ResultArena, b32 IgnoreNPCs = false);

#endif
