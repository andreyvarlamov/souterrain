#include "souterrain.h"

#include <cstring> // memset

// SECTION: ENTITY AND SPATIAL

entity *
GetEntitiesAt(vec2i P, world *World)
{
    if(IsPValid(P, World))
    {
        int WorldI = XYToIdx(P, World->Width);
        return World->SpatialEntities[WorldI];
    }

    return NULL;
}

entity *
GetEntitiesOfTypeAt(vec2i P, entity_type EntityType, world *World)
{
    entity *Entity = GetEntitiesAt(P, World);

    while (Entity)
    {
        if (Entity->Type == EntityType)
        {
            break;
        }

        Entity = Entity->Next;
    }

    return Entity;
}

void
GetAllEntitiesOfType(entity_type EntityType, world *World, memory_arena *TrArena, entity ***Entities, int *Count)
{
    *Entities = MemoryArena_PushArray(TrArena, World->EntityUsedCount, entity *);
    *Count = 0;
    
    for (int i = 0; i < World->EntityUsedCount; i++)
    {
        entity *Entity = World->Entities + i;
        if (Entity->Type == EntityType)
        {
            (*Entities)[(*Count)++] = Entity;
        }
    }
    
    MemoryArena_ResizePreviousPushArray(TrArena, *Count, entity *);
}

collision_info
CheckCollisions(world *World, vec2i P, b32 IgnoreNPCs = false)
{
    collision_info CI;
    
    if (IsPValid(P, World))
    {
        entity *HeadEntity = GetEntitiesAt(P, World);

        b32 FoundBlocking = false;

        entity *Entity;
        for (Entity = HeadEntity; Entity; Entity = Entity->Next)
        {
            // NOTE: Right now the assumption is that only one entity is blocking per tile
            // (Otherwise how did an entity move to a blocked tile?)
            // if (EntityExists(Entity) && Entity->Pos == P && CheckFlags(Entity->Flags, ENTITY_IS_BLOCKING))
            if (EntityExists(Entity) && Entity->Pos == P)
            {
                if (CheckFlags(Entity->Flags, ENTITY_IS_BLOCKING) && (!IgnoreNPCs || Entity->Type != ENTITY_NPC))
                {
                    FoundBlocking = true;
                    break;
                }
                else
                {
                    Noop;
                }
            }
        }

        CI.Collided = FoundBlocking;
        CI.Entity = Entity;
    }
    else
    {
        CI.Collided = true;
        CI.Entity = NULL;
        return CI;
    }

    return CI;
}

b32
IsTileOpaque(world *World, vec2i P)
{
    if (IsPValid(P, World))
    {
        entity *HeadEntity = GetEntitiesAt(P, World);

        b32 FoundOpaque = false;

        entity *Entity;
        for (Entity = HeadEntity; Entity; Entity = Entity->Next)
        {
            if (EntityExists(Entity) && Entity->Pos == P)
            {
                if (CheckFlags(Entity->Flags, ENTITY_IS_OPAQUE))
                {
                    FoundOpaque = true;
                    break;
                }
            }
        }

        return FoundOpaque;
    }
    else
    {
        return true;
    }
}

void
AddEntityToSpatial(world *World, vec2i Pos, entity *Entity)
{
    int WorldI = XYToIdx(Pos, World->Width);

    Assert(IsPValid(Pos, World));

    entity *HeadEntity = World->SpatialEntities[WorldI];
    Entity->Next = HeadEntity;

    World->SpatialEntities[WorldI] = Entity;
}

void
RemoveEntityFromSpatial(world *World, vec2i Pos, entity *Entity)
{
    int WorldI = XYToIdx(Pos, World->Width);

    entity *HeadEntity = World->SpatialEntities[WorldI];

    Assert(HeadEntity);

    entity *PrevEntity = NULL;
    entity *SearchEntity = HeadEntity;
    while (SearchEntity)
    {
        if (SearchEntity == Entity)
        {
            break;
        }

        PrevEntity = SearchEntity;
        SearchEntity = SearchEntity->Next;
    }

    Assert(SearchEntity);
        
    if (PrevEntity)
    {
        PrevEntity->Next = Entity->Next;
    }
    else
    {
        World->SpatialEntities[WorldI] = Entity->Next;
    }

    Entity->Next = NULL;
}

entity *
FindNextFreeEntitySlot(world *World)
{
    // NOTE: This will hit even if there are spaces based on tight count
    Assert(World->EntityUsedCount < World->EntityMaxCount);

    // TODO: Handle more than a given amount of entities. Need a bucket array or something like that.
    // TODO: Do the assert or allocate another bucket only if entities are tightly packed and it's out of space
    entity *Entity = World->Entities + World->EntityTightCount;
    while (Entity->Type > 0)
    {
        World->EntityTightCount++;
        Entity++;
    }

    World->EntityTightCount++;
    entity *NextNextFreeEntity = Entity + 1;
    while (NextNextFreeEntity->Type > 0)
    {
        World->EntityTightCount++;
        NextNextFreeEntity++;
    }

    if (World->EntityTightCount > World->EntityUsedCount)
    {
        World->EntityUsedCount = World->EntityTightCount;
    }

    return Entity;
}

void EntityTurnQueueInsert(world *World, entity *Entity, int NewCostOwed);

enum { ENTITY_HEALTH_DEFAULT = 100 };

entity *
AddEntity(world *World, vec2i Pos, entity *CopyEntity)
{
    entity *Entity = FindNextFreeEntitySlot(World);

    *Entity = *CopyEntity;
    
    b32 NeedFOV = Entity->Type == ENTITY_PLAYER;
    if (NeedFOV && Entity->FieldOfView == NULL)
    {
        // TODO: Allocate only for the entity max range rect
        Entity->FieldOfView = MemoryArena_PushArray(&World->Arena, World->Width * World->Height, u8);
    }

    b32 NeedInventory = Entity->Type == ENTITY_PLAYER || Entity->Type == ENTITY_ITEM_PICKUP;
    if (NeedInventory)
    {
        if (Entity->Inventory == NULL)
        {
            Entity->Inventory = MemoryArena_PushArrayAndZero(&World->Arena, INVENTORY_SLOTS_PER_ENTITY, item);
        }
        else
        {
            memset(Entity->Inventory, 0, INVENTORY_SLOTS_PER_ENTITY * sizeof(item));
        }
    }
    
    Entity->Pos = Pos;
    Entity->DebugID = World->EntityCurrentDebugID++;
    if (Entity->MaxHealth == 0)
    {
        Entity->Health = Entity->MaxHealth = ENTITY_HEALTH_DEFAULT;
    }
    if (Entity->Level == 0)
    {
        Entity->Level = 1;
    }

    AddEntityToSpatial(World, Pos, Entity);

    if (Entity->ActionCost > 0)
    {
        EntityTurnQueueInsert(World, Entity, 0);
    }

    return Entity;
}

void
DeleteEntity(world *World, entity *Entity)
{
    RemoveEntityFromSpatial(World, Entity->Pos, Entity);

    Entity->Type = ENTITY_NONE;
    
    int EntityI = (int) (Entity - World->Entities);
    Assert(EntityI < World->EntityUsedCount);
    
    if (EntityI < World->EntityTightCount)
    {
        World->EntityTightCount = EntityI;
    }
}

b32
ValidateEntitySpatialPartition(world *World)
{
    for (int i = 0; i < World->Width * World->Height; i++)
    {
        entity *HeadEntity = World->SpatialEntities[i];

        for (entity *Entity = HeadEntity; Entity; Entity = Entity->Next)
        {
            Assert(EntityExists(Entity));
        }
    }

    return true;
}

vec2i
GetRandomRoomP(room *Room, int Pad)
{
    int X = GetRandomValue(Room->X+Pad, Room->X+Room->W-Pad);
    int Y = GetRandomValue(Room->Y+Pad, Room->Y+Room->H-Pad);

    return Vec2I(X, Y);
}

