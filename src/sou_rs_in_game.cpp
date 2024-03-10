#include "sav_lib.h"
#include "souterrain.h"

run_state
RunState_InGame(game_state *GameState)
{
    world *World = GameState->World;
    entity *Player = World->PlayerEntity;
    game_input *GameInput = &GameState->GameInput;

    if (MouseWheel() != 0) CameraIncreaseLogZoomSteps(&GameState->Camera, MouseWheel());
    if (MouseDown(SDL_BUTTON_MIDDLE)) GameState->Camera.Target -= CameraScreenToWorldRel(&GameState->Camera, GetMouseRelPos());
    // if (KeyPressed(SDL_SCANCODE_F1))
    // {
    //     GameState->IgnoreFieldOfView = !GameState->IgnoreFieldOfView;
    //     ProcessPlayerFOV(World, GameState->IgnoreFieldOfView);
    // }
    
    run_state NewRunState = RUN_STATE_IN_GAME;
    
    i64 StartTurn = World->CurrentTurn;
            
    entity *ActiveEntity = EntityTurnQueuePeek(World);
    if (ActiveEntity == Player)
    {
        ProcessPlayerInputs(&GameState->PlayerReqAction);
        b32 TurnUsed = UpdatePlayer(Player, World, &GameState->Camera, &GameState->PlayerReqAction, GameState, &NewRunState);
        if (TurnUsed)
        {
            World->CurrentTurn += EntityTurnQueuePopAndReinsert(World, ActiveEntity->ActionCost);
            EntityRegen(Player, World);
            ActiveEntity = EntityTurnQueuePeek(World);
            ProcessPlayerFOV(World, GameState->IgnoreFieldOfView);
        }
        if (Player->GainedLevel)
        {
            NewRunState = RUN_STATE_LEVELUP_MENU;
            Player->GainedLevel = false;
        }
        ResetPlayerInputs(&GameState->PlayerReqAction);
    }

    if (NewRunState == RUN_STATE_IN_GAME)
    {
        b32 FirstEntity = true;
        entity *StartingEntity = ActiveEntity;
        while (ActiveEntity != Player && (FirstEntity || ActiveEntity != StartingEntity))
        {
            switch (ActiveEntity->Type)
            {
                case ENTITY_NPC:
                {
                    b32 TurnUsed = UpdateNpc(GameState, World, ActiveEntity);
                    if (TurnUsed)
                    {
                        World->CurrentTurn += EntityTurnQueuePopAndReinsert(World, ActiveEntity->ActionCost);
                        EntityRegen(ActiveEntity, World);
                    }
                } break;

                default:
                {
                    EntityTurnQueuePopWithoutConsumingCost(World);
                } break;
            }

            ActiveEntity = EntityTurnQueuePeek(World);

            FirstEntity = false;
        }

        i64 TurnsPassed = World->CurrentTurn - StartTurn;

        ProcessSwarms(World, TurnsPassed);
 
        ProcessMinedWalls(World);

        DeleteDeadEntities(World);

        if (MouseReleased(SDL_BUTTON_RIGHT))
        {
            if (IsInFOV(World, Player->FieldOfView, GameInput->MouseWorldTileP))
            {
                entity *EntityToInspect = GetEntitiesAt(GameInput->MouseWorldTileP, World);
                if (EntityToInspect)
                {
                    const char *Action;
                    switch (EntityToInspect->Type)
                    {
                        case ENTITY_NPC: Action = "TALK (Coming soon!)"; break;
                        case ENTITY_STATUE: Action = "PRAY (Coming soon!)"; break;
                        default: Action = NULL; break;
                    }
                    StartInspectEntity(&GameState->InspectState, EntityToInspect, Action);
                }
                else
                {
                    EndInspect(&GameState->InspectState);
                }
            }
            else
            {
                EndInspect(&GameState->InspectState);
            }
        }
    }
    else
    {
        EndInspect(&GameState->InspectState);
    }
    
    DrawGame(GameState, World);

    BeginUIDraw(GameState);
    DrawDebugUI(GameState, GameInput->MouseWorldTileP);
    DrawPlayerStatsUI(GameState, Player, GameInput->MouseWorldPxP);
    
    DrawInspectUI(&GameState->InspectState, GameState);

    if (KeyPressed(SDL_SCANCODE_ESCAPE))
    {
        EndInspect(&GameState->InspectState);
    }
    
    EndUIDraw();

    return NewRunState;
}
