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

    int Range;
    int Area;
    switch (GameState->RangedAttackType)
    {
        default:
        {
            return RUN_STATE_IN_GAME;
        } break;

        case RANGED_WEAPON:
        {
            Range = Player->RangedRange;
            Area = 1;
        } break;

        case RANGED_FIREBALL:
        {
            Range = Player->FireballRange;
            Area = Player->FireballArea;
        } break;

        case RANGED_RENDMIND:
        {
            Range = Player->RendMindRange;
            Area = 1;
        } break;
    }

    b32 MouseInRange = false;
    vec2i MouseP = GameInput->MouseWorldTileP;
    BeginTextureMode(GameState->DebugOverlay, Rect(0)); BeginCameraMode(&GameState->Camera); 
    {

        int StartX = Player->Pos.X - Range;
        int EndX = Player->Pos.X + Range;
        int StartY = Player->Pos.Y - Range;
        int EndY = Player->Pos.Y + Range;

        for (int Y = StartY; Y <= EndY; Y++)
        {
            for (int X = StartX; X <= EndX; X++)
            {
                vec2i P = Vec2I(X, Y);
                if (IsInRangedAttackRange(World, Player->Pos, P, Range))
                {
                    if (P == MouseP)
                    {
                        MouseInRange = true;
                    }
                    DrawRect(World, P, ColorAlpha(VA_RED, 150));
                }
            }
        }

        if (MouseInRange)
        {
            StartX = MouseP.X  - (Area - 1);
            EndX = MouseP.X + (Area - 1);
            StartY = MouseP.Y - (Area - 1);
            EndY = MouseP.Y + (Area - 1);

            for (int Y = StartY; Y <= EndY; Y++)
            {
                for (int X = StartX; X <= EndX; X++)
                {
                    vec2i P = Vec2I(X, Y);
                    DrawRect(World, P, ColorAlpha(VA_RED, 200));
                }
            }
        }
    }
    EndCameraMode(); EndTextureMode();

    BeginUIDraw(GameState);
    DrawDebugUI(GameState, Vec2I(0, 0));
    DrawPlayerStatsUI(GameState, Player, GameInput->MouseWorldPxP);
    DrawInspectUI(&GameState->InspectState, GameState);
    EndUIDraw();

    if (MouseReleased(SDL_BUTTON_LEFT) && MouseInRange)
    {
        if (Area != 1 || GetEntitiesOfTypeAt(MouseP, ENTITY_NPC, World) != NULL)
        {
            EndInspect(&GameState->InspectState);
            GameState->PlayerReqAction.T = ACTION_ATTACK_RANGED;
            GameState->PlayerReqAction.AttackRanged.Target = MouseP;
            GameState->PlayerReqAction.AttackRanged.Type = GameState->RangedAttackType;
            return RUN_STATE_IN_GAME;
        }
    }

    if (KeyPressed(SDL_SCANCODE_ESCAPE) || KeyPressed(SDL_SCANCODE_F) || KeyPressed(SDL_SCANCODE_1) || KeyPressed(SDL_SCANCODE_2))
    {
        EndInspect(&GameState->InspectState);
        return RUN_STATE_IN_GAME;
    }

    return RUN_STATE_RANGED_ATTACK;
}
