#include "sav_lib.h"
#include "souterrain.h"

run_state
RunState_PickupMenu(game_state *GameState)
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
    req_action_pickup_items *PickupItems = &Action->PickupItems;

    int MenuSlotI = 0;
            
    entity *ItemPickup = GetEntitiesAt(Player->Pos, World);
    while (ItemPickup)
    {
        b32 ShouldStop = false;

        if (ItemPickup->Type == ENTITY_ITEM_PICKUP)
        {
            item *ItemFromPickup = ItemPickup->Inventory;
            for (int i = 0; i < INVENTORY_SLOTS_PER_ENTITY; i++, ItemFromPickup++)
            {
                if (ItemFromPickup->ItemType != ITEM_NONE)
                {
                    if (!GameState->PickupSkipSlot[MenuSlotI])
                    {
                        int ButtonPressed = GuiButtonRect(Rect(LineX, LineY, InventoryWidth - 20.0f, 40.0f));
                        switch (ButtonPressed)
                        {
                            case SDL_BUTTON_LEFT:
                            {
                                StartInspectItem(&GameState->InspectState, ItemFromPickup, ItemPickup, MenuSlotI, "PICK UP");
                            } break;

                            case SDL_BUTTON_RIGHT:
                            {
                                Action->T = ACTION_PICKUP_ITEMS;
                                PickupItems->Items[PickupItems->ItemCount] = ItemFromPickup;
                                PickupItems->ItemPickups[PickupItems->ItemCount] = ItemPickup;
                                PickupItems->ItemCount++;
                                GameState->PickupSkipSlot[MenuSlotI] = true;
                                EndInspect(&GameState->InspectState);
                            } break;

                            default: break;
                        }

                        DrawString(TextFormat("%s", ItemFromPickup->Name),
                                   GameState->BodyFont,
                                   GameState->BodyFont->PointSize,
                                   LineX, LineY, 0,
                                   VA_WHITE,
                                   false, VA_BLACK,
                                   &GameState->ScratchArenaA);

                        LineY += 40.0f;

                    }

                    MenuSlotI++;
                    if (MenuSlotI == INVENTORY_SLOTS_PER_ENTITY)
                    {
                        ShouldStop = true;
                        break;
                    }
                }
            }
        }

        if (ShouldStop) break;

        ItemPickup = ItemPickup->Next;
    }

    DrawInspectUI(&GameState->InspectState, GameState);

    if (GameState->InspectState.T == INSPECT_ITEM && GameState->InspectState.ButtonClicked)
    {
        Action->T = ACTION_PICKUP_ITEMS;
        PickupItems->Items[PickupItems->ItemCount] = GameState->InspectState.IS_Item.ItemToInspect;
        PickupItems->ItemPickups[PickupItems->ItemCount] = GameState->InspectState.IS_Item.InventoryEntity;
        PickupItems->ItemCount++;
        GameState->PickupSkipSlot[GameState->InspectState.IS_Item.MenuSlotI] = true;
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
            memset(GameState->PickupSkipSlot, 0, INVENTORY_SLOTS_PER_ENTITY * sizeof(b32));
            return RUN_STATE_IN_GAME;
        }
    }
    
    if (KeyPressed(SDL_SCANCODE_I))
    {
        EndInspect(&GameState->InspectState);
        memset(GameState->PickupSkipSlot, 0, INVENTORY_SLOTS_PER_ENTITY * sizeof(b32));
        return RUN_STATE_IN_GAME;
    }

    if (KeyPressed(SDL_SCANCODE_G))
    {
        EndInspect(&GameState->InspectState);
        GameState->PlayerReqAction.T = ACTION_PICKUP_ALL_ITEMS;
        memset(GameState->PickupSkipSlot, 0, INVENTORY_SLOTS_PER_ENTITY * sizeof(b32));
        return RUN_STATE_IN_GAME;
    }

    return RUN_STATE_PICKUP_MENU;
}
