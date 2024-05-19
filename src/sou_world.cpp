#include "souterrain.h"

#include <cstring> // memset

#include "sou_pathfinding.cpp"
#include "sou_lineofsight.cpp"
#include "sou_entity.cpp"

static_i vec2i
GetRandomRoomP(room *Room, int Pad)
{
    int X = GetRandomValue(Room->X+Pad, Room->X+Room->W-Pad);
    int Y = GetRandomValue(Room->Y+Pad, Room->Y+Room->H-Pad);

    return Vec2I(X, Y);
}

static_i vec2i
GetCenterRoomP(room *Room)
{
    int X = Room->X + Room->W/2;
    int Y = Room->Y + Room->H/2;

    return Vec2I(X, Y);
}

void
GenerateRoomMap(world *World, u8 *GeneratedMap, room *GeneratedRooms, int RoomsMax, int *RoomCount)
{
    int TileCount = World->Width * World->Height;

    for (int i = 0; i < TileCount; i++)
    {
        World->Tiles[i] = TILE_STONE;
    }

    int SizeMin = 6;
    int SizeMax = 20;

    *RoomCount = 0;

    // NOTE: Generate room data and room floors
    for (int RoomI = 0; RoomI < RoomsMax; RoomI++)
    {
        room Room;
        Room.W = GetRandomValue(SizeMin, SizeMax);
        Room.H = GetRandomValue(SizeMin, SizeMax);
        Room.X = GetRandomValue(2, World->Width - Room.W - 2);
        Room.Y = GetRandomValue(2, World->Height - Room.H - 2);

        int RoomArea = Room.W * Room.H;
        if (RoomArea < 65)
        {
            Room.Type = ROOM_SMALL;
        }
        else if (RoomArea < 100)
        {
            Room.Type = ROOM_MEDIUM;
        }
        else if (RoomArea < 256)
        {
            Room.Type = ROOM_LARGE;
        }
        else
        {
            Room.Type = ROOM_XLARGE;
        }

        b32 Intersects = false;
        for (int Y = Room.Y - 2; Y < (Room.Y + Room.H + 2); Y++)
        {
            for (int X = Room.X - 2; X < (Room.X + Room.W + 2); X++)
            {
                if (GeneratedMap[XYToIdx(X, Y, World->Width)] != GEN_TILE_NONE)
                {
                    Intersects = true;
                    break;
                }
            }
        }

        if (!Intersects)
        {
            b32 GrassRoom = GetRandomValue(0, 2);
            for (int Y = Room.Y; Y < (Room.Y + Room.H); Y++)
            {
                for (int X = Room.X; X < (Room.X + Room.W); X++)
                {
                    int Idx = XYToIdx(X, Y, World->Width);
                    GeneratedMap[Idx] = GEN_TILE_FLOOR;
                    World->Tiles[Idx] = (u8) (GrassRoom ? TILE_GRASS : TILE_STONE);
                    World->TilesInitialized[Idx] = true;
                }
            }

            GeneratedRooms[(*RoomCount)++] = Room;
        }
    }

    // NOTE: Post process rooms to determine types
    {
        room *Room = GeneratedRooms;
        for (int RoomI = 0; RoomI < *RoomCount; RoomI++, Room++)
        {
            if (RoomI == 0)
            {
                Room->Type = ROOM_ENTRANCE;
            }
            else if (RoomI == (*RoomCount) - 1)
            {
                Room->Type = ROOM_EXIT;
            }
            else if (Room->Type == ROOM_LARGE)
            {
                if (GetRandomValue(0, 2) == 0)
                {
                    Room->Type = ROOM_TEMPLE;
                }
            }
        }
    }

    GeneratedRooms[0].Type = ROOM_ENTRANCE;
    GeneratedRooms[(*RoomCount) - 1].Type = ROOM_EXIT;

    // NOTE: Connect rooms with corridors
    for (int RoomI = 0; RoomI < *RoomCount - 1; RoomI++)
    {
        room *Room1 = GeneratedRooms + RoomI;
        room *Room2 = GeneratedRooms + RoomI + 1;
        
        vec2i Room1Center = Vec2I(Room1->X + Room1->W / 2, Room1->Y + Room1->H / 2);
        vec2i Room2Center = Vec2I(Room2->X + Room2->W / 2, Room2->Y + Room2->H / 2);

        vec2i LeftCenter = Room1Center.X < Room2Center.X ? Room1Center : Room2Center;
        vec2i RightCenter = Room1Center.X >= Room2Center.X ? Room1Center : Room2Center;

        int CorridorVersion = GetRandomValue(0, 2);

        int ConstY;
        int ConstX;
        if (CorridorVersion)
        {
            ConstX = RightCenter.X;
            ConstY = LeftCenter.Y;
        }
        else
        {
            ConstX = LeftCenter.X;
            ConstY = RightCenter.Y;
        }

        if (LeftCenter.Y < RightCenter.Y)
        {
            for (int X = LeftCenter.X; X <= RightCenter.X; X++)
            {
                int Idx = XYToIdx(X, ConstY, World->Width);
                if (GeneratedMap[Idx] == 0)
                {
                    GeneratedMap[Idx] = GEN_TILE_CORRIDOR;
                    World->Tiles[Idx] = TILE_STONE;
                    World->TilesInitialized[Idx] = true;
                }
            }

            for (int Y = LeftCenter.Y; Y <= RightCenter.Y; Y++)
            {
                int Idx = XYToIdx(ConstX, Y, World->Width);
                if (GeneratedMap[Idx] == 0)
                {
                    GeneratedMap[Idx] = GEN_TILE_CORRIDOR;
                    World->Tiles[Idx] = TILE_STONE;
                    World->TilesInitialized[Idx] = true;
                }
            }
        }
        else
        {
            for (int X = LeftCenter.X; X <= RightCenter.X; X++)
            {
                int Idx = XYToIdx(X, ConstY, World->Width);
                if (GeneratedMap[Idx] == 0)
                {
                    GeneratedMap[Idx] = GEN_TILE_CORRIDOR;
                    World->Tiles[Idx] = TILE_STONE;
                    World->TilesInitialized[Idx] = true;
                }
            }

            for (int Y = RightCenter.Y; Y <= LeftCenter.Y; Y++)
            {
                int Idx = XYToIdx(ConstX, Y, World->Width);
                if (GeneratedMap[Idx] == 0)
                {
                    GeneratedMap[Idx] = GEN_TILE_CORRIDOR;
                    World->Tiles[Idx] = TILE_STONE;
                    World->TilesInitialized[Idx] = true;
                }
            }
        }
    }

    // NOTE: Create walls around all ground tiles
    for (int TileI = 0; TileI < TileCount; TileI++)
    {
        if (GeneratedMap[TileI] == 0)
        {
            vec2i Current = IdxToXY(TileI, World->Width);
            b32 FoundRoomGround = false;
            for (int Dir = 0; Dir < 8; Dir++)
            {
                vec2i Neighbor = Current + DIRECTIONS[Dir];

                if (IsPInBounds(Neighbor, World))
                {
                    u8 GenTileType = GeneratedMap[XYToIdx(Neighbor, World->Width)];
                    if (GenTileType == GEN_TILE_FLOOR || GenTileType == GEN_TILE_CORRIDOR)
                    {
                        FoundRoomGround = true;
                        break;
                    }
                }
            }

            if (FoundRoomGround)
            {
                GeneratedMap[TileI] = GEN_TILE_WALL;
                World->Tiles[TileI] = TILE_STONE;
                World->TilesInitialized[TileI] = true;
            }
        }
    }

    // NOTE: Generate other structures based on room types
    {
        room *Room = GeneratedRooms;
        for (int i = 0; i < *RoomCount; i++, Room++)
        {
            switch (Room->Type)
            {
                case ROOM_ENTRANCE:
                {
                    vec2i P = GetCenterRoomP(Room);
                    GeneratedMap[XYToIdx(P, World->Width)] = GEN_TILE_STAIR_UP;
                    // TODO: Novice item drop close to the middle
                    // TODO: A low level enemy close to the middle
                } break;

                case ROOM_TEMPLE:
                {
                    int StatuesMax = 3;
                    int AttemptsMax = 10;
                    int Statues;
                    int Attempts;
                    for (Statues = 0, Attempts = 0; Statues < StatuesMax && Attempts < AttemptsMax; Attempts++)
                    {
                        vec2i P = GetRandomRoomP(Room, 1);
                        u8 *GenTileType = GeneratedMap + XYToIdx(P, World->Width);
                        if (*GenTileType == GEN_TILE_FLOOR)
                        {
                            *GenTileType = GEN_TILE_STATUE;
                            Statues++;
                        }
                    }
                } break;

                case ROOM_EXIT:
                {
                    vec2i P = GetCenterRoomP(Room);
                    GeneratedMap[XYToIdx(P, World->Width)] = GEN_TILE_STAIR_DOWN;
                    // TODO: Level boss
                } break;;

                default: break;
            }
        }
    }
}

