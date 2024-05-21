#include "sou_turn_queue.h"

#include "va_common.h"
#include "sou_entity.h"

struct turn_queue_node
{
    entity *Entity;
    int TicksToAct;

    turn_queue_node *Previous;
    turn_queue_node *Next;
};

internal_func turn_queue
TurnQueueMake(memory_arena *Arena, int NodeMax)
{
    turn_queue TurnQueue = {};
    TurnQueue.NodeStore = MemoryArena_PushArrayAndZero(Arena, NodeMax, turn_queue_node);
    TurnQueue.NodeMax = NodeMax;
    return TurnQueue;
}

internal_func inline turn_queue_node *
_TurnQueueGetFreeNode(turn_queue *TurnQueue)
{
    turn_queue_node *FreeNode = TurnQueue->NextFreeNode;
    if (FreeNode != NULL)
    {
        TurnQueue->NextFreeNode = FreeNode->Next;
        *FreeNode = {};
    }
    else if (TurnQueue->NodeCount < TurnQueue->NodeMax)
    {
        FreeNode = TurnQueue->NodeStore + TurnQueue->NodeCount++;
    }
    Assert(FreeNode != NULL);

    return FreeNode;
}

internal_func inline void
_TurnQueueDeleteNode(turn_queue *TurnQueue, turn_queue_node *Node)
{
    turn_queue_node *Previous = Node->Previous;
    turn_queue_node *Next = Node->Next;

    if (Previous == NULL)
    {
        TurnQueue->NextNode = Next;
    }
    else
    {
        Previous->Next = Next;
    }

    if (Next != NULL)
    {
        Next->Previous = Previous;
    }

    *Node = {};

    Node->Next = TurnQueue->NextFreeNode;
    TurnQueue->NextFreeNode = Node;
}

internal_func inline void
_TurnQueueInsertNode(turn_queue *TurnQueue, turn_queue_node *NodeToInsert)
{
    turn_queue_node *PreviousNode = NULL;
    turn_queue_node *CurrentNode = TurnQueue->NextNode;
    while (CurrentNode)
    {
        if (CurrentNode->TicksToAct > NodeToInsert->TicksToAct)
        {
            if (CurrentNode->Previous)
            {
                CurrentNode->Previous->Next = NodeToInsert;
                NodeToInsert->Previous = CurrentNode->Previous;
                NodeToInsert->Next = CurrentNode;
                CurrentNode->Previous = NodeToInsert;
            }
            else
            {
                TurnQueue->NextNode = NodeToInsert;
                NodeToInsert->Next = CurrentNode;
                CurrentNode->Previous = NodeToInsert;
            }

            break;
        }
        else
        {
            PreviousNode = CurrentNode;
            CurrentNode = CurrentNode->Next;
        }
    }

    if (CurrentNode == NULL && PreviousNode != NULL)
    {
        PreviousNode->Next = NodeToInsert;
        NodeToInsert->Previous = PreviousNode;
    }
    else if (CurrentNode == NULL)
    {
        TurnQueue->NextNode = NodeToInsert;
    }
}

internal_func void
TurnQueueInsertEntity(turn_queue *TurnQueue, entity *Entity, int TicksToAct)
{
    turn_queue_node *NodeToInsert = _TurnQueueGetFreeNode(TurnQueue);
    NodeToInsert->Entity = Entity;
    NodeToInsert->TicksToAct = TicksToAct;

    _TurnQueueInsertNode(TurnQueue, NodeToInsert);
}

internal_func void
TurnQueueRemoveEntity(turn_queue *TurnQueue, entity *Entity)
{
    turn_queue_node *Current = TurnQueue->NextNode;
    while (Current)
    {
        if (Current->Entity == Entity)
        {
            _TurnQueueDeleteNode(TurnQueue, Current);
            // TODO: Remove this if same entity can be in queue multiple times
            return;
        }

        Current = Current->Next;
    }

    InvalidCodePath; // NOTE: Entity not in the queue was attempted to be removed
}

internal_func entity *
TurnQueuePopEntity(turn_queue *TurnQueue)
{
    turn_queue_node *TopNode = TurnQueue->NextNode;
    Assert(TopNode->TicksToAct == 0);

    entity *TopEntity = TopNode->Entity;

    _TurnQueueDeleteNode(TurnQueue, TopNode);

    return TopEntity;
}

internal_func entity *
TurnQueuePeekEntity(turn_queue *TurnQueue)
{
    return TurnQueue->NextNode->Entity;
}

internal_func int
TurnQueueAdvanceTicks(turn_queue *TurnQueue)
{
    turn_queue_node *TopNode = TurnQueue->NextNode;
    int TicksToAdvance = TopNode->TicksToAct;

    if (TicksToAdvance != 0)
    {
        for (int I = 0; I < TurnQueue->NodeCount; I++)
        {
            if (TurnQueue->NodeStore[I].Entity != NULL)
            {
                TurnQueue->NodeStore[I].TicksToAct -= TicksToAdvance;
            }
        }
    }

    return TicksToAdvance;
}
