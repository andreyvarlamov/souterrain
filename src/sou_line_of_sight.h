#ifndef SOU_LINE_OF_SIGHT_H
#define SOU_LINE_OF_SIGHT_H

internal_func void CalculateFOV(world *World, vec2i Pos, u8 *VisibilityMap, int MaxRange);
internal_func void CalculateExhaustiveFOV(world *World, vec2i Pos, u8 *VisibilityMap, int MaxRange);
internal_func b32 IsInLineOfSight(world *World, vec2i Start, vec2i End, int MaxRange);

#endif
