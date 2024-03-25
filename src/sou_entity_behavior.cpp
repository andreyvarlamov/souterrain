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
