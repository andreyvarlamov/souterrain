b32 IsTileOpaque(world *World, vec2i P);

void
TraceLineBresenham(world *World, vec2i A, vec2i B, u8 *VisibilityMap, int MaxRangeSq)
{
    int X1 = A.X;
    int Y1 = A.Y;
    int X2 = B.X;
    int Y2 = B.Y;
    
    int DeltaX = X2 - X1;
    int IX = ((DeltaX > 0) - (DeltaX < 0));
    DeltaX = Abs(DeltaX) << 1;

    int DeltaY = Y2 - Y1;
    int IY = ((DeltaY > 0) - (DeltaY < 0));
    DeltaY = Abs(DeltaY) << 1;

    VisibilityMap[XYToIdx(X1, Y1, World->Width)] = 1;

    if (DeltaX >= DeltaY)
    {
        int Error = (DeltaY - (DeltaX >> 1));

        while (X1 != X2)
        {
            if ((Error > 0) || (!Error && (IX > 0)))
            {
                Error -= DeltaX;
                Y1 += IY;
            }

            Error += DeltaY;
            X1 += IX;

            VisibilityMap[XYToIdx(X1, Y1, World->Width)] = 1;

            if ((X1 - A.X)*(X1 - A.X) + (Y1 - A.Y)*(Y1 - A.Y) >= MaxRangeSq)
            {
                break;
            }

            if (IsTileOpaque(World, Vec2I(X1, Y1)))
            {
                break;
            }
        }
    }
    else
    {
        int Error = (DeltaX - (DeltaY >> 1));

        while (Y1 != Y2)
        {
            if ((Error > 0) || (!Error && (IY > 0)))
            {
                Error -= DeltaY;
                X1 += IX;
            }

            Error += DeltaX;
            Y1 += IY;

            VisibilityMap[XYToIdx(X1, Y1, World->Width)] = 1;

            if ((X1 - A.X)*(X1 - A.X) + (Y1 - A.Y)*(Y1 - A.Y) >= MaxRangeSq)
            {
                break;
            }
            
            if (IsTileOpaque(World, Vec2I(X1, Y1)))
            {
                break;
            }
        }
    }

}

void
CalculateFOV(world *World, vec2i Pos, u8 *VisibilityMap, int MaxRange)
{
    int MaxRangeSq = MaxRange*MaxRange;
    
    for (int X = 0; X < World->Width; X++)
    {
        TraceLineBresenham(World, Pos, Vec2I(X, 0), VisibilityMap, MaxRangeSq);
        TraceLineBresenham(World, Pos, Vec2I(X, World->Height - 1), VisibilityMap, MaxRangeSq);
    }

    for (int Y = 0; Y < World->Height; Y++)
    {
        TraceLineBresenham(World, Pos, Vec2I(0, Y), VisibilityMap, MaxRangeSq);
        TraceLineBresenham(World, Pos, Vec2I(World->Width - 1, Y), VisibilityMap, MaxRangeSq);
    }
}

void
CalculateExhaustiveFOV(world *World, vec2i Pos, u8 *VisibilityMap, int MaxRange)
{
    int MaxRangeSq = MaxRange*MaxRange;

    int StartX = Max(Pos.X - MaxRange, 0);
    int EndX = Min(Pos.X + MaxRange, World->Width - 1);
    int StartY = Max(Pos.Y - MaxRange, 0);
    int EndY = Min(Pos.Y + MaxRange, World->Height - 1);
    
    for (int Y = StartY; Y <= EndY; Y++)
    {
        for (int X = StartX; X <= EndX; X++)
        {
            if (!VisibilityMap[XYToIdx(X, Y, World->Width)])
            {
                TraceLineBresenham(World, Pos, Vec2I(X, Y), VisibilityMap, MaxRangeSq);
            }
        }
    }
}

