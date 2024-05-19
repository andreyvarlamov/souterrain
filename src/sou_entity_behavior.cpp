enum { FIRST_LEVEL_XP_GAIN = 50 };

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
    }

    return TurnUsed;
}
