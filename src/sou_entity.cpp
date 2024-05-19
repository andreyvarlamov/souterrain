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
CheckCollisions(world *World, vec2i P, b32 IgnoreNPCs)
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

    Entity->Pos = Pos;
    Entity->DebugID = World->EntityCurrentDebugID++;

    if (Entity->Type == ENTITY_PLAYER || Entity->Type == ENTITY_NPC)
    {
        Entity->HealthData.Blood.Current = Entity->HealthData.Blood.Max = 100.0f;
        Entity->HealthData.Head.Current = Entity->HealthData.Head.Max = 100.0f;
        Entity->HealthData.Chest.Current = Entity->HealthData.Chest.Max = 100.0f;
        Entity->HealthData.Stomach.Current = Entity->HealthData.Stomach.Max = 100.0f;
        Entity->HealthData.LeftArm.Current = Entity->HealthData.LeftArm.Max = 100.0f;
        Entity->HealthData.RightArm.Current = Entity->HealthData.RightArm.Max = 100.0f;
        Entity->HealthData.LeftLeg.Current = Entity->HealthData.LeftLeg.Max = 100.0f;
        Entity->HealthData.RightLeg.Current = Entity->HealthData.RightLeg.Max = 100.0f;
    }

    AddEntityToSpatial(World, Pos, Entity);

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

void
CopyEntity(entity *From, entity *To, world *ToWorld)
{
    To->Type = From->Type;
    To->Flags = From->Flags;
    
    To->Glyph = From->Glyph;
    To->Color = From->Color;

    To->ViewRange = From->ViewRange;

    Assert(To->FieldOfView || !From->FieldOfView);
    // NOTE: No need to copy of fov as it's world specific

    To->NpcState = From->NpcState;
    To->Target = From->Target;

    To->Name = From->Name;
    To->Description = From->Description;
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
            // *Out_TurnUsed = ResolveEntityCollision(Entity, Col.Entity, World, ShouldAttack);
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

b32
UpdatePlayer(entity *Player, world *World, camera_2d *Camera, req_action *Action, game_state *GameState)
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
    }

    return TurnUsed;
}