vec2i
GetCenterRoomP(room *Room)
{
    int X = Room->X + Room->W/2;
    int Y = Room->Y + Room->H/2;

    return Vec2I(X, Y);
}

// SECTION: WORLD GEN
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

void
RemoveItemEffectsFromEntity(item *Item, entity *Entity);

void
ApplyItemEffectsToEntity(item *Item, entity *Entity);

void
CopyEntityInventory(entity *From, entity *To)
{
    item *ToInventoryItem = To->Inventory;
    item *FromInventoryItem = From->Inventory;
    for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ToInventoryItem++, FromInventoryItem++)
    {
        *ToInventoryItem = *FromInventoryItem;
        // if (ToInventoryItem->ItemType != ITEM_NONE)
        // {
        //     RemoveItemEffectsFromEntity(ToInventoryItem, To);
        //     ToInventoryItem->ItemType = ITEM_NONE;
        // }

        // if (FromInventoryItem->ItemType != ITEM_NONE)
        // {
        //     *ToInventoryItem = *FromInventoryItem;
        //     ApplyItemEffectsToEntity(ToInventoryItem, To);
        // }
    }
}

void
CopyEntity(entity *From, entity *To, world *ToWorld)
{
    To->Type = From->Type;
    To->Flags = From->Flags;
    
    To->Glyph = From->Glyph;
    To->Color = From->Color;

    To->ActionCost = From->ActionCost;

    To->ViewRange = From->ViewRange;
    To->RangedRange = From->RangedRange;

    Assert(To->FieldOfView || !From->FieldOfView);
    // NOTE: No need to copy of fov as it's world specific

    To->NpcState = From->NpcState;
    To->Target = From->Target;

    To->Name = From->Name;
    To->Description = From->Description;

    To->Health = From->Health;
    To->MaxHealth = From->MaxHealth;
    To->ArmorClass = From->ArmorClass;

    To->Mana = From->Mana;
    To->MaxMana = From->MaxMana;

    // To->LastHealTurn = From->LastHealTurn;
    To->RegenActionCost = From->RegenActionCost;
    To->RegenAmount = From->RegenAmount;

    To->Haima = From->Haima;
    To->Kitrina = From->Kitrina;
    To->Melana = From->Melana;
    To->Sera = From->Sera;

    To->HaimaBonus = From->HaimaBonus;

    To->Armor = From->Armor;
    To->Damage = From->Damage;
    To->RangedDamage = From->RangedDamage;

    To->RangedRange = From->RangedRange;
    To->FireballDamage = From->FireballDamage;
    To->FireballRange = From->FireballRange;
    To->FireballArea = From->FireballArea;
    To->RendMindDamage = From->RendMindDamage;
    To->RendMindRange = From->RendMindRange;

    To->XP = From->XP;
    To->Level = From->Level;


    Assert(To->Inventory || !From->Inventory);
    CopyEntityInventory(From, To);
}

#define GENERATED_MAP 1

world *
GenerateWorld(int WorldW, int WorldH, int TilePxW, int TilePxH, memory_arena *ScratchArena)
{
    // SECTION: PREPARE WORLD STATE
    
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

    // SECTION: GENERATE MAP
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

    // SECTION: ADD ENTITIES
    entity Player = Template_Player();
    World->PlayerEntity = AddEntity(World, PlayerP, &Player);

    entity ItemPickupTemplate = Template_ItemPickup();
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

    entity AetherFly = Template_AetherFly();
    entity EtherealMartyr = Template_EtherealMartyr();
    entity FacelessSoul = Template_FacelessSoul();
    entity Crane = Template_Crane();
    
#if 0
    int EnemyCount = 0;
    int AttemptCount = 0;
    int EnemiesToAdd = 150;
    int MaxAttempts = 500;
    while (EnemyCount < EnemiesToAdd && AttemptCount < MaxAttempts)
    {
        int X = GetRandomValue(0, World->Width);
        int Y = GetRandomValue(0, World->Height);

        vec2i P = Vec2I(X, Y);
        if (!CheckCollisions(World, P).Collided)
        {
            AddEntity(World, P, &AetherFly);
            EnemyCount++;
        }
        AttemptCount++;
    }
#elif 0
    int EnemyCount = 0;
    int AttemptCount = 0;
    int EnemiesToAdd = 0;
    int MaxAttempts = 500;
    while (EnemyCount < EnemiesToAdd && AttemptCount < MaxAttempts)
    {
        int X = GetRandomValue(World->PlayerEntity->Pos.X - 10, World->PlayerEntity->Pos.X + 10);
        int Y = GetRandomValue(World->PlayerEntity->Pos.Y - 10, World->PlayerEntity->Pos.Y + 10);

        vec2i P = Vec2I(X, Y);
        if (!CheckCollisions(World, P).Collided)
        {
            AddEntity(World, P, &AetherFly);
            EnemyCount++;
        }
        AttemptCount++;
    }
#else
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
#endif

#ifdef SAV_DEBUG
    // NOTE: Signed overflow so we know which entities got added post world gen :)
    World->EntityCurrentDebugID = INT_MAX;
    World->EntityCurrentDebugID++;
#endif

    // SECTION: GENERATE GROUND SPLATS
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

    TraceLog("Generated world. Added %d enemies.", EnemyCount);

    return World;
}

// SECTION: PATHFINDING
f32
GetHeuristic(vec2i Start, vec2i End)
{
    #if 1
    // NOTE: Sq Dist. Really unadmissible. Fast but very unoptimal path.
    f32 dX = (f32) (End.X - Start.X);
    f32 dY = (f32) (End.Y - Start.Y);
    return dX*dX + dY*dY;
    #elif 0
    // NOTE: Manhattan. Slightly unadmissible. Fast ish, somewhat optimal path.
    return AbsF((f32) ((End.X - Start.X) + (End.Y - Start.Y)));
    #else
    // NOTE: Euclidian distance. Admissible. Slow calculation, way more iterations and guaranteed optimal path.
    f32 dX = (f32) (End.X - Start.X);
    f32 dY = (f32) (End.Y - Start.Y);
    return SqrtF(dX*dX + dY*dY);
    #endif
}

b32
PopLowestScoreFromOpenSet(path_state *PathState, int *LowestScoreIdx)
{
    // NOTE: One of the first optimizations that could be done here is to convert this to a min heap
    if (PathState->OpenSetCount > 0)
    {
        f32 LowestScore = FLT_MAX;
        int LowestScoreOpenSetI = 0;
        for (int OpenSetI = 0; OpenSetI < PathState->OpenSetCount; OpenSetI++)
        {
            int Idx = PathState->OpenSet[OpenSetI];

            if (PathState->FScores[Idx] < LowestScore)
            {
                LowestScore = PathState->FScores[Idx];
                LowestScoreOpenSetI = OpenSetI;
                *LowestScoreIdx = Idx;
            }
        }

        for (int OpenSetI = LowestScoreOpenSetI; OpenSetI < PathState->OpenSetCount - 1; OpenSetI++)
        {
            PathState->OpenSet[OpenSetI] = PathState->OpenSet[OpenSetI + 1];
        }
        PathState->OpenSetCount--;
        return true;
    }

    return false;
}

b32
IsInOpenSet(path_state *PathState, int Idx)
{
    for (int i = 0; i < PathState->OpenSetCount; i++)
    {
        if (PathState->OpenSet[i] == Idx)
        {
            return true;
        }
    }

    return false;
}

// static_g int CalculatePathCalls = 0;
// static_g int OpenSetMax = 0;

// void ResetPathCallCount() { CalculatePathCalls = 0; OpenSetMax = 0; }
// int GetPathCallCount() { return CalculatePathCalls; }
// int GetPathOpenSetMax() { return OpenSetMax; }

