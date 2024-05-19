struct process_state_result
{
    npc_state NewState;
    b32 FinishedTurn;
    b32 TurnUsed;
};

static_i process_state_result
StateIdle(game_state *GameState, world *World, entity *Entity)
{
    process_state_result Result = {};
    
    if (LookAround(Entity, World))
    {
        LogEntityAction(Entity, World, "%s (%d): player is in FOV. Now hunting player", Entity->Name, Entity->DebugID);
        Result.NewState = NPC_STATE_HUNTING;
        Result.FinishedTurn = false;
        return Result;
    }

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
                
        MoveEntity(GameState->World, Entity, NewEntityP, ShouldAttack, &Result.TurnUsed);
    }

    Result.NewState = NPC_STATE_IDLE;
    Result.FinishedTurn = true;
    return Result;
}

static_i process_state_result
StateHunting(game_state *GameState, world *World, entity *Entity)
{
    process_state_result Result = {};
    
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
        LogEntityAction(Entity, World, "%s (%d): player is missing. Searching where last seen: (%d, %d)", Entity->Name, Entity->DebugID, Entity->Target.X, Entity->Target.Y);
        Result.NewState = NPC_STATE_SEARCHING;
        Result.FinishedTurn = false;
        return Result;
    }

    Entity->Target = World->PlayerEntity->Pos;

    // TODO: Extract the swarm logic
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
        MoveEntity(GameState->World, Entity, NewEntityP, ShouldAttack, &Result.TurnUsed);
        Result.NewState = NPC_STATE_HUNTING;
    }
    else
    {
        LogEntityAction(Entity, World, "%s (%d): cannot path to player (%d, %d). Now idle", Entity->Name, Entity->DebugID, World->PlayerEntity->Pos.X, World->PlayerEntity->Pos.Y);
        Result.NewState = NPC_STATE_IDLE;
    }

    Result.FinishedTurn = true;
    return Result;
}

static_i process_state_result
StateSearching(game_state *GameState, world *World, entity *Entity)
{
    process_state_result Result = {};
    
    if (LookAround(Entity, World))
    {
        LogEntityAction(Entity, World, "%s (%d): found player. Now hunting player", Entity->Name, Entity->DebugID);
        Result.FinishedTurn = false;
        Result.NewState = NPC_STATE_HUNTING;
        return Result;
    }
    else if (Entity->Pos == Entity->Target)
    {
        LogEntityAction(Entity, World, "%s (%d): no player in last known location. Now idle", Entity->Name, Entity->DebugID);
        Result.FinishedTurn = false;
        Result.NewState = NPC_STATE_IDLE;
        return Result;
    }
    else if (Entity->SearchTurns <= 0)
    {
        Result.FinishedTurn = false;
        Result.NewState = NPC_STATE_IDLE;
        return Result;
    }

    Entity->SearchTurns--;

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
        Result.NewState = NPC_STATE_IDLE;
    }
    else if (Path.FoundPath && Path.Path)
    {
        vec2i NewEntityP = Path.Path[0];
        MoveEntity(GameState->World, Entity, NewEntityP, false, &Result.TurnUsed);
        Result.NewState = NPC_STATE_SEARCHING;
        
    }
    else
    {
        LogEntityAction(Entity, World, "%s (%d): cannot path to target (%d, %d). Now idle", Entity->Name, Entity->DebugID, Entity->Target.X, Entity->Target.Y);
        Result.NewState = NPC_STATE_IDLE;
    }

    Result.FinishedTurn = true;
    return Result;
}

b32
UpdateNpc(game_state *GameState, world *World, entity *Entity)
{
    b32 FinishedTurn = false;
    b32 TurnUsed = false;
    while (!FinishedTurn)
    {
        switch (Entity->NpcState)
        {
            case NPC_STATE_IDLE:
            {
                process_state_result Result = StateIdle(GameState, World, Entity);
                Entity->NpcState = Result.NewState;
                FinishedTurn = Result.FinishedTurn;
                TurnUsed = Result.TurnUsed;
            } break;

            case NPC_STATE_HUNTING:
            {
                process_state_result Result = StateHunting(GameState, World, Entity);
                Entity->NpcState = Result.NewState;
                FinishedTurn = Result.FinishedTurn;
                TurnUsed = Result.TurnUsed;
            } break;

            case NPC_STATE_SEARCHING:
            {
                process_state_result Result = StateSearching(GameState, World, Entity);
                Entity->NpcState = Result.NewState;
                FinishedTurn = Result.FinishedTurn;
                TurnUsed = Result.TurnUsed;
            } break;

            default:
            {
                    InvalidCodePath;
            } break;
        }
    }
    
    // NOTE: NPCs always use turn, even if they decided not to act
    TurnUsed = true;
    return TurnUsed;
}
