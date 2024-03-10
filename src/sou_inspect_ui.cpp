#include "sav_lib.h"
#include "souterrain.h"

void
StartInspectEntity(inspect_state *InspectState, entity *Entity, const char *ButtonLabel = NULL)
{
    inspect_state NewInspectState = {};
    NewInspectState.T = INSPECT_ENTITY;
    NewInspectState.JustOpened = true;
    NewInspectState.IS_Entity.EntityToInspect = Entity;
    if (ButtonLabel)
    {
        NewInspectState.ButtonLabel = ButtonLabel;
    }
    
    *InspectState = NewInspectState;
}

void
StartInspectItem(inspect_state *InspectState, item *Item, entity *InventoryEntity, int MenuSlotI = 0, const char *ButtonLabel = NULL)
{
    inspect_state NewInspectState = {};
    NewInspectState.T = INSPECT_ITEM;
    NewInspectState.JustOpened = true;
    NewInspectState.IS_Item.ItemToInspect = Item;
    NewInspectState.IS_Item.InventoryEntity = InventoryEntity;
    NewInspectState.IS_Item.MenuSlotI = MenuSlotI;
    if (ButtonLabel)
    {
        NewInspectState.ButtonLabel = ButtonLabel;
    }
    
    *InspectState = NewInspectState;
}

void
EndInspect(inspect_state *InspectState)
{
    inspect_state NewInspectState = {};
    *InspectState = NewInspectState;
}

void
DrawInspectEntity(inspect_state *InspectState, game_state *GameState)
{
    entity *EntityToInspect = InspectState->IS_Entity.EntityToInspect;
                
    f32 LineX = 1510.0f;
    f32 LineY = 10.0f;
    f32 IncY = 50.0f;

    if (EntityToInspect->Name)
    {
        DrawString(TextFormat("%s (%d)", EntityToInspect->Name, EntityToInspect->DebugID),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   LineX, LineY, 0,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);
        LineY += IncY;
    }

    if (EntityToInspect->MaxHealth > 0)
    {
        DrawString(TextFormat("HP: %d/%d", EntityToInspect->Health, EntityToInspect->MaxHealth),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   LineX, LineY, 0,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);
        LineY += IncY;
                    
        DrawString(TextFormat("AC: %d", EntityToInspect->ArmorClass),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   LineX, LineY, 0,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);
        LineY += IncY;
                    
        DrawString(TextFormat("Damage: 1d%d", EntityToInspect->Damage),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   LineX, LineY, 0,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);
        LineY += IncY;
    }

    if (EntityToInspect->Type == ENTITY_NPC)
    {
        const char *Action;
        switch (EntityToInspect->NpcState)
        {
            case NPC_STATE_IDLE:
            {
                Action = "Idle";
            } break;
                        
            case NPC_STATE_HUNTING:
            {
                Action = "Hunting";
            } break;
                        
            case NPC_STATE_SEARCHING:
            {
                Action = "Searching";
            } break;

            default:
            {
                Action = "Unknown";
            } break;
        }

        DrawString(TextFormat("Action: %s", Action),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   LineX, LineY, 0,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);
        LineY += IncY;
    }
            
    if (EntityToInspect->Description)
    {
        DrawString(EntityToInspect->Description,
                   GameState->BodyFont,
                   GameState->BodyFont->PointSize,
                   LineX, LineY, 400,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);
    }
}

void
DrawInspectItem(inspect_state *InspectState, game_state *GameState)
{
    item *ItemToInspect = GameState->InspectState.IS_Item.ItemToInspect;
    entity *ItemPickup = GameState->InspectState.IS_Item.InventoryEntity;
                
    if (ItemToInspect->Name)
    {
        DrawString(TextFormat("%s", ItemToInspect->Name),
                   GameState->TitleFont,
                   GameState->TitleFont->PointSize,
                   1510, 10, 0,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);

        DrawString(TextFormat("%s", ItemToInspect->Description),
                   GameState->BodyFont,
                   GameState->BodyFont->PointSize,
                   1510, 60, 400,
                   VA_WHITE,
                   false, VA_BLACK,
                   &GameState->ScratchArenaA);

        if (ItemToInspect->HaimaBonus > 0)
        {
            DrawString(TextFormat("Haima Bonus: %d", ItemToInspect->HaimaBonus),
                       GameState->BodyFont,
                       GameState->BodyFont->PointSize,
                       1510, 100, 0,
                       VA_WHITE,
                       false, VA_BLACK,
                       &GameState->ScratchArenaA);
        }
    }
}

void
DrawInspectUI(inspect_state *InspectState, game_state *GameState)
{
    if (InspectState->T != INSPECT_NONE)
    {
        rect InspectRect = Rect(1500, 0, 420, 1080);
        DrawRect(InspectRect, ColorAlpha(VA_BLACK, 240));
        switch (InspectState->T)
        {
            case INSPECT_ENTITY:
            {
                DrawInspectEntity(InspectState, GameState);
            } break;

            case INSPECT_ITEM:
            {
                DrawInspectItem(InspectState, GameState);
            } break;
        
            default: break;
        }

        if (InspectState->ButtonLabel != NULL)
        {
            int ButtonPressed = GuiButtonRect(Rect(1510.0f, 1040.0f, InspectRect.Width - 20.0f, 40.0f));
            if (ButtonPressed == SDL_BUTTON_LEFT)
            {
                InspectState->ButtonClicked = true;
            }

            DrawString(TextFormat("%s", InspectState->ButtonLabel),
                       GameState->TitleFont,
                       GameState->TitleFont->PointSize,
                       1510, 1040, 0,
                       VA_WHITE,
                       false, VA_BLACK,
                       &GameState->ScratchArenaA);
        }
    }
}