void
AddToOpenSet(path_state *PathState, int Idx)
{
    if (!IsInOpenSet(PathState, Idx))
    {
        PathState->OpenSet[PathState->OpenSetCount++] = Idx;
        
        // if (PathState->OpenSetCount > OpenSetMax)
        // {
        //     OpenSetMax = PathState->OpenSetCount;
        // }
    }
}

void
GetNeighbors(vec2i Pos, world *World,
             vec2i *Neighbors, b32 *DiagonalNeighbors, int *NeighborCount)
{
    *NeighborCount = 0;
    
    for (int Dir = 0; Dir < 8; Dir++)
    {
        vec2i Neighbor = Pos + DIRECTIONS[Dir];

        if (Neighbor.X >= 0 && Neighbor.X < World->Width &&
            Neighbor.Y >= 0 && Neighbor.Y < World->Height)
        {
            DiagonalNeighbors[*NeighborCount] = (Dir % 2 == 1);
            Neighbors[*NeighborCount] = Neighbor;
            (*NeighborCount)++;
        }
    }
}

#undef VIZ

path_result
CalculatePath(world *World, vec2i Start, vec2i End, memory_arena *TrArena, memory_arena *ResultArena, int MaxIterations = 0xFFFFFFFF, b32 IgnoreNPCs = false, int VizGenMax = 0)
{
    // CalculatePathCalls++;
    
    int VizGen = 0;
    path_result Result = {};

    MemoryArena_Freeze(TrArena);

#ifdef VIZ
    if (VizGen >= VizGenMax)
    {
        goto routine_end;
    }
    VizGen++;
#endif

    path_state PathState;
    PathState.OpenSet = MemoryArena_PushArray(TrArena, OPEN_SET_MAX, int);
    PathState.OpenSetCount = 0;

    PathState.MapSize = World->Width * World->Height;
    // NOTE: For unbounded maps, or just big maps, these could be hash tables
    PathState.CameFrom = MemoryArena_PushArray(TrArena, PathState.MapSize, int);
    PathState.GScores = MemoryArena_PushArray(TrArena, PathState.MapSize, f32);
    PathState.FScores = MemoryArena_PushArray(TrArena, PathState.MapSize, f32);
    for (int i = 0; i < PathState.MapSize; i++)
    {
        PathState.GScores[i] = FLT_MAX;
        PathState.FScores[i] = FLT_MAX;
    }

    int StartIdx = XYToIdx(Start, World->Width);
    PathState.OpenSet[PathState.OpenSetCount++] = StartIdx;
    PathState.CameFrom[StartIdx] = 0;
    PathState.GScores[StartIdx] = 0;
    PathState.FScores[StartIdx] = GetHeuristic(Start, End);

    int EndIdx = XYToIdx(End, World->Width);

    if (StartIdx == EndIdx)
    {
        Result.FoundPath = true;
        MemoryArena_Unfreeze(TrArena);
        return Result;
    }

    b32 FoundPath = false;
    int CurrentIdx;
    int IterationCount = 0;
    while (PopLowestScoreFromOpenSet(&PathState, &CurrentIdx))
    {
        if (CurrentIdx == EndIdx)
        {
            FoundPath = true;
            break;
        }

        vec2i Neighbors[8];
        b32 DiagonalNeighbors[8];
        int NeighborCount;
        GetNeighbors(IdxToXY(CurrentIdx, World->Width), World, Neighbors, DiagonalNeighbors, &NeighborCount);

        for (int i = 0; i < NeighborCount; i++)
        {
            vec2i Neighbor = Neighbors[i];
            b32 Diagonal = DiagonalNeighbors[i];
            f32 ThisGScore = PathState.GScores[CurrentIdx] + (Diagonal ? 1.414f : 1.0f);
            
            int NeighborIdx = XYToIdx(Neighbor, World->Width);
            if (ThisGScore < PathState.GScores[NeighborIdx])
            {
                // TODO: This can be optimized by caching results of collisions.
                // Right now if a cell gets rejected because there is a collision, it will recheck collisions again
                // from another currentIdx position.
                // In addition, if a cell has already been discovered with a higher GScore, we know that there was no collision,
                // but this is gonna check collisions for that cell again.
                b32 Collided = (NeighborIdx == EndIdx) ? false : CheckCollisions(World, Neighbor, IgnoreNPCs).Collided;
                if (!Collided)
                {
                    PathState.CameFrom[NeighborIdx] = CurrentIdx;
                    PathState.GScores[NeighborIdx] = ThisGScore;
                    PathState.FScores[NeighborIdx] = ThisGScore + GetHeuristic(Neighbor, End);

                    AddToOpenSet(&PathState, NeighborIdx);
                }
            }
        }

#ifdef VIZ
        if (VizGen >= VizGenMax)
        {
            int I = 0;
            int PrevIdx = CurrentIdx;
            while (PrevIdx != StartIdx && I < PATH_MAX)
            {
                DrawRect(World, IdxToXY(PrevIdx, World->Width), I == 0 ? ColorAlpha(VA_YELLOW, 200) : ColorAlpha(VA_BLUE, 100));
                I++;
                PrevIdx = PathState.CameFrom[PrevIdx];
            }
            DrawRect(World, Start, ColorAlpha(VA_RED, 200));

            goto routine_end;
        }
        VizGen++;
#endif

        if (IterationCount++ > MaxIterations) break;
    }

    Result.FoundPath = FoundPath;
    
    if (FoundPath)
    {
        Result.Path = MemoryArena_PushArray(ResultArena, PATH_MAX, vec2i);

        int Count = 0;
        // NOTE: Reconstitute path backwards
        int PrevIdx = EndIdx;
        while (PrevIdx != StartIdx && Count < PATH_MAX)
        {
            Result.Path[Count++] = IdxToXY(PrevIdx, World->Width);
            PrevIdx = PathState.CameFrom[PrevIdx];
        }

        Assert(Count < PATH_MAX);

        // NOTE: And then reverse it
        for (int i = 0; i < Count / 2; i++)
        {
            vec2i Temp = Result.Path[i];
            Result.Path[i] = Result.Path[Count - 1  - i];
            Result.Path[Count - 1  - i] = Temp;
        }
        Result.PathSteps = Count;
        MemoryArena_ResizePreviousPushArray(ResultArena, Count, vec2i);
    }

#ifdef VIZ
 routine_end:
#endif
    MemoryArena_Unfreeze(TrArena);

    return Result;
}

// SECTION: LINE OF SIGHT
void
TraceLineBresenham(world *World, vec2i A, vec2i B, u8 *VisibilityMap, int MaxRangeSq)
{
    int X1 = A.X;
    int Y1 = A.Y;
    int X2 = B.X;
    int Y2 = B.Y;
    
    int DeltaX = X2 - X1;
    int IX = ((DeltaX > 0) - (DeltaX < 0));
    DeltaX = Abs(DeltaX) << 1;

    int DeltaY = Y2 - Y1;
    int IY = ((DeltaY > 0) - (DeltaY < 0));
    DeltaY = Abs(DeltaY) << 1;

    VisibilityMap[XYToIdx(X1, Y1, World->Width)] = 1;

    if (DeltaX >= DeltaY)
    {
        int Error = (DeltaY - (DeltaX >> 1));

        while (X1 != X2)
        {
            if ((Error > 0) || (!Error && (IX > 0)))
            {
                Error -= DeltaX;
                Y1 += IY;
            }

            Error += DeltaY;
            X1 += IX;

            VisibilityMap[XYToIdx(X1, Y1, World->Width)] = 1;

            if ((X1 - A.X)*(X1 - A.X) + (Y1 - A.Y)*(Y1 - A.Y) >= MaxRangeSq)
            {
                break;
            }

            if (IsTileOpaque(World, Vec2I(X1, Y1)))
            {
                break;
            }
        }
    }
    else
    {
        int Error = (DeltaX - (DeltaY >> 1));

        while (Y1 != Y2)
        {
            if ((Error > 0) || (!Error && (IY > 0)))
            {
                Error -= DeltaY;
                X1 += IX;
            }

            Error += DeltaX;
            Y1 += IY;

            VisibilityMap[XYToIdx(X1, Y1, World->Width)] = 1;

            if ((X1 - A.X)*(X1 - A.X) + (Y1 - A.Y)*(Y1 - A.Y) >= MaxRangeSq)
            {
                break;
            }
            
            if (IsTileOpaque(World, Vec2I(X1, Y1)))
            {
                break;
            }
        }
    }

}

