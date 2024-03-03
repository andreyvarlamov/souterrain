#ifndef VA_SSTRING_H
#define VA_SSTRING_H

#define SIMPLE_STRING_SIZE 128
struct simple_string
{
    u32 BufferSize = SIMPLE_STRING_SIZE;
    u32 Length = 0;
    char D[SIMPLE_STRING_SIZE];
};

inline simple_string
SimpleString(const char *String)
{
    simple_string Result = {};

    for (u32 StringIndex = 0;
         StringIndex < (Result.BufferSize - 1);
         ++StringIndex)
    {
        if (String[StringIndex] == '\0')
        {
            break;
        }
        
        Result.D[StringIndex] = String[StringIndex];
        Result.Length++;
    }

    Result.D[Result.Length] = '\0';

    return Result;
}

inline b32
ValidateIndexInString(const char *String, u32 Index)
{
    for (u32 StringIndex = 0;
         StringIndex < SIMPLE_STRING_SIZE; // TODO: Should this be more generic?
         ++StringIndex)
    {
        if (String[StringIndex] == '\0')
        {
            return false;
        }

        if (StringIndex >= Index)
        {
            return true;
        }
    }

    return false;
}

inline simple_string
SimpleString(const char *String, u32 StartIndex, u32 Count)
{
    simple_string Result = {};

    // TODO: If this is a valid scenario, need to handle when index is out of string range, instead of asserting
    Assert(ValidateIndexInString(String, StartIndex));

    for (u32 StringIndex = 0;
         StringIndex < Min(Count, (Result.BufferSize - 1));
         ++StringIndex)
    {
        if (String[StringIndex + StartIndex] == '\0')
        {
            break;
        }
        
        Result.D[StringIndex] = String[StringIndex + StartIndex];
        Result.Length++;
    }

    Result.D[Result.Length] = '\0';

    return Result;
}

#include <cstdio>
#include <cstdarg>

inline simple_string
SimpleStringF(const char *Format, ...)
{
    char TempBuf[SIMPLE_STRING_SIZE];

    // TODO: Don't use stdio and stdargs for this
    va_list VarArgs;
    va_start(VarArgs, Format);
    i32 SprintfResult = vsprintf_s(TempBuf, SIMPLE_STRING_SIZE - 1, Format, VarArgs);
    va_end(VarArgs);
    
    Assert(SprintfResult > 0);
    Assert(SprintfResult < SIMPLE_STRING_SIZE - 1);
    TempBuf[SprintfResult] = '\0';

    simple_string Result = SimpleString(TempBuf);
    return Result;
}

inline simple_string
CatStrings(const char *A, const char *B)
{
    simple_string Result {};

    u32 StringIndex = 0;
    for (StringIndex;
         StringIndex < (Result.BufferSize - 1);
         ++StringIndex)
    {
        if (A[StringIndex] == '\0')
        {
            break;
        }

        Result.D[StringIndex] = A[StringIndex];
        Result.Length++;
    }

    for (u32 BIndex = 0;
         StringIndex < (Result.BufferSize - 1);
         ++BIndex, ++StringIndex)
    {
        if (B[BIndex] == '\0')
        {
            break;
        }

        Result.D[StringIndex] = B[BIndex];
        Result.Length++;
    }

    Result.D[Result.Length] = '\0';

    return Result;
}

inline b32
CompareStrings(const char *A, const char *B)
{
    u32 Index = 0;

    while (A[Index] != '\0')
    {
        if (B[Index] == '\0' || (A[Index] != B[Index]))
        {
            return false;
        }

        Index++;
    }

    return (B[Index] == '\0');
}

// TODO: This shouldn't really be here

inline simple_string
GetDirectoryFromPath(const char *Path)
{
    simple_string Result {};

    i32 LastSlashIndex = -1;
    for (u32 StringIndex = 0;
         StringIndex < (Result.BufferSize - 1);
         ++StringIndex)
    {
        if (Path[StringIndex] == '\0')
        {
            break;
        }
        if (Path[StringIndex] == '/')
        {
            LastSlashIndex = StringIndex;
        }

        Result.D[StringIndex] = Path[StringIndex];
        Result.Length++;
    }

    if (LastSlashIndex != -1)
    {
        Result.Length = LastSlashIndex + 1;
    }

    Result.D[Result.Length] = '\0';

    return Result;
}

inline simple_string
GetFilenameFromPath(const char *Path, b32 IncludeExt)
{
    i32 LastSlashIndex = -1;
    for (u32 StringIndex = 0;
         StringIndex < (SIMPLE_STRING_SIZE - 1);
         ++StringIndex)
    {
        if (Path[StringIndex] == '\0')
        {
            break;
        }
        if (Path[StringIndex] == '/')
        {
            LastSlashIndex = StringIndex;
        }
    }
    
    i32 CountToExt = -1;
    if (!IncludeExt)
    {
        for (u32 StringIndex = LastSlashIndex + 1;
             StringIndex < (SIMPLE_STRING_SIZE - 1);
             ++StringIndex)
        {
            if (Path[StringIndex] == '\0')
            {
                break;
            }
                
            if (Path[StringIndex] == '.')
            {
                CountToExt = StringIndex - (LastSlashIndex + 1);
            }
        }

        if (CountToExt == 0) // If it's a dot-file (e.g. .emacs), keep the whole name
        {
            CountToExt = -1;
        }
    }

    simple_string Result = SimpleString(Path, LastSlashIndex + 1, (u32) CountToExt);
    return Result;
}

#endif
