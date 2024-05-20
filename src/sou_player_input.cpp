#include "sou_player_input.h"

#include "sav_lib.h"
#include "sou_entity.h"

#include <sdl2/SDL_scancode.h>

void
ResetPlayerInputs(req_action *Action)
{
    *Action = {};
}

void
ProcessPlayerInputs(req_action *Action)
{
    if (Action->T != ACTION_NONE) return;
    
    if (KeyPressedOrRepeat(SDL_SCANCODE_Q)) { Action->T = ACTION_MOVE; Action->DP = Vec2I(-1, -1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_W)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 0, -1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_E)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 1, -1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_A)) { Action->T = ACTION_MOVE; Action->DP = Vec2I(-1,  0); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_S)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 0,  1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_D)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 1,  0); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_Z)) { Action->T = ACTION_MOVE; Action->DP = Vec2I(-1,  1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_C)) { Action->T = ACTION_MOVE; Action->DP = Vec2I( 1,  1); }
    else if (KeyPressedOrRepeat(SDL_SCANCODE_X)) Action->T = ACTION_SKIP_TURN;
}