void
CalculateFOV(world *World, vec2i Pos, u8 *VisibilityMap, int MaxRange)
{
    int MaxRangeSq = MaxRange*MaxRange;
    
    for (int X = 0; X < World->Width; X++)
    {
        TraceLineBresenham(World, Pos, Vec2I(X, 0), VisibilityMap, MaxRangeSq);
        TraceLineBresenham(World, Pos, Vec2I(X, World->Height - 1), VisibilityMap, MaxRangeSq);
    }

    for (int Y = 0; Y < World->Height; Y++)
    {
        TraceLineBresenham(World, Pos, Vec2I(0, Y), VisibilityMap, MaxRangeSq);
        TraceLineBresenham(World, Pos, Vec2I(World->Width - 1, Y), VisibilityMap, MaxRangeSq);
    }
}

void
CalculateExhaustiveFOV(world *World, vec2i Pos, u8 *VisibilityMap, int MaxRange)
{
    int MaxRangeSq = MaxRange*MaxRange;

    int StartX = Max(Pos.X - MaxRange, 0);
    int EndX = Min(Pos.X + MaxRange, World->Width - 1);
    int StartY = Max(Pos.Y - MaxRange, 0);
    int EndY = Min(Pos.Y + MaxRange, World->Height - 1);
    
    for (int Y = StartY; Y <= EndY; Y++)
    {
        for (int X = StartX; X <= EndX; X++)
        {
            if (!VisibilityMap[XYToIdx(X, Y, World->Width)])
            {
                TraceLineBresenham(World, Pos, Vec2I(X, Y), VisibilityMap, MaxRangeSq);
            }
        }
    }
}

b32
IsInLineOfSight(world *World, vec2i Start, vec2i End, int MaxRange)
{
    if (VecLengthSq(End - Start) <= MaxRange*MaxRange)
    {
        int CurrentX = Start.X;
        int CurrentY = Start.Y;
        int EndX = End.X;
        int EndY = End.Y;
    
        int DeltaX = EndX - CurrentX;
        int IX = ((DeltaX > 0) - (DeltaX < 0));
        DeltaX = Abs(DeltaX) << 1;

        int DeltaY = EndY - CurrentY;
        int IY = ((DeltaY > 0) - (DeltaY < 0));
        DeltaY = Abs(DeltaY) << 1;

        if (DeltaX >= DeltaY)
        {
            int Error = (DeltaY - (DeltaX >> 1));
            while (CurrentX != EndX)
            {
                if ((Error > 0) || (!Error && (IX > 0)))
                {
                    Error -= DeltaX;
                    CurrentY += IY;
                }

                Error += DeltaY;
                CurrentX += IX;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                if (TestPos == End) return true;
                if (IsTileOpaque(World, TestPos)) break;
            }
        }
        else
        {
            int Error = (DeltaX - (DeltaY >> 1));
            while (CurrentY != EndY)
            {
                if ((Error > 0) || (!Error && (IY > 0)))
                {
                    Error -= DeltaY;
                    CurrentX += IX;
                }

                Error += DeltaX;
                CurrentY += IY;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                if (TestPos == End) return true;
                if (IsTileOpaque(World, TestPos)) break;
            }
        }
    }
    
    return false;
}

b32
IsInRangedAttackRange(world *World, vec2i Start, vec2i End, int MaxRange)
{
    if (VecLengthSq(End - Start) <= MaxRange*MaxRange)
    {
        int CurrentX = Start.X;
        int CurrentY = Start.Y;
        int EndX = End.X;
        int EndY = End.Y;
    
        int DeltaX = EndX - CurrentX;
        int IX = ((DeltaX > 0) - (DeltaX < 0));
        DeltaX = Abs(DeltaX) << 1;

        int DeltaY = EndY - CurrentY;
        int IY = ((DeltaY > 0) - (DeltaY < 0));
        DeltaY = Abs(DeltaY) << 1;

        if (DeltaX >= DeltaY)
        {
            int Error = (DeltaY - (DeltaX >> 1));
            while (CurrentX != EndX)
            {
                if ((Error > 0) || (!Error && (IX > 0)))
                {
                    Error -= DeltaX;
                    CurrentY += IY;
                }

                Error += DeltaY;
                CurrentX += IX;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                collision_info Col = CheckCollisions(World, TestPos);
                
                // NOTE: If encountered non npc collision - it can't be hit in ranged, even if it's our target
                if (IsTileOpaque(World, TestPos) ||
                    ((Col.Collided && Col.Entity == NULL) ||
                     (Col.Entity && Col.Entity->Type != ENTITY_NPC)))
                {
                    return false;
                }

                if (TestPos == End) return true;
                
                // NOTE: If encountered npc collision - it can be hit in ranged,
                //       so if it was the target we would've returned true,
                //       but if it's not, don't continue tracing line beyond that
                if (Col.Entity && Col.Entity->Type == ENTITY_NPC) return false;
            }
        }
        else
        {
            int Error = (DeltaX - (DeltaY >> 1));
            while (CurrentY != EndY)
            {
                if ((Error > 0) || (!Error && (IY > 0)))
                {
                    Error -= DeltaY;
                    CurrentX += IX;
                }

                Error += DeltaX;
                CurrentY += IY;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                collision_info Col = CheckCollisions(World, TestPos);

                // NOTE: If encountered non npc collision - it can't be hit in ranged, even if it's our target
                if (IsTileOpaque(World, TestPos) ||
                    ((Col.Collided && Col.Entity == NULL) ||
                     (Col.Entity && Col.Entity->Type != ENTITY_NPC)))
                {
                    return false;
                }

                if (TestPos == End) return true;
                
                // NOTE: If encountered npc collision - it can be hit in ranged,
                //       so if it was the target we would've returned true,
                //       but if it's not, don't continue tracing line beyond that
                if (Col.Entity && Col.Entity->Type == ENTITY_NPC) return false;
            }
        }
    }

    return false;
}

// SECTION: ENTITY TURN QUEUE
inline entity_queue_node
MakeEntityQueueNode(entity *Entity, int Cost)
{
    entity_queue_node Node;
    Node.Entity = Entity;
    Node.LeftoverCost = Cost;
    return Node;
}

inline entity *
EntityTurnQueuePeek(world *World)
{
    return World->EntityTurnQueue->Entity;
}

entity *
EntityTurnQueuePopWithoutConsumingCost(world *World)
{
    entity_queue_node *TopNode = World->EntityTurnQueue;
    entity *TopEntity = TopNode->Entity;

    for (int I = 0; I < World->TurnQueueCount - 1; I++)
    {
        World->EntityTurnQueue[I] = World->EntityTurnQueue[I + 1];
    }
    World->TurnQueueCount--;

    return TopEntity;
}

entity *
EntityTurnQueuePop(world *World)
{
    entity_queue_node *TopNode = World->EntityTurnQueue;

    entity *TopEntity = TopNode->Entity;
    int CostToConsume = TopNode->LeftoverCost;

    for (int I = 0; I < World->TurnQueueCount - 1; I++)
    {
        World->EntityTurnQueue[I] = World->EntityTurnQueue[I + 1];
        World->EntityTurnQueue[I].LeftoverCost -= CostToConsume;
    }
    World->TurnQueueCount--;

    return TopEntity;
}