b32
IsInLineOfSight(world *World, vec2i Start, vec2i End, int MaxRange)
{
    if (VecLengthSq(End - Start) <= MaxRange*MaxRange)
    {
        int CurrentX = Start.X;
        int CurrentY = Start.Y;
        int EndX = End.X;
        int EndY = End.Y;
    
        int DeltaX = EndX - CurrentX;
        int IX = ((DeltaX > 0) - (DeltaX < 0));
        DeltaX = Abs(DeltaX) << 1;

        int DeltaY = EndY - CurrentY;
        int IY = ((DeltaY > 0) - (DeltaY < 0));
        DeltaY = Abs(DeltaY) << 1;

        if (DeltaX >= DeltaY)
        {
            int Error = (DeltaY - (DeltaX >> 1));
            while (CurrentX != EndX)
            {
                if ((Error > 0) || (!Error && (IX > 0)))
                {
                    Error -= DeltaX;
                    CurrentY += IY;
                }

                Error += DeltaY;
                CurrentX += IX;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                if (TestPos == End) return true;
                if (IsTileOpaque(World, TestPos)) break;
            }
        }
        else
        {
            int Error = (DeltaX - (DeltaY >> 1));
            while (CurrentY != EndY)
            {
                if ((Error > 0) || (!Error && (IY > 0)))
                {
                    Error -= DeltaY;
                    CurrentX += IX;
                }

                Error += DeltaX;
                CurrentY += IY;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                if (TestPos == End) return true;
                if (IsTileOpaque(World, TestPos)) break;
            }
        }
    }
    
    return false;
}

b32
IsInRangedAttackRange(world *World, vec2i Start, vec2i End, int MaxRange)
{
    if (VecLengthSq(End - Start) <= MaxRange*MaxRange)
    {
        int CurrentX = Start.X;
        int CurrentY = Start.Y;
        int EndX = End.X;
        int EndY = End.Y;
    
        int DeltaX = EndX - CurrentX;
        int IX = ((DeltaX > 0) - (DeltaX < 0));
        DeltaX = Abs(DeltaX) << 1;

        int DeltaY = EndY - CurrentY;
        int IY = ((DeltaY > 0) - (DeltaY < 0));
        DeltaY = Abs(DeltaY) << 1;

        if (DeltaX >= DeltaY)
        {
            int Error = (DeltaY - (DeltaX >> 1));
            while (CurrentX != EndX)
            {
                if ((Error > 0) || (!Error && (IX > 0)))
                {
                    Error -= DeltaX;
                    CurrentY += IY;
                }

                Error += DeltaY;
                CurrentX += IX;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                collision_info Col = CheckCollisions(World, TestPos);
                
                // NOTE: If encountered non npc collision - it can't be hit in ranged, even if it's our target
                if (IsTileOpaque(World, TestPos) ||
                    ((Col.Collided && Col.Entity == NULL) ||
                     (Col.Entity && Col.Entity->Type != ENTITY_NPC)))
                {
                    return false;
                }

                if (TestPos == End) return true;
                
                // NOTE: If encountered npc collision - it can be hit in ranged,
                //       so if it was the target we would've returned true,
                //       but if it's not, don't continue tracing line beyond that
                if (Col.Entity && Col.Entity->Type == ENTITY_NPC) return false;
            }
        }
        else
        {
            int Error = (DeltaX - (DeltaY >> 1));
            while (CurrentY != EndY)
            {
                if ((Error > 0) || (!Error && (IY > 0)))
                {
                    Error -= DeltaY;
                    CurrentX += IX;
                }

                Error += DeltaX;
                CurrentY += IY;

                vec2i TestPos = Vec2I(CurrentX, CurrentY);
                collision_info Col = CheckCollisions(World, TestPos);

                // NOTE: If encountered non npc collision - it can't be hit in ranged, even if it's our target
                if (IsTileOpaque(World, TestPos) ||
                    ((Col.Collided && Col.Entity == NULL) ||
                     (Col.Entity && Col.Entity->Type != ENTITY_NPC)))
                {
                    return false;
                }

                if (TestPos == End) return true;
                
                // NOTE: If encountered npc collision - it can be hit in ranged,
                //       so if it was the target we would've returned true,
                //       but if it's not, don't continue tracing line beyond that
                if (Col.Entity && Col.Entity->Type == ENTITY_NPC) return false;
            }
        }
    }

    return false;
}
