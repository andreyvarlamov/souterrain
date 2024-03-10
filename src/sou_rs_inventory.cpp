#include "sav_lib.h"
#include "souterrain.h"

run_state
RunState_InventoryMenu(game_state *GameState)
{
    world *World = GameState->World;
    entity *Player = World->PlayerEntity;
    game_input *GameInput = &GameState->GameInput;
    
    DrawGame(GameState, World);

    BeginUIDraw(GameState);
    DrawDebugUI(GameState, Vec2I(0, 0));
    DrawPlayerStatsUI(GameState, Player, GameInput->MouseWorldPxP);
            
    f32 InventoryWidth = 500.0f;
    f32 InventoryHeight = 900.0f;
    rect InventoryRect = Rect(GameState->UiRenderTex.Texture.Width * 0.5f - InventoryWidth * 0.5f,
                              GameState->UiRenderTex.Texture.Height * 0.5f - InventoryHeight * 0.5f,
                              InventoryWidth,
                              InventoryHeight);
    DrawRect(InventoryRect, ColorAlpha(VA_BLACK, 240));

    f32 LineX = InventoryRect.X + 10.0f;
    f32 LineY = InventoryRect.Y + 10.0f;

    req_action *Action = &GameState->PlayerReqAction;
    req_action_drop_items *DropItems = &Action->DropItems;
    
    item *InventoryItem = Player->Inventory;
    for (int SlotI = 0; SlotI < INVENTORY_SLOTS_PER_ENTITY; SlotI++, InventoryItem++)
    {
        if (InventoryItem->ItemType != ITEM_NONE && !GameState->InventorySkipSlot[SlotI])
        {
            int Clicked = GuiButtonRect(Rect(LineX, LineY, InventoryWidth - 20.0f, 40.0f));
            switch (Clicked)
            {
                case SDL_BUTTON_LEFT:
                { 
                    StartInspectItem(&GameState->InspectState, InventoryItem, Player, SlotI, "DROP");
                } break;

                case SDL_BUTTON_RIGHT:
                {
                    Action->T = ACTION_DROP_ITEMS;
                    DropItems->Items[DropItems->ItemCount++] = InventoryItem;
                    GameState->InventorySkipSlot[SlotI] = true;
                    EndInspect(&GameState->InspectState);
                } break;

                default: break;
            }

            DrawString(TextFormat("%s", InventoryItem->Name),
                       GameState->BodyFont,
                       GameState->BodyFont->PointSize,
                       LineX, LineY, 0,
                       VA_WHITE,
                       false, VA_BLACK,
                       &GameState->ScratchArenaA);

            LineY += 40.0f;
        }
    }

    DrawInspectUI(&GameState->InspectState, GameState);

    if (GameState->InspectState.T == INSPECT_ITEM && GameState->InspectState.ButtonClicked)
    {
        Action->T = ACTION_DROP_ITEMS;
        DropItems->Items[DropItems->ItemCount++] = GameState->InspectState.IS_Item.ItemToInspect;
        GameState->InventorySkipSlot[GameState->InspectState.IS_Item.MenuSlotI] = true;
        EndInspect(&GameState->InspectState);
    }
            
    EndUIDraw();

    if (KeyPressed(SDL_SCANCODE_ESCAPE))
    {
        if (GameState->InspectState.T != INSPECT_NONE)
        {
            EndInspect(&GameState->InspectState);
        }
        else
        {
            memset(GameState->InventorySkipSlot, 0, INVENTORY_SLOTS_PER_ENTITY * sizeof(b32));
            return RUN_STATE_IN_GAME;
        }
    }

    if (KeyPressed(SDL_SCANCODE_I))
    {
        EndInspect(&GameState->InspectState);
        memset(GameState->InventorySkipSlot, 0, INVENTORY_SLOTS_PER_ENTITY * sizeof(b32));
        return RUN_STATE_IN_GAME;
    }

    return RUN_STATE_INVENTORY_MENU;
}