void
EntityTurnQueueInsert(world *World, entity *Entity, int NewCostOwed)
{
    Assert(World->TurnQueueCount < World->TurnQueueMax);
    int InsertI;
    for (InsertI = World->TurnQueueCount - 1; InsertI >= 0; InsertI--)
    {
        if (World->EntityTurnQueue[InsertI].LeftoverCost <= NewCostOwed)
        {
            break;
        }
    }
    InsertI++;

    for (int ShiftI = World->TurnQueueCount - 1; ShiftI >= InsertI; ShiftI--)
    {
        World->EntityTurnQueue[ShiftI + 1] = World->EntityTurnQueue[ShiftI];
    }
    World->TurnQueueCount++;

    Assert(InsertI >= 0 && InsertI < World->TurnQueueMax);

    World->EntityTurnQueue[InsertI] = MakeEntityQueueNode(Entity, NewCostOwed);
}

int
EntityTurnQueuePopAndReinsert(world *World, int NewCostOwed)
{
    entity *Entity = World->EntityTurnQueue->Entity;
    int CostToConsume = World->EntityTurnQueue->LeftoverCost;
    
    int InsertI;
    for (InsertI = World->TurnQueueCount - 1; InsertI > 0; InsertI--)
    {
        World->EntityTurnQueue[InsertI].LeftoverCost -= CostToConsume;
        if (World->EntityTurnQueue[InsertI].LeftoverCost <= NewCostOwed)
        {
            // NOTE: Set cost back up, because it will be subtracted again in the next for loop
            World->EntityTurnQueue[InsertI].LeftoverCost += CostToConsume;
            break;
        }
    }

    for (int I = 0; I < InsertI; I++)
    {
        World->EntityTurnQueue[I] = World->EntityTurnQueue[I + 1];
        World->EntityTurnQueue[I].LeftoverCost -= CostToConsume;
    }

    Assert(InsertI >= 0 && InsertI < World->TurnQueueMax);

    World->EntityTurnQueue[InsertI] = MakeEntityQueueNode(Entity, NewCostOwed);

    return CostToConsume;
}

void
EntityTurnQueueDelete(world *World, entity *Entity)
{
    int ToDeleteI;
    for (ToDeleteI = 0; ToDeleteI < World->TurnQueueCount; ToDeleteI++)
    {
        if (World->EntityTurnQueue[ToDeleteI].Entity == Entity)
        {
            break;
        }
    }

    for (int ShiftI = ToDeleteI; ShiftI < World->TurnQueueCount - 1; ShiftI++)
    {
        World->EntityTurnQueue[ShiftI] = World->EntityTurnQueue[ShiftI + 1];
    }
    World->TurnQueueCount--;
}

// SECTION: ENTITY BEHAVIOR
enum { FIRST_LEVEL_XP_GAIN = 50 };

int
GetXPNeededForLevel(int Level)
{
    int XPNeeded = 50;
    for (int i = 1; i < Level; i++)
    {
        XPNeeded *= 2;
    }
    return XPNeeded;
}

void
EntityXPGain(entity *Entity, int XP)
{
    Entity->XP += XP;
    if (Entity->XP >= GetXPNeededForLevel(Entity->Level + 1))
    {
        Entity->Level++;
        Entity->GainedLevel = true;
    }
}

b32
DamageEntity(entity *Entity, int Damage)
{
    Entity->Health -= Damage;
    return Entity->Health <= 0;
}

void
EntityAttacksMelee(entity *Attacker, entity *Defender, world *World)
{
    int AttackRoll = RollDice(1, 20);

    b32 AttackConnects = AttackRoll + Attacker->Kitrina > Defender->ArmorClass;

    b32 AttackMisses = false;
    if (AttackConnects)
    {
        int DamageValue = RollDice(1, Attacker->Damage) + Max(0, (Attacker->Haima - 5) / 2);
        if (DamageValue > 0)
        {
            b32 Killed = DamageEntity(Defender, DamageValue);
            if (Killed)
            {
                if (Attacker == World->PlayerEntity)
                {
                    EntityXPGain(Attacker, Defender->XPGain);
                }
            
                LogEntityAction(Attacker, World,
                                "%s (%d) hits %s (%d) for %d damage (%d + %d > %d), killing them.",
                                Attacker->Name, Attacker->DebugID,
                                Defender->Name, Defender->DebugID,
                                DamageValue, AttackRoll, Attacker->Kitrina, Defender->ArmorClass);
            }
            else
            {
                LogEntityAction(Attacker, World,
                                "%s (%d) hits %s (%d) for %d damage (%d + %d > %d). Health: %d.",
                                Attacker->Name, Attacker->DebugID,
                                Defender->Name, Defender->DebugID,
                                DamageValue, AttackRoll, Attacker->Kitrina, Defender->ArmorClass,
                                Defender->Health);
            }
        }
        else
        {
            AttackMisses = true;
        }
    }
    else
    {
        AttackMisses = true;
    }

    if (AttackMisses)
    {
        LogEntityAction(Attacker, World,
                        "%s (%d) misses %s (%d) (%d + %d <= %d).",
                        Attacker->Name, Attacker->DebugID,
                        Defender->Name, Defender->DebugID,
                        AttackRoll, Attacker->Kitrina, Defender->ArmorClass);
    }
}