#define GENERATED_MAP 0
#define ITEM_PICKUP_TEST 0
#define TEST_ENEMY 1

world *
GenerateWorld(int WorldW, int WorldH, int TilePxW, int TilePxH, memory_arena *ScratchArena)
{
#pragma region PREPARE WORLD STATE
    // TODO: The allocation size should depend on the world size
    // = MemoryArena_PushStructAndZero(WorldArena, world);
    memory_arena NewWorldArena = AllocArena(Megabytes(8));
    world *World = MemoryArena_PushStruct(&NewWorldArena, world);
    World->Arena = NewWorldArena;
    World->Width = WorldW;
    World->Height = WorldH;
    World->TilePxW = TilePxW;
    World->TilePxH = TilePxH;
    memory_arena *WorldArena = &World->Arena;

    World->TilesInitialized = MemoryArena_PushArrayAndZero(WorldArena, World->Width * World->Height, u8);
    World->Tiles = MemoryArena_PushArray(WorldArena, World->Width * World->Height, u8);
    World->DarknessLevels = MemoryArena_PushArray(WorldArena, World->Width * World->Height, u8);
    for (int i = 0; i < World->Width * World->Height; i++)
    {
        World->DarknessLevels[i] = DARKNESS_UNSEEN;
    }

    World->EntityUsedCount = 0;
    World->EntityMaxCount = ENTITY_MAX_COUNT;
    World->Entities = MemoryArena_PushArray(WorldArena, World->EntityMaxCount, entity);

    World->SpatialEntities = MemoryArena_PushArray(WorldArena, World->Width * World->Height, entity *);

    World->TurnQueueCount = 0;
    World->TurnQueueMax = World->EntityMaxCount;
    World->EntityTurnQueue = MemoryArena_PushArray(WorldArena, World->TurnQueueMax, entity_queue_node);
#pragma endregion

#pragma region GET TEMPLATES
    entity PumiceWall = Template_PumiceWall();
    entity Statues[] = {
        Template_LatenaStatue(),
        Template_XetelStatue(),
        Template_KirstStatue(),
        Template_ShlekStatue(),
        Template_TempestStatue(),
        Template_GadekaStatue()
    };
    entity StairsUp = Template_StairsUp();
    entity StairsDown = Template_StairsDown();
    entity Player = Template_Player();
    entity ItemPickupTemplate = Template_ItemPickup();
    entity TestEnemy = Template_TestEnemy();
    // entity AetherFly = Template_AetherFly();
    // entity EtherealMartyr = Template_EtherealMartyr();
    // entity FacelessSoul = Template_FacelessSoul();
    // entity Crane = Template_Crane();
#pragma endregion

#pragma region GENERATE MAP
#if (GENERATED_MAP == 1)
    
    u8 *GeneratedEntityMap = MemoryArena_PushArrayAndZero(ScratchArena, World->Width * World->Height, u8);
    int RoomsMax = 50;
    int RoomCount;
    room *GeneratedRooms = MemoryArena_PushArrayAndZero(ScratchArena, RoomsMax, room);
    GenerateRoomMap(World, GeneratedEntityMap, GeneratedRooms, RoomsMax, &RoomCount);
    MemoryArena_ResizePreviousPushArray(ScratchArena, RoomCount, room);

    vec2i PlayerP = Vec2I(GeneratedRooms[0].X + GeneratedRooms[0].W / 2, GeneratedRooms[0].Y + GeneratedRooms[0].H / 2);

    // entity W = GetTestEntityBlueprint(ENTITY_STATIC, '#', VA_WHITE);
    // entity G = GetTestEntityBlueprint(ENTITY_STATIC, '@', VA_BLACK);
    // for (int i = 0; i < World->Width * World->Height; i++)
    // {
    //     if (GeneratedMap[i] == 1)
    //     {
    //         AddEntity(World, IdxToXY(i, World->Width), &G);
    //     }
    //     if (GeneratedMap[i] == 2)
    //     {
    //         AddEntity(World, IdxToXY(i, World->Width), &W);
    //     }
    // }

    // NOTE: Add structures
    for (int WorldI = 0; WorldI < World->Width * World->Height; WorldI++)
    {
        switch (GeneratedEntityMap[WorldI])
        {
            case GEN_TILE_WALL:
            {
                AddEntity(World, IdxToXY(WorldI, World->Width), &PumiceWall);
            } break;

            case GEN_TILE_STATUE:
            {
                entity *Statue = &Statues[GetRandomValue(0, ArrayCount(Statues))];
                AddEntity(World, IdxToXY(WorldI, World->Width), Statue);
            } break;

            case GEN_TILE_STAIR_UP:
            {
                AddEntity(World, IdxToXY(WorldI, World->Width), &StairsUp);
            } break;

            case GEN_TILE_STAIR_DOWN:
            {
                AddEntity(World, IdxToXY(WorldI, World->Width), &StairsDown);
                World->Exit = IdxToXY(WorldI, World->Width);
            } break;

            default: break;
        }
    }
#else
    vec2i PlayerP = Vec2I(10, 10);

    for (int i = 0; i < World->Width * World->Height; i++)
    {
        World->TilesInitialized[i] = true;
        World->Tiles[i] = TILE_STONE;
    }

    for (int X = 0; X < World->Width; X++)
    {
        AddEntity(World, Vec2I(X, 0), &PumiceWall);
        AddEntity(World, Vec2I(X, World->Height - 1), &PumiceWall);
    }

    for (int Y = 1; Y < World->Height - 1; Y++)
    {
        AddEntity(World, Vec2I(0, Y), &PumiceWall);
        AddEntity(World, Vec2I(World->Width - 1, Y), &PumiceWall);
    }
#endif
#pragma endregion

#pragma region POST GEN ENTITIES
    World->PlayerEntity = AddEntity(World, PlayerP, &Player);

#if (ITEM_PICKUP_TEST == 1)
    entity *ItemPickupTest = AddEntity(World, World->PlayerEntity->Pos + Vec2I(0, -1), &ItemPickupTemplate);

    ItemPickupTest->Inventory[0] = Template_HaimaPotion();
    ItemPickupTest->Inventory[1] = Template_ShortBow();

    ItemPickupTest->Glyph = ItemPickupTest->Inventory[0].Glyph;
    ItemPickupTest->Color = ItemPickupTest->Inventory[0].Color;
    
    ItemPickupTest = AddEntity(World, World->PlayerEntity->Pos + Vec2I(0, -1), &ItemPickupTemplate);

    ItemPickupTest->Inventory[0] = Template_ShoddyPickaxe();
    ItemPickupTest->Inventory[1] = Template_Sword();
    ItemPickupTest->Inventory[2] = Template_LeatherCuirass();

    ItemPickupTest->Glyph = ItemPickupTest->Inventory[0].Glyph;
    ItemPickupTest->Color = ItemPickupTest->Inventory[0].Color;
#endif

#if (GENERATED_MAP == 1)
    int EnemyCount = 0;
    {
        room *Room = GeneratedRooms;
        int MaxAttempts = 100;
        World->SwarmCount = 1;
        int AddedToSwarm = 0;
        int MaxInSwarm = 7;
        for (int i = 0; i < RoomCount; i++, Room++)
        {
            if (Room->Type == ROOM_LARGE)
            {
                if (GetRandomValue(0, 2) == 0)
                {
                    // NOTE: Aether Fly room
                    int Attempt = 0;
                    int AetherFlyCount = 0;
                    int AetherFlyMaxCount = GetRandomValue(20, 30);
                    while (AetherFlyCount < AetherFlyMaxCount && Attempt < MaxAttempts)
                    {
                        int X = GetRandomValue(Room->X, Room->X + Room->W);
                        int Y = GetRandomValue(Room->Y, Room->Y + Room->H);
                        vec2i P = Vec2I(X, Y);
                        if (!CheckCollisions(World, P).Collided)
                        {
                            AetherFly.SwarmID = World->SwarmCount;
                            AddEntity(World, P, &AetherFly);
                            EnemyCount++;
                            AddedToSwarm++;
                            if (AddedToSwarm >= 7)
                            {
                                World->SwarmCount++;
                                AddedToSwarm = 0;
                            }
                        }
                        Attempt++;
                    }
                }
            }
            else if (Room->Type == ROOM_SMALL || Room->Type == ROOM_MEDIUM)
            {
                int Value = GetRandomValue(0, 3);
                entity *Template;
                switch (Value)
                {
                    default:
                    {
                        Template = &EtherealMartyr;
                    } break;

                    case 1:
                    {
                        Template = &FacelessSoul;
                    } break;

                    case 2:
                    {
                        Template = &Crane;
                    } break;
                }

                int Attempt = 0;
                int Count = 0;
                int MaxCount = GetRandomValue(1, 3);
                while (Count < MaxCount && Attempt < MaxAttempts)
                {
                    int X = GetRandomValue(Room->X, Room->X + Room->W);
                    int Y = GetRandomValue(Room->Y, Room->Y + Room->H);
                    vec2i P = Vec2I(X, Y);
                    if (!CheckCollisions(World, P).Collided)
                    {
                        AddEntity(World, P, Template);
                        Count++;
                        EnemyCount++;
                    }
                    Attempt++;
                }
            }
        }
    }
#elif (TEST_ENEMY == 1)
    int EnemyCount = 0;
    int AttemptCount = 0;
    int EnemiesToAdd = 1;
    int MaxAttempts = 500;
    while (EnemyCount < EnemiesToAdd && AttemptCount < MaxAttempts)
    {
        vec2i P = PlayerP + Vec2I(GetRandomValue(-5, 5), GetRandomValue(-5, 5));
        if (!CheckCollisions(World, P).Collided)
        {
            AddEntity(World, P, &TestEnemy);
            EnemyCount++;
        }
        AttemptCount++;
    }
#else
    int EnemyCount = 0;
    int AttemptCount = 0;
    int EnemiesToAdd = 50;
    int MaxAttempts = 500;
    while (EnemyCount < EnemiesToAdd && AttemptCount < MaxAttempts)
    {
        int X = GetRandomValue(0, World->Width);
        int Y = GetRandomValue(0, World->Height);

        vec2i P = Vec2I(X, Y);
        if (!CheckCollisions(World, P).Collided)
        {
            AddEntity(World, P, &TestEnemy);
            EnemyCount++;
        }
        AttemptCount++;
    }
#endif
#pragma endregion

#pragma region GENERATE GROUND SPLATS
    // TODO: Instead of hardcoding like this, calculate scale and dist between splats based on ground brush tex size
    f32 Scale = 10.0f;
    f32 DistBwSplatsX = 135.0f;
    f32 DistBwSplatsY = 135.0f;

    f32 WorldPxWidth = (f32) World->TilePxW * World->Width;
    f32 WorldPxHeight = (f32) World->TilePxH * World->Height;
    
    int GroundSplatXCount = (int) (WorldPxWidth / DistBwSplatsX) + 1;
    int GroundSplatYCount = (int) (WorldPxHeight / DistBwSplatsY) + 1;
    World->GroundSplatCount = GroundSplatXCount * GroundSplatYCount;
    World->GroundSplats = MemoryArena_PushArray(WorldArena, World->GroundSplatCount, ground_splat);
    
    {
        ground_splat *GroundSplat = World->GroundSplats;
        for (int SplatI = 0; SplatI < World->GroundSplatCount; SplatI++, GroundSplat++)
        {
            vec2i P = IdxToXY(SplatI, GroundSplatXCount);
            f32 PxX = P.X * DistBwSplatsX;
            f32 PxY = P.Y * DistBwSplatsY;
            f32 RndX = (GetRandomFloat() * 0.1f - 0.05f) * DistBwSplatsX;
            f32 RndY = (GetRandomFloat() * 0.1f - 0.05f) * DistBwSplatsY;
            GroundSplat->Position = Vec2(PxX + RndX, PxY + RndY);

            f32 SpriteRot = GetRandomFloat() * 360.0f - 180.0f;
            GroundSplat->Rotation = SpriteRot;

            GroundSplat->Scale = Scale;
        }
    }
#pragma endregion

#ifdef SAV_DEBUG
    // NOTE: Signed overflow so we know which entities got added post world gen :)
    World->EntityCurrentDebugID = INT_MAX;
    World->EntityCurrentDebugID++;
#endif

    TraceLog("Generated world. Added %d enemies.", EnemyCount);

    return World;
}

#include "sou_entity_behavior.cpp"
