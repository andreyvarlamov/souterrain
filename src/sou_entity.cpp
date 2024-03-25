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

void RemoveItemEffectsFromEntity(item *Item, entity *Entity);
void ApplyItemEffectsToEntity(item *Item, entity *Entity);

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