b32
EntityAttacksRanged(entity *Attacker, vec2i Target, ranged_attack_type RangedAttackType, world *World)
{
    switch (RangedAttackType)
    {
        case RANGED_WEAPON:
        {
            entity *EntityToHit = GetEntitiesOfTypeAt(Target, ENTITY_NPC, World);
            if (EntityToHit)
            {
                int AttackRoll = RollDice(1, 20);
                b32 AttackConnects = AttackRoll + Attacker->Kitrina > EntityToHit->ArmorClass;
                b32 AttackMisses = false;
                if (AttackConnects)
                {
                    int DamageValue = RollDice(1, Attacker->RangedDamage) + Max(0, Attacker->Kitrina - 5) / 2;
                    if (DamageValue > 0)
                    {
                        b32 Killed = DamageEntity(EntityToHit, DamageValue);
                        if (Killed)
                        {
                            if (Attacker == World->PlayerEntity)
                            {
                                EntityXPGain(Attacker, EntityToHit->XPGain);
                            }
                            
                            LogEntityAction(Attacker, World,
                                            "%s (%d) shoots %s (%d) for %d damage, killing them.",
                                            Attacker->Name, Attacker->DebugID,
                                            EntityToHit->Name, EntityToHit->DebugID,
                                            DamageValue);
                        }
                        else
                        {
                            LogEntityAction(Attacker, World,
                                            "%s (%d) shoots %s (%d) for %d damage. Health: %d.",
                                            Attacker->Name, Attacker->DebugID,
                                            EntityToHit->Name, EntityToHit->DebugID,
                                            DamageValue, EntityToHit->Health);
                        }
                    }
                    else
                    {
                        AttackMisses = true;
                    }
                }
                else
                {
                    AttackMisses = true;
                }

                if (AttackMisses)
                {
                    LogEntityAction(Attacker, World,
                                    "%s (%d) tries to shoot %s (%d), but misses.",
                                    Attacker->Name, Attacker->DebugID,
                                    EntityToHit->Name, EntityToHit->DebugID);
                }
                return true;
            }
            else
            {
                return false;
            }
        } break;

        case RANGED_FIREBALL:
        {
            int Area = Attacker->FireballArea;
            int StartX = Target.X  - (Area - 1);
            int EndX = Target.X + (Area - 1);
            int StartY = Target.Y - (Area - 1);
            int EndY = Target.Y + (Area - 1);

            int FireballCost = 2;
            if (Attacker->Mana >= FireballCost)
            {
                Attacker->Mana -= FireballCost;
            }
            else
            {
                LogEntityAction(Attacker, World, "Not enough mana to shoot a fireball!");
                return false;
            }

            int AttackRoll = RollDice(1, 20);
            b32 AttackConnects = AttackRoll + Attacker->Melana > 10;
            int DamageValue = RollDice(1, Attacker->FireballDamage) + Max(0, Attacker->Melana - 5) / 2;
            
            if (AttackConnects && DamageValue > 0)
            {
                for (int Y = StartY; Y <= EndY; Y++)
                {
                    for (int X = StartX; X <= EndX; X++)
                    {
                        vec2i P = Vec2I(X, Y);
                        entity *EntityToHit = GetEntitiesOfTypeAt(P, ENTITY_NPC, World);
                        if (EntityToHit)
                        {
                            b32 Killed = DamageEntity(EntityToHit, DamageValue);
                            if (Killed)
                            {
                                if (Attacker == World->PlayerEntity)
                                {
                                    EntityXPGain(Attacker, EntityToHit->XPGain);
                                }
                            
                                LogEntityAction(Attacker, World,
                                                "%s (%d) shoots fireball at %s (%d) for %d damage, killing them.",
                                                Attacker->Name, Attacker->DebugID,
                                                EntityToHit->Name, EntityToHit->DebugID,
                                                DamageValue);
                            }
                            else
                            {
                                LogEntityAction(Attacker, World,
                                                "%s (%d) shoots fireball at %s (%d) for %d damage. Health: %d.",
                                                Attacker->Name, Attacker->DebugID,
                                                EntityToHit->Name, EntityToHit->DebugID,
                                                DamageValue, EntityToHit->Health);
                            }
                        }
                    }
                }
            }
            else
            {
                LogEntityAction(Attacker, World,
                                "%s (%d) tries to shoot fireball, but misses.",
                                Attacker->Name, Attacker->DebugID);
            }
            return true;
        } break;

        case RANGED_RENDMIND:
        {
            entity *EntityToHit = GetEntitiesOfTypeAt(Target, ENTITY_NPC, World);
            if (EntityToHit)
            {
                int AttackRoll = RollDice(1, 20);
                b32 AttackConnects = AttackRoll + Attacker->Sera > EntityToHit->ArmorClass;
                b32 AttackMisses = false;
                if (AttackConnects)
                {
                    int DamageValue = RollDice(1, Attacker->RendMindDamage) + Max(0, Attacker->Sera - 5) / 2;
                    if (DamageValue > 0)
                    {
                        b32 Killed = DamageEntity(EntityToHit, DamageValue);
                        if (Killed)
                        {
                            if (Attacker == World->PlayerEntity)
                            {
                                EntityXPGain(Attacker, EntityToHit->XPGain);
                            }
                            
                            LogEntityAction(Attacker, World,
                                            "%s (%d) rends the mind of %s (%d) for %d damage, killing them.",
                                            Attacker->Name, Attacker->DebugID,
                                            EntityToHit->Name, EntityToHit->DebugID,
                                            DamageValue);
                        }
                        else
                        {
                            LogEntityAction(Attacker, World,
                                            "%s (%d) rends the mind of %s (%d) for %d damage. Health: %d.",
                                            Attacker->Name, Attacker->DebugID,
                                            EntityToHit->Name, EntityToHit->DebugID,
                                            DamageValue, EntityToHit->Health);
                        }
                    }
                    else
                    {
                        AttackMisses = true;
                    }
                }
                else
                {
                    AttackMisses = true;
                }

                if (AttackMisses)
                {
                    LogEntityAction(Attacker, World,
                                    "%s (%d) tries to rend the mind of %s (%d), but fails.",
                                    Attacker->Name, Attacker->DebugID,
                                    EntityToHit->Name, EntityToHit->DebugID);
                }
                return true;
            }
            else
            {
                return false;
            }
        } break;

        default: return false;
    }
}

b32
EntityAttacksWall(entity *Entity, entity *Wall, world *World)
{
    item *Pickaxe = IsItemTypeInEntityInventory(ITEM_PICKAXE, Entity);
    if (Pickaxe)
    {
        Wall->Health -= Pickaxe->WallDamage;
        if (Wall->Health > 0)
        {
            LogEntityAction(Entity, World,
                            "%s (%d) hits %s (%d) with a %s. Wall still has %d/%d health.",
                            Entity->Name, Entity->DebugID,
                            Wall->Name, Wall->DebugID,
                            Pickaxe->Name, Wall->Health, Wall->MaxHealth);
        }
        else
        {
            LogEntityAction(Entity, World,
                            "%s (%d) hits %s (%d) with a %s, breaking the wall.",
                            Entity->Name, Entity->DebugID,
                            Wall->Name, Wall->DebugID,
                            Pickaxe->Name);
        }

        return true;
    }

    return false;
}

b32
ResolveEntityCollision(entity *ActiveEntity, entity *PassiveEntity, world *World, b32 ShouldAttack)
{
    switch(ActiveEntity->Type)
    {
        case ENTITY_PLAYER:
        case ENTITY_NPC:
        {
            switch (PassiveEntity->Type)
            {
                case ENTITY_PLAYER:
                case ENTITY_NPC:
                {
                    if (ShouldAttack)
                    {
                        EntityAttacksMelee(ActiveEntity, PassiveEntity, World);
                        return true;
                    }
                    return false;
                } break;

                case ENTITY_WALL:
                {
                    if (ShouldAttack)
                    {
                        return EntityAttacksWall(ActiveEntity, PassiveEntity, World);
                    }
                    return false;
                } break;

                default: break;
            }
        };
        
        default: break;
    }
    
    return false;
}

b32
MoveEntity(world *World, entity *Entity, vec2i NewP, b32 ShouldAttack, b32 *Out_TurnUsed)
{
    *Out_TurnUsed = false;
    
    Assert(Entity->Type > 0);
    
    collision_info Col = CheckCollisions(World, NewP);

    if (Col.Collided)
    {
        if (Col.Entity)
        {
            *Out_TurnUsed = ResolveEntityCollision(Entity, Col.Entity, World, ShouldAttack);
        }

        return false;
    }
    else
    {
        RemoveEntityFromSpatial(World, Entity->Pos, Entity);
        AddEntityToSpatial(World, NewP, Entity);

        Entity->Pos = NewP;
        
#if 0
        LogEntityAction(Entity, World, "%s (%d) moves without hitting anyone", Entity->Name, Entity->DebugID);
#endif
        
        *Out_TurnUsed = true;
        return true;
    }
}

b32
LookAround(entity *Entity, world *World)
{
    return EntityExists(World->PlayerEntity) && IsInLineOfSight(World, Entity->Pos, World->PlayerEntity->Pos, Entity->ViewRange);
}

void
EntityRegen(entity *Entity, world *World)
{
    if (Entity->RegenActionCost > 0)
    {
        b32 WillHeal = GetRandomValue(0, 2) == 0;
        i64 TurnsSinceLastHeal = World->CurrentTurn - World->PlayerEntity->LastHealTurn;
        if (WillHeal && TurnsSinceLastHeal > Entity->RegenActionCost && Entity->Health < Entity->MaxHealth)
        {
            int RegenAmount = RollDice(1, Entity->RegenAmount);
            Entity->Health += RegenAmount;
            if (Entity->Health > Entity->MaxHealth)
            {
                Entity->Health = Entity->MaxHealth;
            }

            Entity->Mana += 1;
            if (Entity->Mana > Entity->MaxMana)
            {
                Entity->Mana = Entity->MaxMana;
            }
                        
            Entity->LastHealTurn = World->CurrentTurn;
            // LogEntityAction(Entity, World, "%s (%d) regens %d health.", Entity->Name, Entity->DebugID, RegenAmount);
        }
    }
}

