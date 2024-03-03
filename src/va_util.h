#ifndef VA_UTIL_H
#define VA_UTIL_H

#define static_g static
#define static_p static
#define static_i static

#ifdef SAV_DEBUG
  #define Assert(Expression) if (!(Expression)) { *(int *) 0 = 0; }
  #define InvalidCodePath Assert(!"Invalid Code Path")
  #define Noop { volatile int X = 0; }
  void __debugbreak(); // usually in <intrin.h>
  #define Breakpoint __debugbreak()
#else
  #define Assert(Expression)
  #define InvalidCodePath
  #define Noop
  #define Breakpoint
#endif

#define ArrayCount(Array) (sizeof((Array)) / (sizeof((Array)[0])))

#define Kilobytes(Value) (         (Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define Max(X, Y) (((X) > (Y)) ? (X) : (Y))
#define Min(X, Y) (((X) < (Y)) ? (X) : (Y))

#endif
