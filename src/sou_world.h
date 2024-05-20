#ifndef SOU_WORLD_H
#define SOU_WORLD_H

#include "va_common.h"
#include "sou_entity.h"
#include "sou_turn_queue.h"

struct ground_splat
{
    vec2 Position;
    f32 Rotation;
    f32 Scale;
};

struct world
{
    memory_arena Arena;
    
    int Width;
    int Height;
    int TilePxW;
    int TilePxH;

    u8 *Tiles;
    u8 *TilesInitialized;
    u8 *DarknessLevels;

    entity *Entities;
    int EntityTightCount;
    int EntityUsedCount;
    int EntityMaxCount;
    int EntityCurrentDebugID;

    entity **SpatialEntities;

    turn_queue TurnQueue;

    int GroundSplatCount;
    ground_splat *GroundSplats;

    entity *PlayerEntity;

    i64 CurrentTurn;

    vec2i Exit;
};

enum tile_type
{
    TILE_NONE = 0,
    TILE_STONE = 1,
    TILE_GRASS,
    TILE_WATER,
    TILE_COUNT
};

internal_func world *GenerateWorld(int WorldW, int WorldH, int TilePxW, int TilePxH, memory_arena *ScratchArena);

#endif