b32
UpdateNpc(game_state *GameState, world *World, entity *Entity)
{
    // NOTE: Pre-update
    switch (Entity->NpcState)
    {
        case NPC_STATE_IDLE:
        {
            if (LookAround(Entity, World))
            {
                Entity->NpcState = NPC_STATE_HUNTING;
                LogEntityAction(Entity, World, "%s (%d): player is in FOV. Now hunting player", Entity->Name, Entity->DebugID);
            }
        } break;

        case NPC_STATE_HUNTING:
        {
            if (!LookAround(Entity, World))
            {
                // NOTE: The player position at this point is already the position that the entity hasn't seen,
                //       but to help with bad fov around corners, give entity one turn of "clairvoyance".
                //       Check if new player pos is in line of sight of old target, to prevent entity knowing where player moved,
                //       if he teleported.
                if (IsInLineOfSight(World, Entity->Target, World->PlayerEntity->Pos, Entity->ViewRange))
                {
                    Entity->Target = World->PlayerEntity->Pos;
                }

                Entity->SearchTurns = 10;
                Entity->NpcState = NPC_STATE_SEARCHING;
                LogEntityAction(Entity, World, "%s (%d): player is missing. Searching where last seen: (%d, %d)", Entity->Name, Entity->DebugID, Entity->Target.X, Entity->Target.Y);
            }
            else
            {
                Entity->Target = World->PlayerEntity->Pos;
            }
            
        } break;

        case NPC_STATE_SEARCHING:
        {
            if (LookAround(Entity, World))
            {
                Entity->NpcState = NPC_STATE_HUNTING;
                LogEntityAction(Entity, World, "%s (%d): found player. Now hunting player", Entity->Name, Entity->DebugID);
            }
            else if (Entity->Pos == Entity->Target)
            {
                Entity->NpcState = NPC_STATE_IDLE;
                LogEntityAction(Entity, World, "%s (%d): no player in last known location. Now idle", Entity->Name, Entity->DebugID);
            }
            else if (Entity->SearchTurns <= 0)
            {
                Entity->NpcState = NPC_STATE_IDLE;
            }
            else
            {
                Entity->SearchTurns--;
            }
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }

    // NOTE: Update
    b32 TurnUsed = false;
    switch (Entity->NpcState)
    {
        case NPC_STATE_IDLE:
        {
            int ShouldMove = GetRandomValue(0, 12);
            if (ShouldMove >= 10)
            {
                int RandDir = GetRandomValue(0, 4);
                vec2i NewEntityP = Entity->Pos;
                switch (RandDir) {
                    case 0: {
                        NewEntityP += Vec2I(0, -1);
                    } break;
                    case 1:
                    {
                        NewEntityP += Vec2I(1, 0);
                    } break;
                    case 2:
                    {
                        NewEntityP += Vec2I(0, 1);
                    } break;
                    case 3:
                    {
                        NewEntityP += Vec2I(-1, 0);
                    } break;
                    default: break;
                }

                int ShouldAttack = GetRandomValue(0, 2);
                
                MoveEntity(GameState->World, Entity, NewEntityP, ShouldAttack, &TurnUsed);
            }
        } break;

        case NPC_STATE_HUNTING:
        {
            Entity->Target = World->PlayerEntity->Pos;

            if (Entity->SwarmID > 0 && Entity->SwarmID < World->SwarmCount && World->Swarms[Entity->SwarmID].Cooldown <= 0)
            {
                entity *WorldEntity = World->Entities;
                for (int i = 0; i < World->EntityUsedCount; i++, WorldEntity++)
                {
                    if (WorldEntity->Type != ENTITY_NONE &&
                        WorldEntity->SwarmID == Entity->SwarmID &&
                        WorldEntity->NpcState == NPC_STATE_IDLE &&
                        GetRandomValue(0, 10)  < 8)
                    {
                        WorldEntity->SearchTurns = 10;
                        WorldEntity->NpcState = NPC_STATE_SEARCHING;
                        WorldEntity->Target = Entity->Target;
                    }
                }

                World->Swarms[Entity->SwarmID].Cooldown = 1000;
            }
            
            // NOTE: First check path taking other npcs into account, if path not found, check if there's one when ignoring other npcs
            path_result Path = CalculatePath(World,
                                             Entity->Pos, World->PlayerEntity->Pos,
                                             &GameState->ScratchArenaA, &GameState->ScratchArenaB,
                                             512);

            // NOTE: TOO SLOW :(
            if (!Path.FoundPath)
            {
                Path = CalculatePath(World,
                                     Entity->Pos, Entity->Target,
                                     &GameState->ScratchArenaA, &GameState->ScratchArenaB,
                                     512, true);
            }
            
            if (Path.FoundPath && Path.Path)
            {
                vec2i NewEntityP = Path.Path[0];
                b32 ShouldAttack = NewEntityP == World->PlayerEntity->Pos;
                MoveEntity(GameState->World, Entity, NewEntityP, ShouldAttack, &TurnUsed);
            }
            else
            {
                Entity->NpcState = NPC_STATE_IDLE;
                LogEntityAction(Entity, World, "%s (%d): cannot path to player (%d, %d). Now idle", Entity->Name, Entity->DebugID, World->PlayerEntity->Pos.X, World->PlayerEntity->Pos.Y);
            }
        } break;

        case NPC_STATE_SEARCHING:
        {
            // NOTE: First check path avoiding npcs, if path not found, check if there's one when ignoring other npcs
            path_result Path = CalculatePath(World,
                                             Entity->Pos, Entity->Target,
                                             &GameState->ScratchArenaA, &GameState->ScratchArenaB,
                                             512);

            int MaxWillingToGo = 20;
            if (!Path.FoundPath || Path.PathSteps > MaxWillingToGo)
            {
                Path = CalculatePath(World,
                                     Entity->Pos, Entity->Target,
                                     &GameState->ScratchArenaA, &GameState->ScratchArenaB,
                                     512, true);
            }

            if (Path.FoundPath && Path.PathSteps > MaxWillingToGo)
            {
                Entity->NpcState = NPC_STATE_IDLE;
            }
            else if (Path.FoundPath && Path.Path)
            {
                vec2i NewEntityP = Path.Path[0];
                MoveEntity(GameState->World, Entity, NewEntityP, false, &TurnUsed);
            }
            else
            {
                Entity->NpcState = NPC_STATE_IDLE;
                LogEntityAction(Entity, World, "%s (%d): cannot path to target (%d, %d). Now idle", Entity->Name, Entity->DebugID, Entity->Target.X, Entity->Target.Y);
            }
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }

    // NOTE: NPCs always use turn rn, even if they decided not to act
    TurnUsed = true;
    
    return TurnUsed;
}

b32
MovePlayer(world *World, entity *Player, vec2i NewP, camera_2d *Camera)
{
    vec2i OldP = Player->Pos;
    b32 TurnUsed = false;
    if (MoveEntity(World, Player, NewP, true, &TurnUsed))
    {
        if (IsPositionInCameraView(OldP, Camera, World))
        {
            Camera->Target += GetPxPFromTileP(World, Player->Pos) - GetPxPFromTileP(World, OldP);
        }
        else
        {
            Camera->Target = GetPxPFromTileP(World, Player->Pos);
        }
    }
    return TurnUsed;
}


void
RemoveItemEffectsFromEntity(item *Item, entity *Entity)
{
    if (Entity->Type == ENTITY_PLAYER || Entity->Type == ENTITY_NPC)
    {
        Entity->Haima -= Item->HaimaBonus;
        SetEntityStatsBasedOnAttributes(Entity);
    }
}

void
ApplyItemEffectsToEntity(item *Item, entity *Entity)
{
    if (Entity->Type == ENTITY_PLAYER || Entity->Type == ENTITY_NPC)
    {
        Entity->Haima += Item->HaimaBonus;
        if (Item->AC > 0) Entity->Armor = Item->AC;
        if (Item->Damage > 0) Entity->Damage = Item->Damage;
        if (Item->RangedDamage > 0) Entity->RangedDamage = Item->RangedDamage;
        SetEntityStatsBasedOnAttributes(Entity);
    }
}

b32
AddItemToEntityInventory(item *Item, entity *Entity)
{
    if (Entity->Inventory)
    {
        item *EntityItemSlot = Entity->Inventory;
        for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, EntityItemSlot++)
        {
            if (EntityItemSlot->ItemType == ITEM_NONE)
            {
                *EntityItemSlot = *Item;
                ApplyItemEffectsToEntity(Item, Entity);
                return true;
            }
        }
    }

    return false;
}

void
RemoveItemFromEntityInventory(item *Item, entity *Entity)
{
    if (Entity->Inventory)
    {
        item *EntityItemSlot = Entity->Inventory;
        for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, EntityItemSlot++)
        {
            if (EntityItemSlot == Item)
            {
                RemoveItemEffectsFromEntity(EntityItemSlot, Entity);
                EntityItemSlot->ItemType = ITEM_NONE;
            }
        }
    }
}

