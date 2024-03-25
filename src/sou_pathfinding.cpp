f32
GetHeuristic(vec2i Start, vec2i End)
{
    #if 1
    // NOTE: Sq Dist. Really unadmissible. Fast but very unoptimal path.
    f32 dX = (f32) (End.X - Start.X);
    f32 dY = (f32) (End.Y - Start.Y);
    return dX*dX + dY*dY;
    #elif 0
    // NOTE: Manhattan. Slightly unadmissible. Fast ish, somewhat optimal path.
    return AbsF((f32) ((End.X - Start.X) + (End.Y - Start.Y)));
    #else
    // NOTE: Euclidian distance. Admissible. Slow calculation, way more iterations and guaranteed optimal path.
    f32 dX = (f32) (End.X - Start.X);
    f32 dY = (f32) (End.Y - Start.Y);
    return SqrtF(dX*dX + dY*dY);
    #endif
}

b32
PopLowestScoreFromOpenSet(path_state *PathState, int *LowestScoreIdx)
{
    // NOTE: One of the first optimizations that could be done here is to convert this to a min heap
    if (PathState->OpenSetCount > 0)
    {
        f32 LowestScore = FLT_MAX;
        int LowestScoreOpenSetI = 0;
        for (int OpenSetI = 0; OpenSetI < PathState->OpenSetCount; OpenSetI++)
        {
            int Idx = PathState->OpenSet[OpenSetI];

            if (PathState->FScores[Idx] < LowestScore)
            {
                LowestScore = PathState->FScores[Idx];
                LowestScoreOpenSetI = OpenSetI;
                *LowestScoreIdx = Idx;
            }
        }

        for (int OpenSetI = LowestScoreOpenSetI; OpenSetI < PathState->OpenSetCount - 1; OpenSetI++)
        {
            PathState->OpenSet[OpenSetI] = PathState->OpenSet[OpenSetI + 1];
        }
        PathState->OpenSetCount--;
        return true;
    }

    return false;
}

b32
IsInOpenSet(path_state *PathState, int Idx)
{
    for (int i = 0; i < PathState->OpenSetCount; i++)
    {
        if (PathState->OpenSet[i] == Idx)
        {
            return true;
        }
    }

    return false;
}

// static_g int CalculatePathCalls = 0;
// static_g int OpenSetMax = 0;

// void ResetPathCallCount() { CalculatePathCalls = 0; OpenSetMax = 0; }
// int GetPathCallCount() { return CalculatePathCalls; }
// int GetPathOpenSetMax() { return OpenSetMax; }

void
AddToOpenSet(path_state *PathState, int Idx)
{
    if (!IsInOpenSet(PathState, Idx))
    {
        PathState->OpenSet[PathState->OpenSetCount++] = Idx;
        
        // if (PathState->OpenSetCount > OpenSetMax)
        // {
        //     OpenSetMax = PathState->OpenSetCount;
        // }
    }
}

void
GetNeighbors(vec2i Pos, world *World,
             vec2i *Neighbors, b32 *DiagonalNeighbors, int *NeighborCount)
{
    *NeighborCount = 0;
    
    for (int Dir = 0; Dir < 8; Dir++)
    {
        vec2i Neighbor = Pos + DIRECTIONS[Dir];

        if (Neighbor.X >= 0 && Neighbor.X < World->Width &&
            Neighbor.Y >= 0 && Neighbor.Y < World->Height)
        {
            DiagonalNeighbors[*NeighborCount] = (Dir % 2 == 1);
            Neighbors[*NeighborCount] = Neighbor;
            (*NeighborCount)++;
        }
    }
}

#undef VIZ

collision_info CheckCollisions(world *World, vec2i P, b32 IgnoreNPCs = false);

