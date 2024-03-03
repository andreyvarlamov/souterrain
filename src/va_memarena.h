#ifndef VA_MEMARENA_H
#define VA_MEMARENA_H

#include "va_util.h"

struct memory_arena
{
    size_t Size;
    u8 *Base;
    size_t Used;
    size_t PrevUsed;
    size_t FrozenUsed;
    size_t FrozenPrevUsed;
#ifdef SAV_DEBUG
    b32 Frozen;
#endif
};

inline memory_arena
MemoryArena(u8 *Base, size_t Size)
{
    memory_arena Arena = {};
    
    Arena.Size = Size;
    Arena.Base = Base;

    return Arena;
}

inline void *
MemoryArena_PushSize_(memory_arena *Arena, size_t Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->PrevUsed = Arena->Used;
    Arena->Used += Size;
    return Result;
}

#include <cstring>

inline void *
MemoryArena_PushSizeAndZero_(memory_arena *Arena, size_t Size)
{
    void *Base = MemoryArena_PushSize_(Arena, Size);

#if 1
    memset(Base, 0, Size);
#else
    u8 *Cursor = (u8 *) Base;
    for (size_t ByteIndex = 0;
         ByteIndex < Size;
         ++ByteIndex)
    {
        *Cursor++ = 0;
    }
#endif
    
    return Base;
}

inline void
MemoryArena_ResizePreviousPushArray_(memory_arena *Arena, size_t Size)
{
    Arena->Used = Arena->PrevUsed + Size;
}

#define MemoryArena_PushStruct(Arena, type) (type *) MemoryArena_PushSize_(Arena, sizeof(type))
#define MemoryArena_PushArray(Arena, Count, type) (type *) MemoryArena_PushSize_(Arena, Count * sizeof(type))
#define MemoryArena_PushBytes(Arena, ByteCount) (u8 *) MemoryArena_PushSize_(Arena, ByteCount)
#define MemoryArena_PushArrayAndZero(Arena, Count, type) (type *) MemoryArena_PushSizeAndZero_(Arena, Count * sizeof(type))
#define MemoryArena_ResizePreviousPushArray(Arena, Count, type) MemoryArena_ResizePreviousPushArray_(Arena, Count * sizeof(type))

inline memory_arena
MemoryArenaNested(memory_arena *Arena, size_t Size)
{
    memory_arena NewArena = MemoryArena(MemoryArena_PushBytes(Arena, Size), Size);
    return NewArena;
}

inline void
MemoryArena_Freeze(memory_arena *Arena)
{
#ifdef SAV_DEBUG
    Assert(!Arena->Frozen);
    Arena->Frozen = true;
#endif
    Assert(Arena->FrozenUsed == 0);
    Arena->FrozenUsed = Arena->Used;
    Arena->FrozenPrevUsed = Arena->PrevUsed;
}

inline void
MemoryArena_Unfreeze(memory_arena *Arena)
{
#ifdef SAV_DEBUG
    Assert(Arena->Frozen);
    Arena->Frozen = false;
#endif
    Arena->Used = Arena->FrozenUsed;
    Arena->PrevUsed = Arena->FrozenPrevUsed;
    Arena->FrozenUsed = 0;
    Arena->FrozenPrevUsed = 0;
}

inline void
MemoryArena_Reset(memory_arena *Arena)
{
    Arena->Used = 0;
    Arena->PrevUsed = 0;
    Arena->FrozenUsed = 0;
    Arena->FrozenPrevUsed = 0;
}

#endif