void
RefreshItemPickupState(entity *ItemPickup)
{
    int I;
    for (I = 0; I < INVENTORY_SLOTS_PER_ENTITY; I++)
    {
        if (ItemPickup->Inventory[I].ItemType > ITEM_NONE)
        {
            break;
        }
    }

    if (I < INVENTORY_SLOTS_PER_ENTITY)
    {
        ItemPickup->Glyph = ItemPickup->Inventory[I].Glyph;
        ItemPickup->Color = ItemPickup->Inventory[I].Color;
    }
    else
    {
        ItemPickup->Health = 0;
    }
}

b32
UpdatePlayer(entity *Player, world *World, camera_2d *Camera, req_action *Action, game_state *GameState, run_state *Out_NewRunState)
{
    b32 TurnUsed = false;
    switch (Action->T)
    {
        default: break;
                    
        case ACTION_MOVE:
        {
            TurnUsed = MovePlayer(World, Player, Player->Pos + Action->DP, Camera);
        } break;

        case ACTION_TELEPORT:
        {
            vec2i NewP;
            int Iter = 0;
            int MaxIters = 10;
            b32 FoundPosition = true;
            do
            {
                NewP = Vec2I(GetRandomValue(0, World->Width), GetRandomValue(0, World->Height));

                if (Iter++ >= MaxIters)
                {
                    FoundPosition = false;
                    break;
                }
            }
            while (CheckCollisions(World, NewP).Collided);

            if (FoundPosition)
            {
                TurnUsed = MovePlayer(World, Player, NewP, Camera);
            }
        } break;

        case ACTION_SKIP_TURN:
        {
            TurnUsed = true;
        } break;

        case ACTION_NEXT_LEVEL:
        {
            if (GetEntitiesOfTypeAt(Player->Pos, ENTITY_STAIR_DOWN, World))
            {
                if (GameState->CurrentWorld < MAX_WORLDS - 1)
                {
                    GameState->CurrentWorld++;
                    *Out_NewRunState = RUN_STATE_LOAD_WORLD;
                }
                else
                {
                    TraceLog("The way down is caved in.");
                }
            }
        } break;

        case ACTION_PREV_LEVEL:
        {
            if (GetEntitiesOfTypeAt(Player->Pos, ENTITY_STAIR_UP, World))
            {
                if (GameState->CurrentWorld > 0)
                {
                    GameState->CurrentWorld--;
                    *Out_NewRunState = RUN_STATE_LOAD_WORLD;
                }
                else
                {
                    TraceLog("The way up is caved in.");
                }
            }
        } break;

        case ACTION_OPEN_INVENTORY:
        {
            *Out_NewRunState = RUN_STATE_INVENTORY_MENU;
        } break;

        case ACTION_OPEN_PICKUP:
        {
            if (GetEntitiesOfTypeAt(Player->Pos, ENTITY_ITEM_PICKUP, World) != NULL)
            {
                *Out_NewRunState = RUN_STATE_PICKUP_MENU;
            }
        } break;

        case ACTION_DROP_ALL_ITEMS:
        {
            if (Player->Inventory)
            {
                entity ItemPickupTemplate = Template_ItemPickup();
                entity *ItemPickup = AddEntity(World, Player->Pos, &ItemPickupTemplate);
                Assert(ItemPickup->Inventory);

                item *ItemFromInventory = Player->Inventory;
                for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ItemFromInventory++)
                {
                    if (ItemFromInventory->ItemType > ITEM_NONE)
                    {
                        if (AddItemToEntityInventory(ItemFromInventory, ItemPickup))
                        {
                            RemoveItemFromEntityInventory(ItemFromInventory, Player);
                            TurnUsed = true;
                        }
                    }
                }

                RefreshItemPickupState(ItemPickup);
            }
        } break;

        case ACTION_DROP_ITEMS:
        {
            entity ItemPickupTemplate = Template_ItemPickup();
            entity *ItemPickup = AddEntity(World, Player->Pos, &ItemPickupTemplate);
            Assert(ItemPickup->Inventory);

            req_action_drop_items *DropItems = &Action->DropItems;

            for (int ItemI = 0; ItemI < DropItems->ItemCount; ItemI++)
            {
                if (AddItemToEntityInventory(DropItems->Items[ItemI], ItemPickup))
                {
                    RemoveItemFromEntityInventory(DropItems->Items[ItemI], Player);
                    TurnUsed = true;
                }
            }

            RefreshItemPickupState(ItemPickup);
        } break;

        case ACTION_PICKUP_ALL_ITEMS:
        {
            entity *ItemPickup = GetEntitiesAt(Player->Pos, World);
            while (ItemPickup)
            {
                if (ItemPickup->Type == ENTITY_ITEM_PICKUP)
                {
                    item *ItemFromPickup = ItemPickup->Inventory;
                    for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ItemFromPickup++)
                    {
                        if (ItemFromPickup->ItemType != ITEM_NONE)
                        {
                            if (AddItemToEntityInventory(ItemFromPickup, Player))
                            {
                                RemoveItemFromEntityInventory(ItemFromPickup, ItemPickup);
                                TurnUsed = true;
                            }
                        }
                    }

                    RefreshItemPickupState(ItemPickup);
                } 
                        
                ItemPickup = ItemPickup->Next;
            }
        } break;

        case ACTION_PICKUP_ITEMS:
        {
            req_action_pickup_items *PickupItems = &Action->PickupItems;
            
            for (int ItemI = 0; ItemI < PickupItems->ItemCount; ItemI++)
            {
                if (AddItemToEntityInventory(PickupItems->Items[ItemI], Player))
                {
                    RemoveItemFromEntityInventory(PickupItems->Items[ItemI], PickupItems->ItemPickups[ItemI]);
                    RefreshItemPickupState(PickupItems->ItemPickups[ItemI]);
                    TurnUsed = true;
                }
            }
        } break;

        case ACTION_START_RANGED_ATTACK:
        {
            GameState->RangedAttackType = RANGED_WEAPON;
            *Out_NewRunState = RUN_STATE_RANGED_ATTACK;
        } break;

        case ACTION_START_FIREBALL:
        {
            GameState->RangedAttackType = RANGED_FIREBALL;
            *Out_NewRunState = RUN_STATE_RANGED_ATTACK;
        } break;

        case ACTION_START_RENDMIND:
        {
            GameState->RangedAttackType = RANGED_RENDMIND;
            *Out_NewRunState = RUN_STATE_RANGED_ATTACK;
        } break;

        case ACTION_ATTACK_RANGED:
        {
            req_action_attack_ranged *AttackRanged = &Action->AttackRanged;
            TurnUsed = EntityAttacksRanged(Player, AttackRanged->Target, AttackRanged->Type, World);
        } break;
    }

    return TurnUsed;
}
