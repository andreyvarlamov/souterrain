#include "sav_lib.h"
#include "souterrain.h"

run_state
RunState_RangedAttack(game_state *GameState)
{
    world *World = GameState->World;
    entity *Player = World->PlayerEntity;
    game_input *GameInput = &GameState->GameInput;
    
    if (MouseWheel() != 0) CameraIncreaseLogZoomSteps(&GameState->Camera, MouseWheel());
    if (MouseDown(SDL_BUTTON_MIDDLE)) GameState->Camera.Target -= CameraScreenToWorldRel(&GameState->Camera, GetMouseRelPos());
            
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
            
    DrawGame(GameState, World);
    DrawRangedAttackDebugUI(GameState);

    BeginUIDraw(GameState);
    DrawDebugUI(GameState, Vec2I(0, 0));
    DrawPlayerStatsUI(GameState, Player, GameInput->MouseWorldPxP);
    DrawInspectUI(&GameState->InspectState, GameState);
    EndUIDraw();

    if (MouseReleased(SDL_BUTTON_LEFT))
    {
        if (IsInRangedAttackRange(World, Player->Pos, GameInput->MouseWorldTileP, Player->RangedRange))
        {
            entity *EntityToHit = GetEntitiesOfTypeAt(GameInput->MouseWorldTileP, ENTITY_NPC, World);
            if (EntityToHit)
            {
                EndInspect(&GameState->InspectState);
                GameState->PlayerReqAction.T = ACTION_ATTACK_ENTITY;
                GameState->PlayerReqAction.AttackEntity.Entity = EntityToHit;
                return RUN_STATE_IN_GAME;
            }
        }
    }

    if (KeyPressed(SDL_SCANCODE_ESCAPE) || KeyPressed(SDL_SCANCODE_F))
    {
        EndInspect(&GameState->InspectState);
        return RUN_STATE_IN_GAME;
    }

    return RUN_STATE_RANGED_ATTACK;
}