path_result
CalculatePath(world *World, vec2i Start, vec2i End, memory_arena *TrArena, memory_arena *ResultArena, int MaxIterations = 0xFFFFFFFF, b32 IgnoreNPCs = false, int VizGenMax = 0)
{
    // CalculatePathCalls++;
    
    int VizGen = 0;
    path_result Result = {};

    MemoryArena_Freeze(TrArena);

#ifdef VIZ
    if (VizGen >= VizGenMax)
    {
        goto routine_end;
    }
    VizGen++;
#endif

    path_state PathState;
    PathState.OpenSet = MemoryArena_PushArray(TrArena, OPEN_SET_MAX, int);
    PathState.OpenSetCount = 0;

    PathState.MapSize = World->Width * World->Height;
    // NOTE: For unbounded maps, or just big maps, these could be hash tables
    PathState.CameFrom = MemoryArena_PushArray(TrArena, PathState.MapSize, int);
    PathState.GScores = MemoryArena_PushArray(TrArena, PathState.MapSize, f32);
    PathState.FScores = MemoryArena_PushArray(TrArena, PathState.MapSize, f32);
    for (int i = 0; i < PathState.MapSize; i++)
    {
        PathState.GScores[i] = FLT_MAX;
        PathState.FScores[i] = FLT_MAX;
    }

    int StartIdx = XYToIdx(Start, World->Width);
    PathState.OpenSet[PathState.OpenSetCount++] = StartIdx;
    PathState.CameFrom[StartIdx] = 0;
    PathState.GScores[StartIdx] = 0;
    PathState.FScores[StartIdx] = GetHeuristic(Start, End);

    int EndIdx = XYToIdx(End, World->Width);

    if (StartIdx == EndIdx)
    {
        Result.FoundPath = true;
        MemoryArena_Unfreeze(TrArena);
        return Result;
    }

    b32 FoundPath = false;
    int CurrentIdx;
    int IterationCount = 0;
    while (PopLowestScoreFromOpenSet(&PathState, &CurrentIdx))
    {
        if (CurrentIdx == EndIdx)
        {
            FoundPath = true;
            break;
        }

        vec2i Neighbors[8];
        b32 DiagonalNeighbors[8];
        int NeighborCount;
        GetNeighbors(IdxToXY(CurrentIdx, World->Width), World, Neighbors, DiagonalNeighbors, &NeighborCount);

        for (int i = 0; i < NeighborCount; i++)
        {
            vec2i Neighbor = Neighbors[i];
            b32 Diagonal = DiagonalNeighbors[i];
            f32 ThisGScore = PathState.GScores[CurrentIdx] + (Diagonal ? 1.414f : 1.0f);
            
            int NeighborIdx = XYToIdx(Neighbor, World->Width);
            if (ThisGScore < PathState.GScores[NeighborIdx])
            {
                // TODO: This can be optimized by caching results of collisions.
                // Right now if a cell gets rejected because there is a collision, it will recheck collisions again
                // from another currentIdx position.
                // In addition, if a cell has already been discovered with a higher GScore, we know that there was no collision,
                // but this is gonna check collisions for that cell again.
                b32 Collided = (NeighborIdx == EndIdx) ? false : CheckCollisions(World, Neighbor, IgnoreNPCs).Collided;
                if (!Collided)
                {
                    PathState.CameFrom[NeighborIdx] = CurrentIdx;
                    PathState.GScores[NeighborIdx] = ThisGScore;
                    PathState.FScores[NeighborIdx] = ThisGScore + GetHeuristic(Neighbor, End);

                    AddToOpenSet(&PathState, NeighborIdx);
                }
            }
        }

#ifdef VIZ
        if (VizGen >= VizGenMax)
        {
            int I = 0;
            int PrevIdx = CurrentIdx;
            while (PrevIdx != StartIdx && I < PATH_MAX)
            {
                DrawRect(World, IdxToXY(PrevIdx, World->Width), I == 0 ? ColorAlpha(VA_YELLOW, 200) : ColorAlpha(VA_BLUE, 100));
                I++;
                PrevIdx = PathState.CameFrom[PrevIdx];
            }
            DrawRect(World, Start, ColorAlpha(VA_RED, 200));

            goto routine_end;
        }
        VizGen++;
#endif

        if (IterationCount++ > MaxIterations) break;
    }

    Result.FoundPath = FoundPath;
    
    if (FoundPath)
    {
        Result.Path = MemoryArena_PushArray(ResultArena, PATH_MAX, vec2i);

        int Count = 0;
        // NOTE: Reconstitute path backwards
        int PrevIdx = EndIdx;
        while (PrevIdx != StartIdx && Count < PATH_MAX)
        {
            Result.Path[Count++] = IdxToXY(PrevIdx, World->Width);
            PrevIdx = PathState.CameFrom[PrevIdx];
        }

        Assert(Count < PATH_MAX);

        // NOTE: And then reverse it
        for (int i = 0; i < Count / 2; i++)
        {
            vec2i Temp = Result.Path[i];
            Result.Path[i] = Result.Path[Count - 1  - i];
            Result.Path[Count - 1  - i] = Temp;
        }
        Result.PathSteps = Count;
        MemoryArena_ResizePreviousPushArray(ResultArena, Count, vec2i);
    }

#ifdef VIZ
 routine_end:
#endif
    MemoryArena_Unfreeze(TrArena);

    return Result;
}
