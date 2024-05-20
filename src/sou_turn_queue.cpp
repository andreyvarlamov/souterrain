#include "sou_turn_queue.h"

turn_queue
TurnQueueMake(memory_arena *Arena, int NodeMax)
{
    turn_queue TurnQueue = {};
    TurnQueue.NodeStore = MemoryArena_PushArrayAndZero(Arena, NodeMax, turn_queue_node);
    TurnQueue.NodeMax = NodeMax;
    return TurnQueue;
}

turn_queue_node *
TurnQueueGetFreeNode(turn_queue *TurnQueue)
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

void
TurnQueueDeleteNode(turn_queue *TurnQueue, turn_queue_node *Node)
{
    // TODO: Error here (if Previous or Nest is null)
    turn_queue_node *Previous = Node->Previous;
    turn_queue_node *Next = Node->Next;

    Previous->Next = Next;
    Next->Previous = Previous;

    *Node = {};

    Node->Next = TurnQueue->NextFreeNode;
    TurnQueue->NextFreeNode = Node;
}

inline void
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

void
TurnQueueInsertEntity(turn_queue *TurnQueue, entity *Entity, int TicksToAct)
{
    turn_queue_node *NodeToInsert = TurnQueueGetFreeNode(TurnQueue);
    NodeToInsert->Entity = Entity;
    NodeToInsert->TicksToAct = TicksToAct;

    _TurnQueueInsertNode(TurnQueue, NodeToInsert);
}

int
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

entity *
TurnQueuePeekEntity(turn_queue *TurnQueue)
{
    return TurnQueue->NextNode->Entity;
}

entity *
TurnQueuePopEntity(turn_queue *TurnQueue)
{
    turn_queue_node *TopNode = TurnQueue->NextNode;
    Assert(TopNode->TicksToAct == 0);

    entity *TopEntity = TopNode->Entity;

    TurnQueueDeleteNode(TurnQueue, TopNode);

    return TopEntity;
}